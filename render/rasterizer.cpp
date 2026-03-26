#include "rasterizer.hpp"
#include "../gl.hh"

Vec<3> barycentric(const Triangle &tri, const Vec<3> &P) {
  Vec<3> u =
      (Vec<3>{tri[2][0] - tri[0][0], tri[1][0] - tri[0][0], tri[0][0] - P[0]})
          .cross(Vec<3>{tri[2][1] - tri[0][1], tri[1][1] - tri[0][1],
                        tri[0][1] - P[1]});
  if (std::abs(u[2]) < 1e-2)
    return Vec<3>{-1, 1, 1};
  return Vec<3>{1.0 - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]};
}



void rasterize(const Triangle &tri, IShader &shader, TGAImage &framebuffer) {

  Vec<4> p0 = Viewport * (tri[0] / tri[0][3]);
  Vec<4> p1 = Viewport * (tri[1] / tri[1][3]);
  Vec<4> p2 = Viewport * (tri[2] / tri[2][3]);
  Triangle pts = {p0, p1, p2};
  Vec<2> bbox_min{std::numeric_limits<double>::max(),
                  std::numeric_limits<double>::max()};
  Vec<2> bbox_max{-std::numeric_limits<double>::max(),
                  -std::numeric_limits<double>::max()};
  Vec<2> clamp{static_cast<double>(framebuffer.width() - 1),
               static_cast<double>(framebuffer.height() - 1)};

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 2; j++) {
      bbox_min[j] = std::max(0.0, std::min(bbox_min[j], pts[i][j]));
      bbox_max[j] = std::min(clamp[j], std::max(bbox_max[j], pts[i][j]));
    }
  }

  int min_x = std::floor(bbox_min[0]);
  int max_x = std::ceil(bbox_max[0]);
  int min_y = std::floor(bbox_min[1]);
  int max_y = std::ceil(bbox_max[1]);

  Vec<3> P;

  for (int x = min_x; x <= max_x; x++) {
    for (int y = min_y; y <= max_y; y++) {
      P[0] = static_cast<double>(x) + .5;
      P[1] = static_cast<double>(y) + .5;
      P[2] = 0; // z坐标暂时不用，后面插值算

      Vec<3> bc_screen = barycentric(pts, P);
      if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0)
        continue;
      double z = 0;
      for (int i = 0; i < 3; i++)
        z += pts[i][2] * bc_screen[i];

      int idx = x + y * framebuffer.width();

      if (z > ZBuffer[idx]) {
        ZBuffer[idx] = z;

        float w_recip_intrepolated = bc_screen[0] * shader.renderContext->varying_w_recip[0] +
                                     bc_screen[1] * shader.renderContext->varying_w_recip[1] +
                                     bc_screen[2] * shader.renderContext->varying_w_recip[2];
        Vec<3> bc_clip;
        bc_clip[0] =
            bc_screen[0] * shader.renderContext->varying_w_recip[0] / w_recip_intrepolated;
        bc_clip[1] =
            bc_screen[1] * shader.renderContext->varying_w_recip[1] / w_recip_intrepolated;
        bc_clip[2] =
            bc_screen[2] * shader.renderContext->varying_w_recip[2] / w_recip_intrepolated;
        auto [discard, color] = shader.fragment(bc_clip); // 片元着色器
        if (!discard) {
          framebuffer.set(static_cast<int>(P[0]), static_cast<int>(P[1]),
                          color);
        }
      }
    }
  }
}