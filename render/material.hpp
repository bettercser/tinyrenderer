#pragma once

#include "../Vector.hh"
#include "../model.hpp"

enum class MaterialType {
  Lambertian,
  Metal,
  Dielectric,
};

struct Material {
  // 基础反射颜色

  MaterialType type = MaterialType::Lambertian;
  Vec<3> albedo{1.0, 1.0, 1.0};

  double metallic = 0.0;                  // 金属度
  double roughness = 0.5;                 // 粗糙度
  double ior = 1.5;                       // 折射率（仅对Dielectric有效）
  const Model *diffuse_texture = nullptr; // 漫反射贴图
  bool use_diffuse_texture = false;       // 是否使用漫反射贴图
};
