#pragma once

#include "../Vector.hh"
#include "../model.hpp"
#include "texture_mip.hpp"

enum class MaterialType {
  Lambertian,
  Metal,
  Dielectric,
};

struct Material {
  // 基础反射颜色

  MaterialType type = MaterialType::Lambertian;
  Vec<3> base_color{1.0, 1.0, 1.0};
  Vec<3> emission{0.0, 0.0, 0.0};           // 自发光颜色
  Vec<3> transmission_color{1.0, 1.0, 1.0}; // 透射颜色（仅对Dielectric有效）

  double metallic = 0.0;                  // 金属度
  double roughness = 0.5;                 // 粗糙度
  double ior = 1.5;                       // 折射率（仅对Dielectric有效）
  const Model *diffuse_texture = nullptr; // 漫反射贴图
  bool use_diffuse_texture = false;       // 是否使用漫反射贴图

  const Model *normal_texture = nullptr; // 法线贴图
  bool use_normal_texture = false;       // 是否使用法线贴图

  const Model *specular_texture = nullptr;    // 镜面反射贴图
  bool use_specular_texture = false;          // 是否使用镜面反射贴图
  const TextureMipChain *mip_chain = nullptr; // 纹理MIP链
};
