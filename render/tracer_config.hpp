

#pragma once

struct TracerConfig {

  int max_depth = 5;
  int samples_per_pixel = 128;
  double ray_epsilon = 1e-4;

  int rr_start_depth = 3; // Russian Roulette 开始的递归深度
};
