#pragma once

#include "../model.hpp"
#include "hittable.hpp"
#include <memory>
#include <vector>

struct Scene {
  std::vector<const Model *> models;
  std::vector<std::shared_ptr<Hittable>> objects;

  bool hit(const Ray &ray, double t_min, double t_max, HitRecord &rec) const;
};
