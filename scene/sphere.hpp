#pragma once

#include "../Vector.hh"
#include "../render/hit_record.hpp"
#include "../render/material.hpp"
#include "../render/ray.hpp"
#include "hittable.hpp"

struct Sphere : public Hittable {
  Vec<3> center;
  double radius = 1.0;
  const Material *material = nullptr;

  bool hit(const Ray &ray, double t_min, double t_max,
           HitRecord &rec) const override;

  AABB bounding_box() const override;
};
