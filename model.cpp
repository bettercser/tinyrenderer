#include "model.hpp"
#include "tgaimage.h"
#include <fstream>
#include <sstream>

Model::Model(const char *filename) {
  std::ifstream in(filename);

  if (!in.is_open()) {
    throw std::runtime_error("Cannot open file");
  }

  std::string line;
  while (std::getline(in, line)) {
    std::istringstream iss(line);

    std::string type;

    iss >> type;

    if (type == "v") {
      Vec<4> v;
      iss >> v[0] >> v[1] >> v[2];
      v[3] = 1.0;
      verts.push_back(v);
    } else if (type == "vn") {
      Vec<4> n;
      iss >> n[0] >> n[1] >> n[2];
      n[3] = 0.0;
      norms.push_back(n);
    } else if (type == "vt") {
      Vec<2> uv;
      iss >> uv[0] >> uv[1];
      uvs.push_back({uv[0], 1. - uv[1]}); // flip V coordinate
    } else if (type == "f") {
      std::string vertex_str;
      while (iss >> vertex_str) {
        std::istringstream viss(vertex_str);
        std::string index_str;

        std::getline(viss, index_str, '/');
        int vertex_index = std::stoi(index_str);
        face_vertices.push_back(vertex_index - 1);

        std::getline(viss, index_str, '/'); // skip texture coordinate
        int uv_index = std::stoi(index_str);
        face_uvs.push_back(uv_index - 1);

        std::getline(viss, index_str, '/');
        int normal_index = std::stoi(index_str);
        face_normals.push_back(normal_index - 1);
      }
    }
  }
  in.close();

  auto load_texture = [&filename](const std::string &suffix, TGAImage &image) {
    std::string texfile = std::string(filename);
    size_t dot_pos = texfile.find_last_of('.');
    if (dot_pos != std::string::npos) {
      texfile = texfile.substr(0, dot_pos) + suffix;
      if (!image.read_tga_file(texfile.c_str())) {
        throw std::runtime_error("Cannot load texture file: " + texfile);
      }
    }
  };
  load_texture("_nm_tangent.tga", *normal_map);
  load_texture("_diffuse.tga", *diffuse_map);
  load_texture("_spec.tga", *specular_map);
}
