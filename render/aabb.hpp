#pragma once

#include "../Vector.hh"
#include "ray.hpp"
#include <algorithm>

struct AABB {
  Vec<3> minimum;
  Vec<3> maximum;

  AABB() = default;

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

inline AABB surrounding_box(const AABB &a, const AABB &b) {
  Vec<3> small{
      std::min(a.minimum[0], b.minimum[0]),
      std::min(a.minimum[1], b.minimum[1]),
      std::min(a.minimum[2], b.minimum[2]),
  };

  Vec<3> big{
      std::max(a.maximum[0], b.maximum[0]),
      std::max(a.maximum[1], b.maximum[1]),
      std::max(a.maximum[2], b.maximum[2]),
  };

  return AABB{small, big};
}
