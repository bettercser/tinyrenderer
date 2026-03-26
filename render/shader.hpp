#pragma once


#include "../Vector.hh"
#include "../matrix.hh"
#include "../tgaimage.h"
#include <utility>
#include "frame.hpp"
typedef Vec<4> Triangle[3];
struct IShader {

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const = 0;
  

  virtual ~IShader() = default;
  // 3D顶点坐标变换到齐次裁剪空间, 透视除法和视口变换在 光栅化阶段做
  virtual Vec<4> vertex(int iface, int nthvert) = 0;
  RenderContext* renderContext = nullptr;
};