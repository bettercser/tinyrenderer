#pragma once

#include "../shader.hpp"
#include <cstdlib>

constexpr double PI = 3.14159265358979323846;

inline Matrix<3> compute_TBN(const Triangle &tri, const Vec<2> uv[3],
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

inline Matrix<4> rot(double angle) {
  double c = std::cos(angle * PI / 180.);
  double s = std::sin(angle * PI / 180.);

  Matrix<4> r = {{{c, 0, s, 0}, {0, 1, 0, 0}, {-s, 0, c, 0}, {0, 0, 0, 1}}};
  return r;
}

inline double random_double() { return std::rand() / (RAND_MAX + 1.0); }
