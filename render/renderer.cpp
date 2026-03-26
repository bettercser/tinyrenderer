#include "renderer.hpp"



#include "rasterizer.hpp"


void Renderer::draw_model(const Model& model, IShader& shader, TGAImage& framebuffer) {

    for (int i = 0; i < model.nfaces(); i++) {
        Triangle tri;
        for (int j = 0; j < 3; j++) {
            tri[j] = shader.vertex(i, j);
        }
        rasterize(tri, shader, framebuffer);
    }
}

