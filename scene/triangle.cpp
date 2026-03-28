
#include "triangle.hpp"

bool TrianglePrimitive::hit(const Ray &ray, double t_min, double t_max,
                            HitRecord &rec) const {
  const double EPS = 1e-8;

  Vec<3> edge1 = v1 - v0;
  Vec<3> edge2 = v2 - v0;

  Vec<3> h = ray.direction.cross(edge2);

  double a = edge1 * h;

  if (std::abs(a) < EPS) {
    return false; // 光线与三角形平行
  }

  double f = 1.0 / a;
  Vec<3> s = ray.origin - v0;
  double u = f * (s * h);

  if (u < 0.0 || u > 1.0) {
    return false; // 交点在三角形外部
  }

  Vec<3> q = s.cross(edge1);
  double v = f * (ray.direction * q);

  if (v < 0.0 || u + v > 1.0) {
    return false; // 交点在三角形外部
  }

  double t_hit = f * (edge2 * q);

  if (t_hit < t_min || t_hit > t_max) {
    return false; // 交点不在有效范围内
  }

  rec.t = t_hit;
  rec.point = ray.origin + ray.direction * rec.t;
  Vec<3> outward_normal = edge1.cross(edge2).normalized();
  rec.set_front_face(ray, outward_normal);
  rec.material = material;
  return true;
}

AABB TrianglePrimitive::bounding_box() const {
  constexpr double eps = 1e-4;

  Vec<3> min_v{
      std::min({v0[0], v1[0], v2[0]}) - eps,
      std::min({v0[1], v1[1], v2[1]}) - eps,
      std::min({v0[2], v1[2], v2[2]}) - eps,
  };

  Vec<3> max_v{
      std::max({v0[0], v1[0], v2[0]}) + eps,
      std::max({v0[1], v1[1], v2[1]}) + eps,
      std::max({v0[2], v1[2], v2[2]}) + eps,
  };

  return AABB{min_v, max_v};
}
