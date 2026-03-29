#include "tracer.hpp"
#include "hit_record.hpp"
#include "light.hpp"
#include "material.hpp"
#include "texture_mip.hpp"
#include "tracer_config.hpp"
#include "utils/render_util.hpp"
#include <algorithm>

namespace {

struct MaterialSample {
  Vec<3> albedo;
  Vec<3> shading_normal;
  double roughness;
};

Vec<3> evaluate_shading_normal(const HitRecord &rec) {
  if (!rec.material) {
    return rec.normal;
  }

  if (rec.material->use_normal_texture && rec.material->normal_texture) {

    Vec<4> sampled = rec.material->normal_texture->get_normal(rec.uv);

    Vec<3> tangent_normal{sampled[0], sampled[1], sampled[2]};

    tangent_normal = tangent_normal.normalized();

    Vec<3> world_normal =
        (rec.tangent * tangent_normal[0] + rec.bitangent * tangent_normal[1] +
         rec.normal * tangent_normal[2])
            .normalized();
    return world_normal;
  }

  return rec.normal;
}

double evaluate_roughness(const HitRecord &rec) {
  if (!rec.material) {
    return 0.0; // 默认粗糙度
  }

  double roughness = rec.material->roughness;

  if (rec.material->use_specular_texture && rec.material->specular_texture) {
    double spec = rec.material->specular_texture->specular(rec.uv);
    roughness = 1.0 - std::clamp(spec, 0.0, 1.0); // 将贴图值转换为粗糙度
  }
  return std::clamp(roughness, 0.0, 1.0);
}

Vec<3> evaluate_albedo(const HitRecord &rec) {
  if (!rec.material) {
    return Vec<3>{1.0, 1.0, 1.0};
  }
  if (rec.material->mip_chain && !rec.material->mip_chain->empty()) {
    TGAColor tex = sample_mip_bilinear(*rec.material->mip_chain, rec.uv, 0);

    return Vec<3>{tex[2] / 255.0, tex[1] / 255.0, tex[0] / 255.0};
  }

  if (rec.material->use_diffuse_texture && rec.material->diffuse_texture) {
    TGAColor tex = rec.material->diffuse_texture->diffuse(rec.uv);

    return Vec<3>{tex[2] / 255.0, tex[1] / 255.0, tex[0] / 255.0};
  }
  return rec.material->albedo;
}

MaterialSample evaluate_material_sample(const HitRecord &rec) {
  MaterialSample sample;
  sample.albedo = evaluate_albedo(rec);
  sample.shading_normal = evaluate_shading_normal(rec);
  sample.roughness = evaluate_roughness(rec);
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
  Vec<3> base_color = sample.albedo;

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
bool scatter_lambertian(const HitRecord &rec, Ray &scattered,
                        Vec<3> &attenuation, const TracerConfig &config) {

  MaterialSample sample = evaluate_material_sample(rec);
  Vec<3> scatter_dir = sample.shading_normal + random_unit_vector();

  if (scatter_dir * scatter_dir < 1e-8) {
    scatter_dir = sample.shading_normal;
  }

  scattered =
      Ray(rec.point + sample.shading_normal * config.ray_epsilon, scatter_dir);
  attenuation = sample.albedo;
  return true;
}

bool scatter_metal(const HitRecord &rec, const Ray &incoming, Ray &scattered,
                   Vec<3> &attenuation, const TracerConfig &config) {

  MaterialSample sample = evaluate_material_sample(rec);
  Vec<3> reflected =
      reflect(incoming.direction.normalized(), sample.shading_normal);
  double fuzz = sample.roughness;

  Vec<3> scattered_dir = reflected + random_in_unit_sphere() * fuzz;

  if (scattered_dir * sample.shading_normal <= 0) {
    return false; // 散射方向与法线相反，丢弃
  }

  scattered = Ray(rec.point + sample.shading_normal * config.ray_epsilon,
                  scattered_dir);
  attenuation = sample.albedo;
  return true;
}

bool scatter_dielectric(const HitRecord &rec, const Ray &incoming,
                        Ray &scattered, Vec<3> &attenuation,
                        const TracerConfig &config) {
  attenuation =
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

  scattered = Ray(rec.point + direction * config.ray_epsilon, direction);
  return true;
}

bool scatter_material(const Ray &incoming, const HitRecord &rec, Ray &scattered,
                      Vec<3> &attenuation, const TracerConfig &config) {

  if (!rec.material) {
    attenuation = Vec<3>{1.0, 1.0, 1.0};
    scattered = Ray(rec.point + rec.normal * config.ray_epsilon, rec.normal);
    return true;
  }

  switch (rec.material->type) {

  case MaterialType::Lambertian:
    return scatter_lambertian(rec, scattered, attenuation, config);

  case MaterialType::Metal:
    return scatter_metal(rec, incoming, scattered, attenuation, config);
  case MaterialType::Dielectric:
    return scatter_dielectric(rec, incoming, scattered, attenuation, config);
  }

  return false;
}
Vec<3> sample_point_on_sphere_light(const LightRecord &light) {
  return light.position + random_unit_vector() * light.radius;
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

    Vec<3> light_point = sample_point_on_sphere_light(light);
    Vec<3> to_light = light_point - rec.point;
    double dist2 = to_light * to_light;

    Vec<3> light_dir = to_light.normalized();

    double n_dot_l = std::max(sample.shading_normal * light_dir, 0.0);

    if (n_dot_l <= 0.0) {
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
    result +=
        Vec<3>{sample.albedo[0] * emission[0], sample.albedo[1] * emission[1],
               sample.albedo[2] * emission[2]} *
        (n_dot_l / std::max(dist2, 1e-6));
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

} // namespace

Vec<3> trace_ray(const Ray &ray, const Scene &scene, int depth,
                 const DirectionalLight &light, const TracerConfig &config) {

  if (depth <= 0) {
    return Vec<3>{0.0, 0.0, 0.0}; // 超过递归深度，返回黑色
  }
  HitRecord rec;
  if (scene.hit(ray, 0.001, std::numeric_limits<double>::max(), rec)) {

    if (is_emissive(rec)) {
      return evaluate_emission(rec); // 如果是自发光材质，直接返回发光颜色
    }
    Ray scattered{{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    Vec<3> attenuation{1.0, 1.0, 1.0};
    Vec<3> direct{0.0, 0.0, 0.0};
    if (config.enable_direct_lighting) {
      direct = evaluate_direct_lighting(scene, rec, light, config);
    }

    if (scatter_material(ray, rec, scattered, attenuation, config)) {
      // 在达到一定深度后，使用 Russian Roulette
      // 技术随机终止路径，以减少计算量 无偏估计
      if (depth <= config.max_depth - config.rr_start_depth) {
        double survive_prob =
            std::max(attenuation[0], std::max(attenuation[1], attenuation[2]));
        survive_prob = std::clamp(survive_prob, 0.10, 0.95);

        if (random_double() > survive_prob) {
          return Vec<3>{0.0, 0.0, 0.0}; // Russian Roulette 终止路径
        }

        attenuation = attenuation / survive_prob; // 反向补偿
      }
      Vec<3> bounced = trace_ray(scattered, scene, depth - 1, light, config);

      Vec<3> indirect{attenuation[0] * bounced[0], attenuation[1] * bounced[1],
                      attenuation[2] * bounced[2]};

      return indirect + direct;
    }
    return direct;
  }

  return sky_color(ray); // 没有击中任何物体，返回背景色
}
