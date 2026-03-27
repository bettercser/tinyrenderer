#pragma once

#include "../Vector.hh"
#include "../scene/scene.hpp"
#include "tracer_config.hpp"

Vec<3> trace_ray(const Ray &ray, const Scene &scene, int depth,
                 const TracerConfig &config);
