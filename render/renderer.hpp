#pragma once

#include "../model.hpp"
#include "../tgaimage.h"
#include "shader.hpp"



class Renderer {
public:
    void draw_model(const Model& model, IShader& shader, TGAImage& framebuffer);

};