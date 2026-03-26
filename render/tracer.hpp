#pragma once

#include "../scene/scene.hpp"
#include "../Vector.hh"



Vec<3> trace_ray(const Ray& ray, const Scene& scene, int depth);