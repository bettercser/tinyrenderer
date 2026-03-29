#pragma once

#include "../Vector.hh"

struct LightRecord {
  Vec<3> position;
  Vec<3> emission;
  double radius = 0.0;
};
