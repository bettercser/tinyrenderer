#pragma once

#include "../matrix.hh"
#include "../tgaimage.h"
#include <array>
#include <limits>
#include <vector>



struct FrameBuffer {
    TGAImage color;
    std::vector<double> depth;

    FrameBuffer(int width, int height) : color(width, height, TGAImage::RGB), depth(width * height, -std::numeric_limits<double>::infinity()) {}
};

struct RenderContext {
    Matrix<4> ModelView, Projection, ViewPort;

    Matrix<3> TBN;

    std::array<double, 3> varying_w_recip;

    FrameBuffer* frameBuffer;

};