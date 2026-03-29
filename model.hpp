#pragma once

#include <memory>
#include <vector>

#include "Vector.hh"
#include "tgaimage.h"

class Model {

private:
  std::vector<Vec<4>> verts = {};
  std::vector<Vec<4>> norms = {};

  std::vector<Vec<2>> uvs = {};

  std::vector<int> face_vertices = {};
  std::vector<int> face_uvs = {};
  std::vector<int> face_normals = {};

  std::shared_ptr<TGAImage> normal_map = std::make_shared<TGAImage>();
  std::shared_ptr<TGAImage> diffuse_map = std::make_shared<TGAImage>();
  std::shared_ptr<TGAImage> specular_map = std::make_shared<TGAImage>();

public:
  Model(const char *filename);
  ~Model() {

  };

  int nfaces() const { return face_vertices.size() / 3; }
  int nverts() const { return verts.size(); }
  int nnorms() const { return norms.size(); }

  Vec<4> get_vertex(int i) const { return verts[i]; }
  Vec<4> get_vertex(int face_idx, int vert_idx) const {
    int idx = face_vertices[face_idx * 3 + vert_idx];
    return get_vertex(idx);
  };

  Vec<4> get_normal(int i) const { return norms[i]; }
  Vec<4> get_normal(const Vec<2> &uv) const {
    if (!normal_map) {
      return Vec<4>{0, 0, 0, 0};
    }
    TGAColor c = normal_map->get(uv[0] * normal_map->width(),
                                 uv[1] * normal_map->height());

    return Vec<4>{static_cast<double>(c[2]), static_cast<double>(c[1]),
                  static_cast<double>(c[0])} *
               2. / 255. -
           Vec<4>{1., 1., 1., 0.};
  }
  Vec<4> get_normal(int face_idx, int vert_idx) const {
    int idx = face_normals[face_idx * 3 + vert_idx];
    return get_normal(idx);
  };

  Vec<2> get_uv(const int face_idx, const int vert_idx) const {
    int idx = face_uvs[face_idx * 3 + vert_idx];
    return uvs[idx];
  }

  double specular(const Vec<2> &uv) const {
    Vec<2> p = {uv[0] * specular_map->width(), uv[1] * specular_map->height()};
    TGAColor c =
        specular_map->get(static_cast<int>(p[0]), static_cast<int>(p[1]));
    return static_cast<double>(c[0]) / 255.;
  }

  TGAColor diffuse(const Vec<2> &uv) const {
    Vec<2> p = {uv[0] * diffuse_map->width(), uv[1] * diffuse_map->height()};
    TGAColor c =
        diffuse_map->get(static_cast<int>(p[0]), static_cast<int>(p[1]));
    return c;
  }
  const TGAImage *diffuse_image() const { return diffuse_map.get(); }
  const TGAImage *normal_image() const { return normal_map.get(); }
  const TGAImage *specular_image() const { return specular_map.get(); }
};
