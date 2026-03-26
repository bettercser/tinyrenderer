#pragma once


#include "../Vector.hh"
#include "../render/ray.hpp"



class Camera {

public:
    Vec<3> origin{0.0, 0.0, 0.0};
    Vec<3> forward{0.0, 0.0, -1.0};
    Vec<3> right{1.0, 0.0, 0.0};
    Vec<3> up{0.0, 1.0, 0.0};

    double fov = 60.0; // Field of View
    double aspect_ratio = 1.0;

    Ray generate_ray(double u, double v) const;

};