#include "Vector.hh"
#include "gl.hh"
#include "matrix.hh"
#include "model.hpp"
#include "tgaimage.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
constexpr int width = 1024;
constexpr int height = 1024;

constexpr double PI = 3.14159265358979323846;

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};
TGAImage shadowmap(width, height, TGAImage::GRAYSCALE);

struct RandomShader : public IShader {
  const Model &model;
  TGAColor color = {};
  Vec<3> tri[3];

  RandomShader(const Model &m) : model(m) {}
  // 3D顶点坐标变换到齐次裁剪空间
  virtual Vec<4> vertex(int iface, int nthvert) {
    Vec<4> v = model.get_vertex(iface, nthvert);
    Vec<4> gl_Position = ModelView * Vec<4>{v[0], v[1], v[2], 1.0};
    tri[nthvert] =
        Vec<3>{gl_Position[0] / gl_Position[3], gl_Position[1] / gl_Position[3],
               gl_Position[2] / gl_Position[3]};
    gl_Position = Perspective * gl_Position;
    gl_Position = gl_Position / gl_Position[3];
    return Viewport * gl_Position;
  }

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const {
    return {false, color};
  }
};

struct PhongShader : public IShader {
  const Model &model;
  TGAColor color = {};
  Vec<3> light_dir;
  Vec<3> tri[3];

  PhongShader(const Vec<3> light, const Model &m) : model(m) {
    Vec<4> ld = ModelView * Vec<4>{light[0], light[1], light[2], 0.0};
    light_dir = Vec<3>{ld[0], ld[1], ld[2]}.normalized();
  }
  // 3D顶点坐标变换到齐次裁剪空间
  virtual Vec<4> vertex(int iface, int nthvert) {

    Vec<4> v = model.get_vertex(iface, nthvert);
    Vec<4> gl_Position = ModelView * Vec<4>{v[0], v[1], v[2], 1.0};

    tri[nthvert] =
        Vec<3>{gl_Position[0] / gl_Position[3], gl_Position[1] / gl_Position[3],
               gl_Position[2] / gl_Position[3]};
    gl_Position = Perspective * gl_Position;
    gl_Position = gl_Position / gl_Position[3];
    return Viewport * gl_Position;
  }

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const {
    TGAColor gl_PhongColor = {255, 255, 255, 255};

    Vec<3> normal = (tri[1] - tri[0]).cross(tri[2] - tri[0]).normalized();

    Vec<3> r = (normal * (normal * light_dir * 2.) - light_dir).normalized();

    double ambient = .3;
    double diffuse = std::max(0.0, normal * light_dir);

    double specular = std::pow(std::max(r[2], 0.0), 32);

    for (int i = 0; i < 3; i++) {
      gl_PhongColor[i] =
          255 * std::min(1., ambient + .4 * diffuse + .9 * specular);
    }

    return {false, gl_PhongColor};
  }
};
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
    Vec<4> gl_Position = ModelView * Vec<4>{v[0], v[1], v[2], 1.0};
    Vec<4> normal = model.get_normal(iface, nthvert);
    Vec<4> gl_Normal = MIT * Vec<4>{normal[0], normal[1], normal[2], 0.0};

    tri[nthvert] =
        Vec<3>{gl_Position[0] / gl_Position[3], gl_Position[1] / gl_Position[3],
               gl_Position[2] / gl_Position[3]};
    normal_tri[nthvert] =
        Vec<3>{gl_Normal[0], gl_Normal[1], gl_Normal[2]}.normalized();

    gl_Position = Perspective * gl_Position;
    gl_Position = gl_Position / gl_Position[3];
    return Viewport * gl_Position;
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

struct TextureShader : public IShader {
  const Model &model;
  TGAColor color = {};
  Vec<3> light_dir;
  Vec<2> varying_uv[3];

  Matrix<4> MIT;

  TextureShader(const Vec<3> light, const Model &m) : model(m) {
    Vec<4> ld =
        ModelView * Vec<4>{light[0], light[1], light[2], 0.0}.normalized();
    light_dir = {ld[0], ld[1], ld[2]};
    MIT = ModelView.inverted().transposed();
  }
  // 3D顶点坐标变换到齐次裁剪空间
  virtual Vec<4> vertex(int iface, int nthvert) {
    Vec<4> v = model.get_vertex(iface, nthvert);
    Vec<4> gl_Position = ModelView * Vec<4>{v[0], v[1], v[2], 1.0};

    varying_uv[nthvert] = model.get_uv(iface, nthvert);

    gl_Position = Perspective * gl_Position;
    varying_w_recip[nthvert] = 1.0 / gl_Position[3];
    return gl_Position;
  }

