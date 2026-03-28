
#pragma once

#include "../Vector.hh"
#include "../render/material.hpp"
#include "hittable.hpp"

struct TrianglePrimitive : Hittable {
  Vec<3> v0, v1, v2;
  Vec<3> n0, n1, n2;
  Vec<2> uv0, uv1, uv2;

  const Material *material = nullptr;

  bool hit(const Ray &ray, double t_min, double t_max,
           HitRecord &rec) const override;

  AABB bounding_box() const override;
};
