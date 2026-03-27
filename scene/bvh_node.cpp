#include "bvh_node.hpp"
#include "hittable.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <memory>

namespace {
bool box_compare(const std::shared_ptr<Hittable> &a,
                 const std::shared_ptr<Hittable> &b, int axis) {
  AABB box_a = a->bounding_box();
  AABB box_b = b->bounding_box();
  return box_a.minimum[axis] < box_b.minimum[axis];
}

bool box_x_compare(const std::shared_ptr<Hittable> &a,
                   const std::shared_ptr<Hittable> &b) {
  return box_compare(a, b, 0);
}

bool box_y_compare(const std::shared_ptr<Hittable> &a,
                   const std::shared_ptr<Hittable> &b) {
  return box_compare(a, b, 1);
}
bool box_z_compare(const std::shared_ptr<Hittable> &a,
                   const std::shared_ptr<Hittable> &b) {
  return box_compare(a, b, 2);
}
} // namespace

BVHNode::BVHNode(std::vector<std::shared_ptr<Hittable>> &objects, size_t start,
                 size_t end) {
  int axis = std::rand() % 3;
  auto comparator = (axis == 0)   ? box_x_compare
                    : (axis == 1) ? box_y_compare
                                  : box_z_compare;
  size_t object_span = end - start;

  if (object_span == 1) {
    left = right = objects[start];
  } else if (object_span == 2) {
    if (comparator(objects[start], objects[start + 1])) {
      left = objects[start];
      right = objects[start + 1];
    } else {
      left = objects[start + 1];
      right = objects[start];
    }
  } else {
    std::sort(objects.begin() + start, objects.begin() + end, comparator);

    size_t mid = start + object_span / 2;
    left = std::make_shared<BVHNode>(objects, start, mid);
    right = std::make_shared<BVHNode>(objects, mid, end);
  }

  box = surrounding_box(left->bounding_box(), right->bounding_box());
}

AABB BVHNode::bounding_box() const { return box; }

bool BVHNode::hit(const Ray &ray, double t_min, double t_max,
                  HitRecord &rec) const {

  if (!box.hit(ray, t_min, t_max)) {
    return false;
  }

  HitRecord left_rec, right_rec;

  bool hit_left = left->hit(ray, t_min, t_max, left_rec);

  bool hit_right =
      right->hit(ray, t_min, hit_left ? left_rec.t : t_max, right_rec);

  if (hit_left && hit_right) {
    rec = (left_rec.t < right_rec.t) ? left_rec : right_rec;
    return true;
  } else if (hit_left) {
    rec = left_rec;
    return true;
  } else if (hit_right) {
    rec = right_rec;
    return true;
  }
  return false;
}