  virtual std::pair<bool, TGAColor> fragment(const Vec<3> &bar) const {

    Vec<2> uv = varying_uv[0] * bar[0] + varying_uv[1] * bar[1] +
                varying_uv[2] * bar[2];
    TGAColor gl_PhongColor = model.diffuse(uv);

    Vec<4> n_sample = model.get_normal(uv);
    Vec<3> n_tangent = Vec<3>{n_sample[0], n_sample[1], n_sample[2]};
    Vec<3> normal = (TBN * n_tangent).normalized();

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

Matrix<4> rot(double angle) {
  double c = std::cos(angle * PI / 180.);
  double s = std::sin(angle * PI / 180.);

  Matrix<4> r = {{{c, 0, s, 0}, {0, 1, 0, 0}, {-s, 0, c, 0}, {0, 0, 0, 1}}};
  return r;
}

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer,
          TGAColor color) {
  // 通过在更平的一面上计算
  bool steep = std::abs(ax - bx) < std::abs(ay - by);

  if (steep) {
    std::swap(ax, ay);
    std::swap(bx, by);
  }

  if (ax > bx) {
    std::swap(ax, bx);
    std::swap(ay, by);
  }

  int y = ay;
  int ierror = 0;
  for (int x = ax; x < bx; x++) {

    if (steep) {
      framebuffer.set(y, x, color);
    } else {
      framebuffer.set(x, y, color);
    }
    ierror += 2 * std::abs(by - ay);

    if (ierror > bx - ax) {
      y += by > ay ? 1 : -1;
      ierror -= 2 * std::abs(bx - ax);
    }
  }
}

Matrix<3> compute_TBN(const Triangle &tri, const Vec<2> uv[3],
                      const Vec<3> normals) {

  Vec<4> temp_1 = tri[1] - tri[0];
  Vec<4> temp_2 = tri[2] - tri[0];
  Vec<3> E_1 = Vec<3>{temp_1[0], temp_1[1], temp_1[2]};
  Vec<3> E_2 = Vec<3>{temp_2[0], temp_2[1], temp_2[2]};
  Vec<2> UV_1 = uv[1] - uv[0];
  Vec<2> UV_2 = uv[2] - uv[0];
  Vec<3> T, B, N;

  T[0] = UV_2[1] * E_1[0] - UV_1[1] * E_2[0];
  T[1] = UV_2[1] * E_1[1] - UV_1[1] * E_2[1];
  T[2] = UV_2[1] * E_1[2] - UV_1[1] * E_2[2];

  B[0] = -UV_2[0] * E_1[0] + UV_1[0] * E_2[0];
  B[1] = -UV_2[0] * E_1[1] + UV_1[0] * E_2[1];
  B[2] = -UV_2[0] * E_1[2] + UV_1[0] * E_2[2];

  N = normals.normalized();
  T = (T - N * (N * T)).normalized();
  B = N.cross(T).normalized();
  Matrix<3> TBN;
  TBN.set_col(0, T);
  TBN.set_col(1, B);
  TBN.set_col(2, N);
  return TBN;
}

double triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
  return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
               (ay - cy) * (ax + cx));
}

void scan_line(int y, int x_start, int x_end, TGAImage &framebuffer,
               TGAColor color) {
  for (int x = x_start; x <= x_end; x++) {
    framebuffer.set(x, y, color);
  }
}

Vec<3> persp(const Vec<3> &v) {
  constexpr double c = 3.0;
  Vec<3> res;
  double factor = c / (c - v[2]);
  res[0] = v[0] * factor;
  res[1] = v[1] * factor;
  res[2] = v[2] * factor;
  return res;
}

std::tuple<int, int, int> pre(float x0, float y0, float z0) {
  return std::make_tuple(static_cast<int>((x0 + 1.) * width / 2),
                         static_cast<int>((y0 + 1.) * height / 2),
                         static_cast<int>((z0 + 1.) * 255 / 2));
}

Vec<3> pre_vec(const Vec<3> &v) {
  Vec<3> res;
  res[0] = (v[0] + 1.) * width / 2;
  res[1] = (v[1] + 1.) * height / 2;
  res[2] = (v[2] + 1.) * 255 / 2;
  return res;
}
Matrix<4> create_light_viewport_matrix(const int size) {
  // size 即 SHADOW_SIZE，例如 1024

  Matrix<4> LightViewport = {
      {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}};

  // 1. X轴和Y轴的缩放与平移
  // 目标：将 [-1, 1] 映射到 [0, size]
  // 缩放因子：size / 2
  // 平移因子：(size - 1) / 2

  LightViewport[0][0] = (double)size / 2.0;       // 缩放 X
  LightViewport[0][3] = (double)(size - 1) / 2.0; // 平移 X

  LightViewport[1][1] = (double)size / 2.0;       // 缩放 Y
  LightViewport[1][3] = (double)(size - 1) / 2.0; // 平移 Y

  // 2. Z轴的缩放与平移
  // 目标：将 [-1, 1] 映射到 [0, 1] (深度范围)
  // 缩放因子：0.5 (即 1 / 2)
  // 平移因子：0.5

  LightViewport[2][2] = 0.5;
  LightViewport[2][3] = 0.5;

  // 3. W 轴保持 1
  LightViewport[3][3] = 1.0;

  return LightViewport;
}

