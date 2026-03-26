#include "gl.hh"
#include <algorithm>
Matrix<4> ModelView, Viewport, Perspective;
Matrix<4> LightView, LightPerspective, LightViewport, LightMVP;

std::vector<double> ZBuffer;

void lookat(const Vec<3> &eye, const Vec<3> &center, const Vec<3> &up) {
  Vec<3> n = (eye - center).normalized();
  Vec<3> l = up.cross(n).normalized();
  Vec<3> m = n.cross(l).normalized();
  ModelView = Matrix<4>{{{l[0], l[1], l[2], 0},
                         {m[0], m[1], m[2], 0},
                         {n[0], n[1], n[2], 0},
                         {0, 0, 0, 1}}} *
              Matrix<4>{{{1, 0, 0, -center[0]},
                         {0, 1, 0, -center[1]},
                         {0, 0, 1, -center[2]},
                         {0, 0, 0, 1}}};
}
void lookat_shadow(const Vec<3> &eye, const Vec<3> &center, const Vec<3> &up) {
  Vec<3> n = (eye - center).normalized();
  Vec<3> l = up.cross(n).normalized();
  Vec<3> m = n.cross(l).normalized();
  LightView = Matrix<4>{{{l[0], l[1], l[2], 0},
                         {m[0], m[1], m[2], 0},
                         {n[0], n[1], n[2], 0},
                         {0, 0, 0, 1}}} *
              Matrix<4>{{{1, 0, 0, -center[0]},
                         {0, 1, 0, -center[1]},
                         {0, 0, 1, -center[2]},
                         {0, 0, 0, 1}}};
}

void init_light_perspective(const double f, const double near,
                            const double far) {

  LightPerspective =
      Matrix<4>{{{1.0 / f, 0, 0, 0},
                 {0, 1.0 / f, 0, 0},
                 {0, 0, 2.0 / (near - far), (near + far) / (near - far)},
                 {0, 0, 0, 1}}};
}

void init_perspective(const double f) {
  Perspective =
      Matrix<4>{{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, -1 / f, 1}}};
}

void init_viewport(int x, int y, int w, int h) {
  Viewport = Matrix<4>{{{w / 2., 0, 0, x + w / 2.},
                        {0, h / 2., 0, y + h / 2.},
                        {0, 0, 1., 0},
                        {0, 0, 0, 1}}};
}



void init_zbuffer(const int width, const int height) {
  ZBuffer = std::vector<double>(width * height,
                                -std::numeric_limits<double>::infinity());
}


