#include "tracer.hpp"

namespace {
// 定义辅助函数 处理 阴影
// 从被击中的点出发朝光源发射一条射线，检查是否有物体阻挡了光线
// 为了防止自遮挡需要加一点偏移

bool is_in_shadow(const Scene &scene, const Vec<3> &point,
                  const Vec<3> &light_dir, const Vec<3> &normal) {
  Ray shadow_ray(point + normal * 1e-4, light_dir);
  HitRecord shadow_rec;

  return scene.hit(shadow_ray, 0.001, std::numeric_limits<double>::max(),
                   shadow_rec);
}

double random_double() { return rand() / (RAND_MAX + 1.0); }

Vec<3> random_in_unit_sphere() {
  Vec<3> p;
  do {
    p = Vec<3>{random_double(), random_double(), random_double()} * 2.0 -
        Vec<3>{1.0, 1.0, 1.0};
  } while (p * p >= 1.0);
  return p;
}

Vec<3> random_unit_vector() { return random_in_unit_sphere().normalized(); }
} // namespace

Vec<3> trace_ray(const Ray &ray, const Scene &scene, int depth) {

  if (depth <= 0) {
    return Vec<3>{0.0, 0.0, 0.0}; // 超过递归深度，返回黑色
  }
  HitRecord rec;
  if (scene.hit(ray, 0.001, std::numeric_limits<double>::max(), rec)) {
    Vec<3> scatter_dir = rec.normal + random_unit_vector();

    if (scatter_dir * scatter_dir < 1e-8) {
      scatter_dir = rec.normal; // 避免散射方向过于接近零向量
    }

    Ray scattered(rec.point, scatter_dir);
    Vec<3> attenuation =
        rec.material ? rec.material->albedo : Vec<3>{1.0, 1.0, 1.0};

    Vec<3> bounced = trace_ray(scattered, scene, depth - 1);

    return Vec<3>{attenuation[0] * bounced[0], attenuation[1] * bounced[1],
                  attenuation[2] * bounced[2]};
  }
  Vec<3> unit_dir = ray.direction.normalized();
  double t = 0.5 * (unit_dir[1] + 1.0);
  return Vec<3>{1.0, 1.0, 1.0} * (1.0 - t) +
         Vec<3>{0.5, 0.7, 1.0} * t; // 线性插值背景色
}
