#include "tracer.hpp"
#include "hit_record.hpp"
#include "material.hpp"

namespace {
// 定义辅助函数 处理 阴影
// 从被击中的点出发朝光源发射一条射线，检查是否有物体阻挡了光线
// 为了防止自遮挡需要加一点偏移

double random_double() { return rand() / (RAND_MAX + 1.0); }

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
                        Vec<3> &attenuation) {

  Vec<3> scatter_dir = rec.normal + random_unit_vector();

  if (scatter_dir * scatter_dir < 1e-8) {
    scatter_dir = rec.normal;
  }

  scattered = Ray(rec.point + rec.normal * 1e-4, scatter_dir);
  attenuation = rec.material ? rec.material->albedo : Vec<3>{1.0, 1.0, 1.0};
  return true;
}

bool scatter_metal(const HitRecord &rec, const Ray &incoming, Ray &scattered,
                   Vec<3> &attenuation) {
  Vec<3> reflected = reflect(incoming.direction.normalized(), rec.normal);
  double fuzz =
      rec.material ? std::clamp(rec.material->roughness, 0.0, 1.0) : 0.0;

  Vec<3> scattered_dir = reflected + random_in_unit_sphere() * fuzz;

  if (scattered_dir * rec.normal <= 0) {
    return false; // 散射方向与法线相反，丢弃
  }

  scattered = Ray(rec.point + rec.normal * 1e-4, scattered_dir);
  attenuation = rec.material ? rec.material->albedo : Vec<3>{1.0, 1.0, 1.0};
  return true;
}

bool scatter_dielectric(const HitRecord &rec, const Ray &incoming,
                        Ray &scattered, Vec<3> &attenuation) {
  attenuation = Vec<3>{1.0, 1.0, 1.0};

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

  scattered = Ray(rec.point + direction * 1e-4, direction);
  return true;
}

bool scatter_material(const Ray &incoming, const HitRecord &rec, Ray &scattered,
                      Vec<3> &attenuation) {

  if (!rec.material) {
    attenuation = Vec<3>{1.0, 1.0, 1.0};
    scattered = Ray(rec.point + rec.normal * 1e-4, rec.normal);
    return true;
  }

  switch (rec.material->type) {

  case MaterialType::Lambertian:
    return scatter_lambertian(rec, scattered, attenuation);

  case MaterialType::Metal:
    return scatter_metal(rec, incoming, scattered, attenuation);
  case MaterialType::Dielectric:
    return scatter_dielectric(rec, incoming, scattered, attenuation);
  }

  return false;
}

} // namespace

Vec<3> trace_ray(const Ray &ray, const Scene &scene, int depth) {

  if (depth <= 0) {
    return Vec<3>{0.0, 0.0, 0.0}; // 超过递归深度，返回黑色
  }
  HitRecord rec;
  if (scene.hit(ray, 0.001, std::numeric_limits<double>::max(), rec)) {
    Ray scattered{{0.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    Vec<3> attenuation{1.0, 1.0, 1.0};

    if (scatter_material(ray, rec, scattered, attenuation)) {
      Vec<3> bounced = trace_ray(scattered, scene, depth - 1);

      return Vec<3>{attenuation[0] * bounced[0], attenuation[1] * bounced[1],
                    attenuation[2] * bounced[2]};
    }
    return Vec<3>{0.0, 0.0, 0.0}; // 材质散射失败，返回黑色
  }
  Vec<3> unit_dir = ray.direction.normalized();
  double t = 0.5 * (unit_dir[1] + 1.0);
  return Vec<3>{1.0, 1.0, 1.0} * (1.0 - t) +
         Vec<3>{0.5, 0.7, 1.0} * t; // 线性插值背景色
}
