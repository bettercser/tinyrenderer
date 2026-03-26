#include "sphere.hpp"



bool Sphere::hit(const Ray& ray, double t_min, double t_max, HitRecord& rec) const {


    Vec<3> oc = ray.origin - center;
    double a = ray.direction * ray.direction;
    double b = 2.0 * (oc * ray.direction);
    double c = oc * oc - radius * radius;
    double discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return false;
    }

    double sqrt_disc = std::sqrt(discriminant);
    double t1 = (-b - sqrt_disc) / (2.0 * a);
    double t2 = (-b + sqrt_disc) / (2.0 * a);

    double t_hit = t1;
    if (t_hit < t_min || t_hit > t_max) {
        t_hit = t2;
        if (t_hit < t_min || t_hit > t_max) {
            return false;
        }
    }

    rec.t = t_hit;
    rec.point = ray.origin + ray.direction * rec.t;
    rec.normal = (rec.point - center).normalized();
    rec.material = material;

    return true;
    
}