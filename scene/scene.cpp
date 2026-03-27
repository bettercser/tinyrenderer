#include "scene.hpp"

bool Scene::hit(const Ray &ray, double t_min, double t_max,
                HitRecord &rec) const {

  if (accel) {
    return accel->hit(ray, t_min, t_max, rec);
  }
  HitRecord temp_rec;
  bool hit_anything = false;
  double closest_so_far = t_max;

  for (const auto &object : objects) {
    if (object->hit(ray, t_min, closest_so_far, temp_rec)) {
      hit_anything = true;
      closest_so_far = temp_rec.t;
      rec = temp_rec;
    }
  }
  return hit_anything;
}
