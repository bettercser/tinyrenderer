

#pragma once
#include "../shader.hpp"
#include "../../gl.hh"
#include "../../model.hpp"
struct SmoothShader : public IShader {
  const Model &model;
  TGAColor color = {};
  Vec<4> light_dir;
  Vec<3> tri[3];
  Vec<3> normal_tri[3];
  Matrix<4> MIT;
  Vec<3> l;

  SmoothShader(const Vec<3> light, const Model &m) : model(m) {
    Vec<4> ld = ModelView * Vec<4>{light[0], light[1], light[2], 0.0};
    light_dir = (ModelView * Vec<4>{ld[0], ld[1], ld[2], .0}).normalized();
    MIT = ModelView.inverted().transposed();
    l = Vec<3>{ld[0], ld[1], ld[2]}.normalized();
  }
  // 3D顶点坐标变换到齐次裁剪空间
  virtual Vec<4> vertex(int iface, int nthvert) {
    Vec<4> v = model.get_vertex(iface, nthvert);
    Vec<4> view_pos = ModelView * Vec<4>{v[0], v[1], v[2], 1.0};
    Vec<4> normal = model.get_normal(iface, nthvert);
    Vec<4> gl_Normal = MIT * Vec<4>{normal[0], normal[1], normal[2], 0.0};

    tri[nthvert] =
        Vec<3>{view_pos[0] , view_pos[1] ,
               view_pos[2] };
    normal_tri[nthvert] =
        Vec<3>{gl_Normal[0], gl_Normal[1], gl_Normal[2]}.normalized();

    Vec<4> clip_pos = Perspective * view_pos;
    return clip_pos;
  }

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const {
    TGAColor gl_PhongColor = {255, 255, 255, 255};

    Vec<3> normal = normal_tri[0] * bar[0] + normal_tri[1] * bar[1] +
                    normal_tri[2] * bar[2];
    normal = normal.normalized();

    Vec<3> r = (normal * (normal * l * 2.) - l).normalized();

    double ambient = .3;
    double diffuse = std::max(0.0, normal * l);

    double specular = std::pow(std::max(r[2], 0.0), 32);

    for (int i = 0; i < 3; i++) {
      gl_PhongColor[i] =
          255 * std::min(1., ambient + .4 * diffuse + .9 * specular);
    }

    return {false, gl_PhongColor};
  }
};
