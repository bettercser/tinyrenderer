#pragma once

#ifndef GL_HH
#define GL_HH
#include "Vector.hh"
#include "matrix.hh"
#include "tgaimage.h"
#include "render/shader.hpp"

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

#endif // GL_HH
