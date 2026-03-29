#pragma once

#include "../Vector.hh"
#include "../scene/scene.hpp"
#include "../scene/sphere.hpp"
#include "light.hpp"
#include "tracer_config.hpp"

Vec<3> trace_ray(const Ray &ray, const Scene &scene, int depth,
                 const DirectionalLight &light, const TracerConfig &config,
                 bool from_delta_bounce = false, bool is_primary_ray = true,
                 const Vec<3> &prev_shading_normal = Vec<3>{1.0, 1.0, 1.0});
