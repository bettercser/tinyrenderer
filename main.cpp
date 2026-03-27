#include "Vector.hh"
#include "gl.hh"
#include "matrix.hh"
#include "model.hpp"
#include "render/material.hpp"
#include "render/rasterizer.hpp"
#include "render/renderer.hpp"
#include "render/shader.hpp"
#include "render/shaders/depth_shader.hpp"
#include "render/shaders/phong_shader.hpp"
#include "render/shaders/random_shader.hpp"
#include "render/shaders/smooth_shader.hpp"
#include "render/shaders/texture_shader.hpp"
#include "render/tracer.hpp"
#include "render/tracer_config.hpp"
#include "render/utils/render_util.hpp"
#include "scene/camera.hpp"
#include "scene/scene.hpp"
#include "tgaimage.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <vector>

constexpr int width = 1024;
constexpr int height = 1024;

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};
TGAImage shadowmap(width, height, TGAImage::GRAYSCALE);

double triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
  return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
               (ay - cy) * (ax + cx));
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

double random_double() { return rand() / (RAND_MAX + 1.0); }

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
  FrameBuffer frame(width, height);
  RenderContext ctx;
  ctx.ModelView = ModelView;
  ctx.Projection = Perspective;
  ctx.ViewPort = Viewport;
  ctx.frameBuffer = &frame;

  DepthShader depth_shader(model, LightMVP_Viewport);
  depth_shader.renderContext = &ctx;
  shader.renderContext = &ctx;

  Renderer renderer;
  renderer.draw_model(model, depth_shader, shadowmap);

  // for (int i = 0; i < model.nfaces(); i++) {

  //   shader.color = {static_cast<unsigned char>(std::rand() % 255),
  //                   static_cast<unsigned char>(std::rand() % 255),
  //                   static_cast<unsigned char>(std::rand() % 255), 255};

  //   Triangle tri{shader.vertex(i, 0), shader.vertex(i, 1), shader.vertex(i,
  //   2)};

  //   Triangle tri_world{
  //       ModelView * model.get_vertex(i, 0), ModelView * model.get_vertex(i,
  //       1), ModelView * model.get_vertex(i, 2)}; // for TBN calculation
  //   Vec<4> n_0 = shader.MIT * model.get_normal(i, 0);
  //   Vec<4> n_1 = shader.MIT * model.get_normal(i, 1);
  //   Vec<4> n_2 = shader.MIT * model.get_normal(i, 2);
  //   Vec<4> normals = ((n_0 + n_1 + n_2) * (1.0 / 3.0)).normalized();
  //   Vec<2> uv_arr[3] = {model.get_uv(i, 0), model.get_uv(i, 1),
  //                       model.get_uv(i, 2)};
  //   shader.renderContext->TBN = compute_TBN(tri_world, uv_arr,
  //                            Vec<3>{normals[0], normals[1], normals[2]});

  //   rasterize(tri, shader, framebuffer);
  // }
  Model eye_model("../obj/african_head/african_head_eye_inner.obj");
  TextureShader eye_shader(light_dir, eye_model);

  eye_shader.renderContext = &ctx;
  // for (int i = 0; i < eye_model.nfaces(); i++) {

  //   eye_shader.color = {static_cast<unsigned char>(std::rand() % 255),
  //                       static_cast<unsigned char>(std::rand() % 255),
  //                       static_cast<unsigned char>(std::rand() % 255), 255};

  //   Triangle tri{eye_shader.vertex(i, 0), eye_shader.vertex(i, 1),
  //                eye_shader.vertex(i, 2)};
  //   Triangle tri_world{ModelView * eye_model.get_vertex(i, 0),
  //                      ModelView * eye_model.get_vertex(i, 1),
  //                      ModelView *
  //                          eye_model.get_vertex(i, 2)}; // for TBN
  //                          calculation

  //   Vec<4> n_0 = eye_shader.MIT * eye_model.get_normal(i, 0);
  //   Vec<4> n_1 = eye_shader.MIT * eye_model.get_normal(i, 1);
  //   Vec<4> n_2 = eye_shader.MIT * eye_model.get_normal(i, 2);
  //   Vec<4> normals = ((n_0 + n_1 + n_2) * (1.0 / 3.0)).normalized();
  //   Vec<2> uv_arr[3] = {eye_model.get_uv(i, 0), eye_model.get_uv(i, 1),
  //                       eye_model.get_uv(i, 2)};
  //   eye_shader.renderContext->TBN = compute_TBN(tri_world, uv_arr,
  //                                                Vec<3>{normals[0],
  //                                                normals[1], normals[2]});

  //   rasterize(tri, eye_shader, framebuffer);
  // }

  Scene scene;
  // scene.models.push_back(&model);
  // scene.models.push_back(&eye_model);
  // Material skin_material;
  // skin_material.albedo = Vec<3>{1.0, 0.8, 0.7};
  // skin_material.roughness = 0.5;
  // Camera ray_camera;
  // ray_camera.origin = eye;
  // ray_camera.aspect_ratio = static_cast<double>(width) / height;
  // Ray center_ray = ray_camera.generate_ray(0.5, 0.5);
  Material test_material;
  test_material.albedo = Vec<3>{1.0, 0.0, 0.0};
  Sphere test_sphere;
  test_sphere.center = Vec<3>{0.0, 0.0, -3.0};
  test_sphere.radius = 1.0;
  test_sphere.material = &test_material;

  // Vec<3> ray_color = trace_ray(center_ray, test_sphere);
  // std::cout << "Ray color: " << ray_color << std::endl;
  constexpr int ray_width = 256;
  constexpr int ray_height = 256;
  TGAImage ray_image(ray_width, ray_height, TGAImage::RGB);

  Camera ray_camera;
  ray_camera.origin = Vec<3>{0.0, 0.0, 0.0};
  ray_camera.forward = Vec<3>{0.0, 0.0, -1.0};
  ray_camera.right = Vec<3>{1.0, 0.0, 0.0};
  ray_camera.up = Vec<3>{0.0, 1.0, 0.0};
  ray_camera.aspect_ratio = static_cast<double>(ray_width) / ray_height;

  Material red_mat;
  red_mat.albedo = Vec<3>{1.0, 0.2, 0.2};

  Material green_mat;
  green_mat.albedo = Vec<3>{0.2, 1.0, 0.2};

  Material blue_mat;
  blue_mat.type = MaterialType::Metal;
  blue_mat.albedo = Vec<3>{0.8, 0.85, 1.0};
  blue_mat.roughness = 0.05;

  Material ground_mat;
  ground_mat.albedo = Vec<3>{0.8, 0.8, 0.75};

  Material glass_mat;
  glass_mat.type = MaterialType::Dielectric;
  glass_mat.ior = 1.5;

  Sphere sphere1;
  sphere1.center = Vec<3>{0.1, 0.5, -3.0};
  sphere1.radius = 1.0;
  sphere1.material = &red_mat;

  Sphere sphere2;
  sphere2.center = Vec<3>{-1.5, 0.3, -4.0};
  sphere2.radius = 0.8;
  sphere2.material = &green_mat;

  Sphere sphere3;
  sphere3.center = Vec<3>{1.4, 0.0, -2.5};
  sphere3.radius = 0.5;
  sphere3.material = &blue_mat;

  Sphere ground;
  ground.center = Vec<3>{0.0, -100.5, -3.0};
  ground.radius = 100.0;
  ground.material = &ground_mat;

  Sphere glass_sphere;
  glass_sphere.center = Vec<3>{0.9, 0.15, -2.2};
  glass_sphere.radius = 0.5;
  glass_sphere.material = &glass_mat;

  scene.spheres.push_back(sphere1);
  scene.spheres.push_back(sphere2);
  scene.spheres.push_back(sphere3);
  scene.spheres.push_back(ground);
  scene.spheres.push_back(glass_sphere);
  TracerConfig tracer_config;
  Vec<3> light_direction = Vec<3>{-1.0, -1.0, 1.0}.normalized();

  for (int y = 0; y < ray_height; y++) {
    for (int x = 0; x < ray_width; x++) {

      Vec<3> accumalate_color{0.0, 0.0, 0.0};

      for (int s = 0; s < tracer_config.samples_per_pixel; s++) {

        double u = (x + random_double()) / static_cast<double>(ray_width);
        double v = (y + random_double()) / static_cast<double>(ray_height);
        Ray ray = ray_camera.generate_ray(u, v);

        accumalate_color +=
            trace_ray(ray, scene, tracer_config.max_depth, tracer_config);
      }
      Vec<3> ray_color = accumalate_color /
                         static_cast<double>(tracer_config.samples_per_pixel);

      ray_color[0] = std::sqrt(std::max(0.0, ray_color[0]));
      ray_color[1] = std::sqrt(std::max(0.0, ray_color[1]));
      ray_color[2] = std::sqrt(std::max(0.0, ray_color[2]));

      TGAColor out_color{static_cast<unsigned char>(
                             255.0 * std::clamp(ray_color[2], 0.0, 1.0)),
                         static_cast<unsigned char>(
                             255.0 * std::clamp(ray_color[1], 0.0, 1.0)),
                         static_cast<unsigned char>(
                             255.0 * std::clamp(ray_color[0], 0.0, 1.0)),
                         255};

      ray_image.set(x, ray_height - 1 - y, out_color);
    }
  }

  ray_image.write_tga_file("ray_traced_sphere_spp128.tga");

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
