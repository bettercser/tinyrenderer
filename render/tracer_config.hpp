

#pragma once

struct TracerConfig {

  int max_depth = 5;
  int samples_per_pixel = 144;
  double ray_epsilon = 1e-4;

  int rr_start_depth = 3; // Russian Roulette 开始的递归深度

  bool enable_direct_lighting = true; // 是否启用直接光照计算
};
