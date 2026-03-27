#pragma once

#include "../Vector.hh"
#include "../scene/scene.hpp"
#include "light.hpp"
#include "tracer_config.hpp"

Vec<3> trace_ray(const Ray &ray, const Scene &scene, int depth,
                 const DirectionalLight &light, const TracerConfig &config);
