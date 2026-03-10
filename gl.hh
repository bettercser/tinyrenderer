#pragma once

#ifndef GL_HH
#define GL_HH
#include "Vector.hh"
#include "matrix.hh"
#include "tgaimage.h"

extern Matrix<4> ModelView, Viewport, Perspective;
extern Matrix<4> LightView, LightViewport, LightPerspective, LightMVP;
extern std::vector<double> ZBuffer;
void lookat(const Vec<3> &eye, const Vec<3> &center, const Vec<3> &up);

void lookat_shadow(const Vec<3> &eye, const Vec<3> &center, const Vec<3> &up);
void init_light_perspective(const double R, const double near,
                            const double far);
void init_perspective(const double f);

void init_viewport(int x, int y, int w, int h);

void init_zbuffer(const int width, const int height);

struct IShader {

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const = 0;
  double varying_w_recip[3];
  Matrix<3> TBN;
};

typedef Vec<4> Triangle[3];
Vec<3> barycentric(const Triangle &tri, const Vec<3> &P);

void rasterize(const Triangle &tri, IShader &shader, TGAImage &framebuffer);
#endif // GL_HH
