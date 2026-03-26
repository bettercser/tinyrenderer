#include "camera.hpp"
#include <cmath>



Ray Camera::generate_ray(double u, double v) const {

    double tan_half_fov = std::tan(fov * 0.5 * M_PI / 180.0);
    double px = (2.0 * u - 1.0) * aspect_ratio * tan_half_fov;
    double py = (1.0 - 2.0 * v) * tan_half_fov;
    Vec<3> ray_dir = (forward + right * px + up * py).normalized();
    return Ray(origin, ray_dir);
}