#pragma once

#include "../Vector.hh"

struct DirectionalLight {
  Vec<3> direction{1.0, -1.0, -1.0};
  Vec<3> color{1.0, 1.0, 1.0};
  double intensity = 1.0;
};
