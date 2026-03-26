#pragma once

#include "../Vector.hh"



struct Ray {
    Vec<3> origin;
    Vec<3> direction;

    Ray(const Vec<3>& o, const Vec<3>& d) : origin(o), direction(d.normalized()) {}
};