#pragma once


#include "../model.hpp"
#include "sphere.hpp"

#include <vector>


struct Scene {
    std::vector<const Model*> models;
    std::vector<Sphere> spheres;

    bool hit(const Ray& ray, double t_min, double t_max, HitRecord& rec) const;
};