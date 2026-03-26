#pragma once


#include "../Vector.hh"
#include "material.hpp"


struct HitRecord {
    double t = 0.0;
    Vec<3> point;
    Vec<3> normal;
    Vec<2> uv;

    const Material* material = nullptr;
};