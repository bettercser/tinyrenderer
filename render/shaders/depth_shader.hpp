#pragma once



#include "../shader.hpp"
#include "../../gl.hh"
#include "../../model.hpp"
struct DepthShader : public IShader {
  const Model &model;

  // LightMVP_Viewport 矩阵是它唯一的输入
  Matrix<4> LightMVP_Viewport;

  // 假设 IShader 已经定义了 model 成员，如果 IShader 没有，需要添加
  DepthShader(const Model &m, const Matrix<4> &light_mvp_vp)
      : model(m), LightMVP_Viewport(light_mvp_vp) {}

  // 核心函数：顶点着色器
  virtual Vec<4> vertex(int iface, int nthvert) {
    Vec<4> v = model.get_vertex(iface, nthvert);

    // 关键：将顶点转换到光源的屏幕空间坐标
    return LightMVP_Viewport * v;
  }

  // 片元着色器
  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const {
    // 不需要任何计算，因为深度值在 rasterize 函数里直接从 tri[i][2] 插值得到
    // 只需要返回 false，表示不丢弃这个像素
    return {false, TGAColor{}};
  }
};
