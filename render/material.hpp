#pragma once

#include "../Vector.hh"

enum class MaterialType {
  Lambertian,
  Metal,
  Dielectric,
};

struct Material {
  // 基础反射颜色

  MaterialType type = MaterialType::Lambertian;
  Vec<3> albedo{1.0, 1.0, 1.0};

  double metallic = 0.0;  // 金属度
  double roughness = 0.5; // 粗糙度
  double ior = 1.5;       // 折射率（仅对Dielectric有效）
};
