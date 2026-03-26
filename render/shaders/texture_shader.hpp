

#pragma once
#include "../shader.hpp"
#include "../../gl.hh"
#include "../../model.hpp"
struct TextureShader : public IShader {
  const Model &model;
  TGAColor color = {};
  Vec<3> light_input;
  Vec<3> light_dir;
  Vec<2> varying_uv[3];

  Matrix<4> MIT;

  TextureShader(const Vec<3> light, const Model &m) : model(m), light_input(light) {}
    
  // 3D顶点坐标变换到齐次裁剪空间
  virtual Vec<4> vertex(int iface, int nthvert) {
    Vec<4> ld =
        renderContext->ModelView * Vec<4>{light_input[0], light_input[1], light_input[2], 0.0}.normalized();
    light_dir = {ld[0], ld[1], ld[2]};
    MIT = renderContext->ModelView.inverted().transposed();
    Vec<4> v = model.get_vertex(iface, nthvert);
    Vec<4> gl_Position = renderContext->ModelView * Vec<4>{v[0], v[1], v[2], 1.0};

    varying_uv[nthvert] = model.get_uv(iface, nthvert);

    gl_Position = renderContext->Projection * gl_Position;
    renderContext->varying_w_recip[nthvert] = 1.0 / gl_Position[3];
    return gl_Position;
  }

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const {

    Vec<2> uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] +
                varying_uv[2] * bar[2];
    TGAColor gl_PhongColor = model.diffuse(uv);

    Vec<4> n_sample = model.get_normal(uv);
    Vec<3> n_tangent = Vec<3>{n_sample[0], n_sample[1], n_sample[2]};
    Vec<3> normal = (renderContext->TBN * n_tangent).normalized();

    Vec<3> r = (normal * (normal * light_dir * 2.) - light_dir).normalized();

    double ambient = .3;
    double diffuse = std::max(0.0, normal * light_dir);

    double spec_mask = model.specular(uv);

    double specular = std::pow(std::max(r[2], 0.0), 50);
    double spec_final = specular * spec_mask;

    for (int i = 0; i < 3; i++) {

      double res =
          (ambient + 0.8 * diffuse) * gl_PhongColor[i] + 0.6 * spec_final * 255;
      gl_PhongColor[i] = std::min(255.0, res);
    }

    return {false, gl_PhongColor};
  }
};