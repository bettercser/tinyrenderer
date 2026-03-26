#pragma once


#include "../Vector.hh"




struct Material {
    // 基础反射颜色
    Vec<3> albedo{1.0, 1.0, 1.0};
    
    double metallic = 0.0; // 金属度
    double roughness = 0.5; // 粗糙度
};