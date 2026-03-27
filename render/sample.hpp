#pragma once

#include "../Vector.hh"
#include "utils/render_util.hpp"
#include <cstdlib>

inline Vec<2> sample_pixel_uv(int x, int y, int width, int height, int sx,
                              int sy, int nx, int ny) {
  double u = (x + (sx + random_double()) / static_cast<double>(nx)) /
             static_cast<double>(width);
  double v = (y + (sy + random_double()) / static_cast<double>(ny)) /
             static_cast<double>(height);
  return Vec<2>{u, v};
}
