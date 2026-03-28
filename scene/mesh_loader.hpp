#pragma once

#include "../model.hpp"
#include "hittable.hpp"
#include "triangle.hpp"

#include <memory>
#include <vector>

inline std::vector<std::shared_ptr<Hittable>>
build_triangle_primitives_from_model(const Model &model,
                                     const Material *material) {
  std::vector<std::shared_ptr<Hittable>> primitives;

  primitives.reserve(model.nfaces());
  for (int i = 0; i < model.nfaces(); i++) {
    auto tri = std::make_shared<TrianglePrimitive>();

    Vec<4> p0 = model.get_vertex(i, 0);
    Vec<4> p1 = model.get_vertex(i, 1);
    Vec<4> p2 = model.get_vertex(i, 2);

    Vec<3> offset{0.0, -0.5, -4.0};
    double scale = 0.8;

    tri->v0 = Vec<3>{p0[0], p0[1], p0[2]} * scale + offset;
    tri->v1 = Vec<3>{p1[0], p1[1], p1[2]} * scale + offset;
    tri->v2 = Vec<3>{p2[0], p2[1], p2[2]} * scale + offset;
    tri->material = material;
    primitives.push_back(tri);
  }
  return primitives;
}
