#pragma once

#include "../render/aabb.hpp"
#include "../render/hit_record.hpp"
#include "../render/ray.hpp"

struct Hittable {
  virtual ~Hittable() = default;
  virtual bool hit(const Ray &ray, double t_min, double t_max,
                   HitRecord &rec) const = 0;
  virtual AABB bounding_box() const = 0;
};
