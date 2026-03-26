
#pragma once
#include "../shader.hpp"
#include "../../gl.hh"
#include "../../model.hpp"


struct RandomShader : public IShader {
  const Model &model;
  TGAColor color = {};
  Vec<3> tri[3];

  RandomShader(const Model &m) : model(m) {}
  // 3D顶点坐标变换到齐次裁剪空间
  virtual Vec<4> vertex(int iface, int nthvert) {
    Vec<4> v = model.get_vertex(iface, nthvert);
    Vec<4> view_pos = ModelView * Vec<4>{v[0], v[1], v[2], 1.0};

    tri[nthvert] =
        Vec<3>{view_pos[0] , view_pos[1] ,
               view_pos[2] };
    Vec<4> clip_pos = Perspective * view_pos;
    return clip_pos ;
  }

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const {
    return {false, color};
  }
};