int main(int argc, char **argv) {
  TGAImage framebuffer(width, height, TGAImage::RGB);
  TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

  std::vector<std::vector<int>> faces;
  std::vector<TGAColor> vertices;
  Model model("../obj/african_head/african_head.obj");
  const Vec<3> light_dir{1, 1, 1};
  const Vec<3> eye{-1, 0, 2};   // camera position
  const Vec<3> center{0, 0, 0}; // camera direction
  const Vec<3> up{0, 1, 0};     // camera up vector

  std::srand(std::time({}));

  TextureShader shader(light_dir, model);

  lookat(eye, center, up);
  lookat_shadow(light_dir * 100, center, up);
  const double R = 2.0;
  const double NEAR = 0.1;
  const double FAR = 200.0; // 远平面必须足够远，以覆盖场景深度
  init_light_perspective(R, NEAR, FAR);
  LightViewport = create_light_viewport_matrix(width);
  LightMVP = LightPerspective * LightView;
  init_perspective(norm(eye - center));
  init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);

  init_zbuffer(width, height);
  Matrix<4> LightMVP_Viewport = LightViewport * LightMVP;

  Matrix<4> rotation = rot(30);

  DepthShader depth_shader(model, LightMVP_Viewport);

  for (int i = 0; i < model.nfaces(); i++) {
    Triangle tri_light{depth_shader.vertex(i, 0), depth_shader.vertex(i, 1),
                       depth_shader.vertex(i, 2)};

    // 渲染到 shadowmap
    rasterize(tri_light, depth_shader, shadowmap);
  }

  for (int i = 0; i < model.nfaces(); i++) {

    shader.color = {static_cast<unsigned char>(std::rand() % 255),
                    static_cast<unsigned char>(std::rand() % 255),
                    static_cast<unsigned char>(std::rand() % 255), 255};

    Triangle tri{shader.vertex(i, 0), shader.vertex(i, 1), shader.vertex(i, 2)};

    Triangle tri_world{
        ModelView * model.get_vertex(i, 0), ModelView * model.get_vertex(i, 1),
        ModelView * model.get_vertex(i, 2)}; // for TBN calculation
    Vec<4> n_0 = shader.MIT * model.get_normal(i, 0);
    Vec<4> n_1 = shader.MIT * model.get_normal(i, 1);
    Vec<4> n_2 = shader.MIT * model.get_normal(i, 2);
    Vec<4> normals = ((n_0 + n_1 + n_2) * (1.0 / 3.0)).normalized();
    Vec<2> uv_arr[3] = {model.get_uv(i, 0), model.get_uv(i, 1),
                        model.get_uv(i, 2)};
    shader.TBN = compute_TBN(tri_world, uv_arr,
                             Vec<3>{normals[0], normals[1], normals[2]});

    rasterize(tri, shader, framebuffer);
  }
  Model eye_model("../obj/african_head/african_head_eye_inner.obj");
  TextureShader eye_shader(light_dir, eye_model);

  for (int i = 0; i < eye_model.nfaces(); i++) {

    eye_shader.color = {static_cast<unsigned char>(std::rand() % 255),
                        static_cast<unsigned char>(std::rand() % 255),
                        static_cast<unsigned char>(std::rand() % 255), 255};

    Triangle tri{eye_shader.vertex(i, 0), eye_shader.vertex(i, 1),
                 eye_shader.vertex(i, 2)};
    Triangle tri_world{ModelView * eye_model.get_vertex(i, 0),
                       ModelView * eye_model.get_vertex(i, 1),
                       ModelView *
                           eye_model.get_vertex(i, 2)}; // for TBN calculation

    Vec<4> n_0 = eye_shader.MIT * eye_model.get_normal(i, 0);
    Vec<4> n_1 = eye_shader.MIT * eye_model.get_normal(i, 1);
    Vec<4> n_2 = eye_shader.MIT * eye_model.get_normal(i, 2);
    Vec<4> normals = ((n_0 + n_1 + n_2) * (1.0 / 3.0)).normalized();
    Vec<2> uv_arr[3] = {eye_model.get_uv(i, 0), eye_model.get_uv(i, 1),
                        eye_model.get_uv(i, 2)};
    eye_shader.TBN = compute_TBN(tri_world, uv_arr,
                                 Vec<3>{normals[0], normals[1], normals[2]});

    rasterize(tri, eye_shader, framebuffer);
  }
  // Model eye_outer_model("../obj/african_head/african_head_eye_outer.obj");
  // TextureShader eye_outer_shader(light_dir, eye_model);
  //
  // for (int i = 0; i < eye_model.nfaces(); i++) {
  //
  //   shader.color = {static_cast<unsigned char>(std::rand() % 255),
  //                   static_cast<unsigned char>(std::rand() % 255),
  //                   static_cast<unsigned char>(std::rand() % 255), 255};
  //
  //   Triangle tri{eye_outer_shader.vertex(i, 0), eye_outer_shader.vertex(i,
  //   1),
  //                eye_outer_shader.vertex(i, 2)};
  //
  //   rasterize(tri, shader, framebuffer);
  // }

  framebuffer.write_tga_file("framebuffer.tga");
  return 0;
}
