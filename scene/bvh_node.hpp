#pragma once

#include "hittable.hpp"
#include <memory>
#include <vector>

struct BVHNode : public Hittable {
  std::shared_ptr<Hittable> left;
  std::shared_ptr<Hittable> right;

  AABB box;

  BVHNode(std::vector<std::shared_ptr<Hittable>> &objects, size_t start,
          size_t end);

  bool hit(const Ray &ray, double t_min, double t_max,
           HitRecord &rec) const override;

  AABB bounding_box() const override;
};
