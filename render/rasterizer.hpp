#pragma once
#include "shader.hpp"
#include "../tgaimage.h"

Vec<3> barycentric(const Triangle &tri, const Vec<3> &P);

void rasterize(const Triangle& tri, IShader& shader, TGAImage& framebuffer);