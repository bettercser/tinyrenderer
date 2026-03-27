#pragma once

#include "../Vector.hh"
#include "material.hpp"
#include "ray.hpp"

struct HitRecord {
  double t = 0.0;
  Vec<3> point;
  Vec<3> normal;
  Vec<2> uv;

  const Material *material = nullptr;
  bool front_face = true;

  void set_front_face(const Ray &ray, const Vec<3> &outward_normal) {
    front_face = (ray.direction * outward_normal) < 0.0;
    normal = front_face ? outward_normal : outward_normal * -1.0;
  }
};
