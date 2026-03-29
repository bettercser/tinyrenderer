#include "tracer.hpp"
#include "hit_record.hpp"
#include "light.hpp"
#include "material.hpp"
#include "texture_mip.hpp"
#include "tracer_config.hpp"
#include "utils/render_util.hpp"
#include <algorithm>

namespace {

constexpr double PI = 3.14159265358979323846;

struct MaterialSample {
  Vec<3> base_color;
  Vec<3> shading_normal;
  double roughness;
  double metallic;
};

struct SphereLightSample {
  Vec<3> point;
  Vec<3> normal;
  double pdf = 0.0;
  Vec<3> direction;
  double distance = 0.0;
};

struct BsdfSample {
  Ray scattered{{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
  Vec<3> attenuation{1.0, 1.0, 1.0};
  double pdf = 1.0;
  bool valid = false;
  bool is_delta = false;
};

struct OrthonormalBasis {
  Vec<3> u;
  Vec<3> v;
  Vec<3> w;
};

OrthonormalBasis build_orthonormal_basis(const Vec<3> &n) {
  OrthonormalBasis basis;
  basis.w = n.normalized();

  Vec<3> a = (std::abs(basis.w[0]) > 0.9) ? Vec<3>{0.0, 1.0, 0.0}
                                          : Vec<3>{1.0, 0.0, 0.0};

  basis.v = basis.w.cross(a).normalized();
  basis.u = basis.v.cross(basis.w);

  return basis;
}

Vec<3> local_to_world(const OrthonormalBasis &basis, const Vec<3> &local_dir) {
  return basis.u * local_dir[0] + basis.v * local_dir[1] +
         basis.w * local_dir[2];
}

double power_heuristic(double pdf_a, double pdf_b) {
  double a2 = pdf_a * pdf_a;
  double b2 = pdf_b * pdf_b;
  if (a2 + b2 <= 0.0) {
    return 0.0;
  }
  return a2 / (a2 + b2);
}

Vec<3> random_cosine_direction() {
  double r1 = random_double();
  double r2 = random_double();

  double phi = 2.0 * PI * r1;
  double x = std::cos(phi) * std::sqrt(r2);
  double y = std::sin(phi) * std::sqrt(r2);
  double z = std::sqrt(1.0 - r2);

  return Vec<3>{x, y, z};
}

double estimate_emissive_light_pdf(const Scene &scene,
                                   const HitRecord &light_hit) {
  for (const auto &light : scene.lights) {
    Vec<3> delta = light_hit.point - light.position;
    double dist2 = delta * delta;
    double radius2 = light.radius * light.radius;

    if (std::abs(dist2 - radius2) < 1e-3) {
      double area = 4.0 * PI * light.radius * light.radius;
      if (area > 0.0) {
        return 1.0 / area;
      }
    }
  }

  return 0.0;
}

double evaluate_lambertian_pdf(const Vec<3> &normal, const Vec<3> &wi) {
  double cos_theta = std::max(normal * wi, 0.0);
  return cos_theta / PI;
}

double estimate_mip_level_f(const TextureMipChain &chain,
                            const HitRecord &rec) {
  if (chain.empty()) {
    return 0.0;
  }

  double d = rec.view_distance;
  double level = 0.0;

  if (d > 1.0) {
    level = std::log2(d);
  }

  return std::clamp(level, 0.0, static_cast<double>(chain.levels() - 1));
}

Vec<3> sample_diffuse_texture(const Material &material, const HitRecord &rec) {
  if (material.diffuse_mips && !material.diffuse_mips->empty()) {
    TGAColor tex =
        sample_mip_trilinear(*material.diffuse_mips, rec.uv,
                             estimate_mip_level_f(*material.diffuse_mips, rec));

    return Vec<3>{tex[2] / 255.0, tex[1] / 255.0, tex[0] / 255.0};
  }

  if (material.use_diffuse_texture && material.diffuse_texture) {
    TGAColor tex = material.diffuse_texture->diffuse(rec.uv);

    return Vec<3>{tex[2] / 255.0, tex[1] / 255.0, tex[0] / 255.0};
  }
  return material.base_color;
}

Vec<3> sample_normal_texture(const Material &material, const HitRecord &rec) {

  if (material.normal_mips && !material.normal_mips->empty()) {
    TGAColor tex =
        sample_mip_trilinear(*material.normal_mips, rec.uv,
                             estimate_mip_level_f(*material.normal_mips, rec));

    Vec<3> tangent_normal{tex[2] / 255.0, tex[1] / 255.0, tex[0] / 255.0};
    tangent_normal = tangent_normal * 2.0 - Vec<3>{1.0, 1.0, 1.0};
    return tangent_normal;
  }
  if (material.use_normal_texture && material.normal_texture) {
    Vec<4> sampled = material.normal_texture->get_normal(rec.uv);
    return Vec<3>{sampled[0], sampled[1], sampled[2]};
  }

  return Vec<3>{0.0, 0.0, 1.0};
}

double sample_specular_texture(const Material &material, const HitRecord &rec) {

  if (material.specular_mips && !material.specular_mips->empty()) {
    TGAColor tex = sample_mip_trilinear(
        *material.specular_mips, rec.uv,
        estimate_mip_level_f(*material.specular_mips, rec));

    return tex[0] / 255.0; // 假设镜面反射强度存储在红色通道
  }

  if (material.use_specular_texture && material.specular_texture) {
    return material.specular_texture->specular(rec.uv);
  }

  return 1.0 - material.roughness;
}

double evaluate_roughness(const HitRecord &rec) {
  if (!rec.material) {
    return 0.0; // 默认粗糙度
  }

  double roughness = rec.material->roughness;

  double spec =
      sample_specular_texture(*rec.material, rec); // 从贴图获取镜面反射值
  roughness = 1.0 - std::clamp(spec, 0.0, 1.0);    // 将贴图值转换为粗糙度
  return std::clamp(roughness, 0.0, 1.0);
}

Vec<3> evaluate_shading_normal(const HitRecord &rec) {
  if (!rec.material) {
    return rec.normal;
  }

  if (rec.material->use_normal_texture && rec.material->normal_texture) {

    Vec<3> tangent_normal = sample_normal_texture(*rec.material, rec);
    tangent_normal = tangent_normal.normalized();

    Vec<3> world_normal =
        (rec.tangent * tangent_normal[0] + rec.bitangent * tangent_normal[1] +
         rec.normal * tangent_normal[2])
            .normalized();
    return world_normal;
  }

  return rec.normal;
}

Vec<3> evaluate_base_color(const HitRecord &rec) {
  if (!rec.material) {
    return Vec<3>{1.0, 1.0, 1.0};
  }

  return sample_diffuse_texture(*rec.material, rec);
}

MaterialSample evaluate_material_sample(const HitRecord &rec) {
  MaterialSample sample;
  sample.base_color = evaluate_base_color(rec);
  sample.shading_normal = evaluate_shading_normal(rec);
  sample.roughness = evaluate_roughness(rec);
  sample.metallic = rec.material ? rec.material->metallic : 0.0;
  return sample;
}

bool is_in_shadow(const Scene &scene, const Vec<3> &point, const Vec<3> &normal,
                  const Vec<3> &light_dir, const TracerConfig &config) {
  Ray shadow_ray(point + normal * config.ray_epsilon, light_dir);
  HitRecord temp_rec;
  return scene.hit(shadow_ray, 0.001, std::numeric_limits<double>::max(),
                   temp_rec);
}

Vec<3> evaluating_directional_direct_lighting(const Scene &scene,
                                              const HitRecord &rec,
                                              const DirectionalLight &light,
                                              const TracerConfig &config) {

  if (!rec.material || rec.material->type != MaterialType::Lambertian) {
    return Vec<3>{0.0, 0.0, 0.0};
  }
  Vec<3> light_dir = (-light.direction).normalized();

  MaterialSample sample = evaluate_material_sample(rec);

  if (is_in_shadow(scene, rec.point, sample.shading_normal, light_dir,
                   config)) {
    return Vec<3>{0.0, 0.0, 0.0}; // 在阴影中，返回黑色
  }

  double n_dot_l = std::max(0.0, sample.shading_normal * light_dir);
  if (n_dot_l <= 0.0) {
    return Vec<3>{0.0, 0.0, 0.0}; // 法线背向光源，返回黑色
  }
  Vec<3> base_color = sample.base_color;

  return Vec<3>{base_color[0] * light.color[0], base_color[1] * light.color[1],
                base_color[2] * light.color[2]} *
         (light.intensity * n_dot_l);
}

Vec<3> sky_color(const Ray &ray) {
  Vec<3> unit_dir = ray.direction.normalized();
  double t = 0.5 * (unit_dir[1] + 1.0);
  return Vec<3>{1.0, 1.0, 1.0} * (1.0 - t) +
         Vec<3>{0.5, 0.7, 1.0} * t; // 线性插值背景色
}

// 此函数用于计算 折射光线的方向
// 通过叠加折射光在垂直于法线方向平面内的分量与折射光沿法线方向的分量
Vec<3> refract(const Vec<3> &uv, const Vec<3> &n, double etai_over_etat) {

  double cos_theta = std::min(-uv * n, 1.0);

  Vec<3> r_out_perp = (uv + n * cos_theta) * etai_over_etat;
  Vec<3> r_out_parallel =
      n * -std::sqrt(std::abs(1.0 - r_out_perp * r_out_perp));

  return r_out_perp + r_out_parallel;
}

// 此函数用于计算 在某个角度下光线被反射的概率
// 通过 Schlick 近似计算
double reflectance(double cosine, double ref_idx) {
  double r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
  r0 = r0 * r0;
  return r0 + (1.0 - r0) * std::pow((1.0 - cosine), 5);
}

Vec<3> random_in_unit_sphere() {
  Vec<3> p;
  do {
    p = Vec<3>{random_double(), random_double(), random_double()} * 2.0 -
        Vec<3>{1.0, 1.0, 1.0};
  } while (p * p >= 1.0);
  return p;
}

Vec<3> random_unit_vector() { return random_in_unit_sphere().normalized(); }

Vec<3> reflect(const Vec<3> &v, const Vec<3> &n) {
  return v - n * (2.0 * (v * n));
}
BsdfSample scatter_lambertian(const HitRecord &rec,
                              const TracerConfig &config) {

  BsdfSample bsdf;
  MaterialSample sample = evaluate_material_sample(rec);

  OrthonormalBasis basis = build_orthonormal_basis(sample.shading_normal);
  Vec<3> local_dir = random_cosine_direction();
  Vec<3> scatter_dir = local_to_world(basis, local_dir).normalized();

  double cos_theta = std::max(sample.shading_normal * scatter_dir, 0.0);
  bsdf.pdf = cos_theta / PI;

  if (bsdf.pdf <= 0.0) {
    bsdf.valid = false;
    return bsdf; // PDF 为零或负数，丢弃
  }

  bsdf.scattered =
      Ray(rec.point + sample.shading_normal * config.ray_epsilon, scatter_dir);
  bsdf.attenuation = sample.base_color;
  bsdf.valid = true;
  bsdf.is_delta = false;
  return bsdf;
}

BsdfSample scatter_metal(const HitRecord &rec, const Ray &incoming,
                         const TracerConfig &config) {

  BsdfSample bsdf;
  MaterialSample sample = evaluate_material_sample(rec);
  Vec<3> reflected =
      reflect(incoming.direction.normalized(), sample.shading_normal);
  double fuzz = sample.roughness;

  Vec<3> scattered_dir = reflected + random_in_unit_sphere() * fuzz;

  if (scattered_dir * sample.shading_normal <= 0) {
    bsdf.valid = false;
    return bsdf; // 散射方向与法线相反，丢弃
  }

  bsdf.scattered = Ray(rec.point + sample.shading_normal * config.ray_epsilon,
                       scattered_dir);
  bsdf.attenuation = sample.base_color;
  bsdf.pdf = 1.0;
  bsdf.valid = true;
  bsdf.is_delta = true;
  return bsdf;
}

BsdfSample scatter_dielectric(const HitRecord &rec, const Ray &incoming,
                              const TracerConfig &config) {

  BsdfSample bsdf;
  bsdf.attenuation =
      rec.material ? rec.material->transmission_color : Vec<3>{1.0, 1.0, 1.0};

  double refraction_ratio =
      rec.front_face ? (1.0 / rec.material->ior) : rec.material->ior;

  Vec<3> unit_dir = incoming.direction.normalized();

  double cos_theta = std::min(-unit_dir * rec.normal, 1.0);
  double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

  bool cannot_refract = refraction_ratio * sin_theta > 1.0;

  Vec<3> direction;

  if (cannot_refract ||
      reflectance(cos_theta, refraction_ratio) > random_double()) {
    direction = reflect(unit_dir, rec.normal);
  } else {
    direction = refract(unit_dir, rec.normal, refraction_ratio);
  }

  bsdf.scattered = Ray(rec.point + direction * config.ray_epsilon, direction);
  bsdf.pdf = 1.0;
  bsdf.valid = true;
  bsdf.is_delta = true;
  return bsdf;
}

BsdfSample scatter_material(const Ray &incoming, const HitRecord &rec,
                            const TracerConfig &config) {

  BsdfSample bsdf;
  if (!rec.material) {
    bsdf.attenuation = Vec<3>{1.0, 1.0, 1.0};
    bsdf.scattered =
        Ray(rec.point + rec.normal * config.ray_epsilon, rec.normal);
    bsdf.valid = true;
    bsdf.pdf = 1.0;
    return bsdf;
  }

  switch (rec.material->type) {

  case MaterialType::Lambertian:
    return scatter_lambertian(rec, config);

  case MaterialType::Metal:
    return scatter_metal(rec, incoming, config);
  case MaterialType::Dielectric:
    return scatter_dielectric(rec, incoming, config);
  }

  return bsdf;
}

SphereLightSample sample_sphere_light(const LightRecord &light,
                                      const Vec<3> &shading_point) {
  Vec<3> dir = random_unit_vector();

  SphereLightSample sample;
  sample.point = light.position + dir * light.radius;
  sample.normal = dir.normalized();

  Vec<3> to_light = sample.point - shading_point;
  sample.distance = norm(to_light);
  sample.direction = to_light / sample.distance;

  double area = 4.0 * PI * light.radius * light.radius;
  sample.pdf = area > 0.0 ? 1.0 / area : 0.0;

  return sample;
}

bool is_emissive(const HitRecord &rec) {
  if (!rec.material) {
    return false;
  }
  const Vec<3> &e = rec.material->emission;
  return e[0] > 0.0 || e[1] > 0.0 || e[2] > 0.0;
}

Vec<3> evaluate_emission(const HitRecord &rec) {
  if (!rec.material) {
    return Vec<3>{0.0, 0.0, 0.0};
  }
  return rec.material->emission;
}

Vec<3> evaluate_emissive_direct_lighting(const Scene &scene,
                                         const HitRecord &rec,
                                         const TracerConfig &config) {

  if (!rec.material || rec.material->type != MaterialType::Lambertian) {
    return Vec<3>{0.0, 0.0, 0.0};
  }
  MaterialSample sample = evaluate_material_sample(rec);
  Vec<3> result{0.0, 0.0, 0.0};

  for (const auto &light : scene.lights) {

    SphereLightSample light_sample = sample_sphere_light(light, rec.point);

    if (light_sample.pdf <= 0.0) {
      continue;
    }
    double dist2 = light_sample.distance * light_sample.distance;

    Vec<3> light_dir = light_sample.direction;

    double n_dot_l = std::max(sample.shading_normal * light_dir, 0.0);

    if (n_dot_l <= 0.0) {
      continue;
    }
    Vec<3> light_to_surface = (rec.point - light_sample.point).normalized();
    double light_cos = std::max(light_sample.normal * light_to_surface, 0.0);
    if (light_cos <= 0.0) {
      continue;
    }

    Ray shadow_ray(rec.point + sample.shading_normal * config.ray_epsilon,
                   light_dir);
    HitRecord shadow_rec;
    if (!scene.hit(shadow_ray, config.ray_epsilon,
                   std::sqrt(dist2) - config.ray_epsilon, shadow_rec)) {
      continue;
    }

    if (!is_emissive(shadow_rec)) {
      continue;
    }

    Vec<3> emission = evaluate_emission(shadow_rec);

    double geometry = (n_dot_l * light_cos) / std::max(dist2, 1e-6);

    double light_pdf = light_sample.pdf;

    double bsdf_pdf = evaluate_lambertian_pdf(sample.shading_normal, light_dir);

    if (bsdf_pdf <= 0.0) {
      continue;
    }
    if (light_pdf <= 0.0) {
      continue;
    }
    double mis_weight = power_heuristic(light_pdf, bsdf_pdf);

    result += Vec<3>{sample.base_color[0] * emission[0],
                     sample.base_color[1] * emission[1],
                     sample.base_color[2] * emission[2]} *
              (geometry / light_pdf) * mis_weight;
  }
  return result;
}

Vec<3> evaluate_direct_lighting(const Scene &scene, const HitRecord &rec,
                                const DirectionalLight &light,
                                const TracerConfig &config) {

  Vec<3> direct{0.0, 0.0, 0.0};
  direct += evaluating_directional_direct_lighting(scene, rec, light, config);
  direct += evaluate_emissive_direct_lighting(scene, rec, config);

  return direct;
}

Vec<3> evaluate_emissive_hit_radiance(const Scene &scene, const HitRecord &rec,
                                      const Ray &incoming,
                                      bool from_delta_bounce,
                                      bool is_primary_ray,
                                      const Vec<3> &prev_shading_normal) {

  Vec<3> emission = evaluate_emission(rec);

  if (is_primary_ray || from_delta_bounce) {
    return emission;
  }

  if (!rec.material) {
    return emission;
  }

  double light_pdf = estimate_emissive_light_pdf(scene, rec);

  if (light_pdf <= 0.0) {
    return emission;
  }

  Vec<3> wi = (incoming.direction).normalized();
  if (prev_shading_normal * prev_shading_normal < 1e-12) {
    return emission;
  }

  double bsdf_pdf = evaluate_lambertian_pdf(prev_shading_normal, wi);

  if (bsdf_pdf <= 0.0) {
    return Vec<3>{0.0, 0.0, 0.0};
  }

  double mis_weight = power_heuristic(light_pdf, bsdf_pdf);
  return emission * mis_weight;
}

} // namespace

Vec<3> trace_ray(const Ray &ray, const Scene &scene, int depth,
                 const DirectionalLight &light, const TracerConfig &config,
                 bool from_delta_bounce, bool is_primary_ray,
                 const Vec<3> &prev_shading_normal) {

  if (depth <= 0) {
    return Vec<3>{0.0, 0.0, 0.0}; // 超过递归深度，返回黑色
  }
  HitRecord rec;
  if (scene.hit(ray, 0.001, std::numeric_limits<double>::max(), rec)) {

    if (is_emissive(rec)) {
      return evaluate_emissive_hit_radiance(
          scene, rec, ray, from_delta_bounce, is_primary_ray,
          prev_shading_normal); // 如果击中的是发光物体，直接返回其辐射度
    }
    Vec<3> direct{0.0, 0.0, 0.0};
    if (config.enable_direct_lighting) {
      direct = evaluate_direct_lighting(scene, rec, light, config);
    }

    BsdfSample bsdf = scatter_material(ray, rec, config);

    if (bsdf.valid) {
      // 在达到一定深度后，使用 Russian Roulette
      // 技术随机终止路径，以减少计算量 无偏估计
      if (depth <= config.max_depth - config.rr_start_depth) {
        double survive_prob =
            std::max(bsdf.attenuation[0],
                     std::max(bsdf.attenuation[1], bsdf.attenuation[2]));
        survive_prob = std::clamp(survive_prob, 0.10, 0.95);

        if (random_double() > survive_prob) {
          return Vec<3>{0.0, 0.0, 0.0}; // Russian Roulette 终止路径
        }

        bsdf.attenuation = bsdf.attenuation / survive_prob; // 反向补偿
      }
      Vec<3> outgoing_normal = rec.normal;
      if (rec.material && rec.material->type == MaterialType::Lambertian) {
        outgoing_normal = evaluate_material_sample(rec).shading_normal;
      }
      Vec<3> bounced = trace_ray(bsdf.scattered, scene, depth - 1, light,
                                 config, bsdf.is_delta, false, outgoing_normal);

      double bsdf_pdf = bsdf.pdf;
      if (bsdf_pdf <= 0.0) {
        return direct; // PDF 为零或负数，丢弃间接光贡献
      }

      Vec<3> indirect{bsdf.attenuation[0] * bounced[0],
                      bsdf.attenuation[1] * bounced[1],
                      bsdf.attenuation[2] * bounced[2]};

      if (!bsdf.is_delta && bsdf_pdf > 1e-8) {
        indirect = indirect / bsdf_pdf; // 反向补偿 PDF
      }

      return indirect + direct;
    }
    return direct;
  }

  return sky_color(ray); // 没有击中任何物体，返回背景色
}
