#pragma once

#include "../Vector.hh"
#include "ray.hpp"
#include <algorithm>

struct AABB {
  Vec<3> minimum;
  Vec<3> maximum;

  AABB(const Vec<3> &min, const Vec<3> &max) : minimum(min), maximum(max) {}

  bool hit(const Ray &ray, double t_min, double t_max) const {

    for (int axis = 0; axis < 3; axis++) {
      double inv_d = 1.0 / ray.direction[axis];
      double t0 = (minimum[axis] - ray.origin[axis]) * inv_d;
      double t1 = (maximum[axis] - ray.origin[axis]) * inv_d;

      if (inv_d < 0.0) {
        std::swap(t0, t1);
      }

      t_min = t0 > t_min ? t0 : t_min;
      t_max = t1 < t_max ? t1 : t_max;

      if (t_max <= t_min) {
        return false;
      }
    }
    return true;
  }
};
