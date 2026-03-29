#pragma once

#include "../Vector.hh"
#include "../tgaimage.h"
#include <algorithm>
#include <cmath>
#include <vector>

struct TextureMipChain {
  std::vector<TGAImage> mip_levels;

  bool empty() const { return mip_levels.empty(); }
  int levels() const { return static_cast<int>(mip_levels.size()); }

  const TGAImage &get_level(int level) const {
    if (level < 0) {
      return mip_levels.front();
    } else if (level >= levels()) {
      return mip_levels.back();
    }
    return mip_levels[level];
  }
};

inline TGAImage downsample_2x(const TGAImage &src) {
  int dst_w = std::max(1, src.width() / 2);
  int dst_h = std::max(1, src.height() / 2);

  TGAImage dst(dst_w, dst_h, src.get_bytespp());

  for (int y = 0; y < dst_h; y++) {

    for (int x = 0; x < dst_w; x++) {

      int sx = x * 2;
      int sy = y * 2;

      TGAColor c00 = src.get(std::min(sx, src.width() - 1),
                             std::min(sy, src.height() - 1));
      TGAColor c10 = src.get(std::min(sx + 1, src.width() - 1),
                             std::min(sy, src.height() - 1));
      TGAColor c01 = src.get(std::min(sx, src.width() - 1),
                             std::min(sy + 1, src.height() - 1));
      TGAColor c11 = src.get(std::min(sx + 1, src.width() - 1),
                             std::min(sy + 1, src.height() - 1));

      TGAColor avg_color;

      for (int i = 0; i < src.get_bytespp(); i++) {
        avg_color[i] = (static_cast<int>(c00[i]) + static_cast<int>(c10[i]) +
                        static_cast<int>(c01[i]) + static_cast<int>(c11[i])) /
                       4;
      }

      dst.set(x, y, avg_color);
    }
  }
  return dst;
}

inline TextureMipChain build_mip_chain(const TGAImage &base) {
  TextureMipChain chain;
  chain.mip_levels.push_back(base);

  while (chain.mip_levels.back().width() > 1 ||
         chain.mip_levels.back().height() > 1) {
    chain.mip_levels.push_back(downsample_2x(chain.mip_levels.back()));
  }
  return chain;
}

inline TGAColor sample_mip_nearest(const TextureMipChain &chain,
                                   const Vec<2> &uv, int level) {
  const TGAImage &img = chain.get_level(level);
  int x = std::clamp(static_cast<int>(uv[0] * img.width()), 0, img.width() - 1);
  int y =
      std::clamp(static_cast<int>(uv[1] * img.height()), 0, img.height() - 1);
  return img.get(x, y);
}

inline TGAColor sample_mip_bilinear(const TextureMipChain &chain,
                                    const Vec<2> &uv, int level) {

  const TGAImage &img = chain.get_level(level);
  double x = uv[0] * (img.width() - 1);
  double y = uv[1] * (img.height() - 1);

  int x0 = static_cast<int>(std::floor(x));
  int y0 = static_cast<int>(std::floor(y));
  int x1 = std::min(x0 + 1, img.width() - 1);
  int y1 = std::min(y0 + 1, img.height() - 1);

  double tx = x - x0;
  double ty = y - y0;

  TGAColor c00 = img.get(x0, y0);
  TGAColor c10 = img.get(x1, y0);
  TGAColor c01 = img.get(x0, y1);
  TGAColor c11 = img.get(x1, y1);

  TGAColor result;

  for (int i = 0; i < img.get_bytespp(); i++) {
    double c0 = (1 - tx) * c00[i] + tx * c10[i];
    double c1 = (1 - tx) * c01[i] + tx * c11[i];
    result[i] = static_cast<unsigned char>((1 - ty) * c0 + ty * c1);
  }
  return result;
}
