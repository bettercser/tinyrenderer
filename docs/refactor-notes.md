# Notes

## 完成内容
- 将 main.cpp 中的 5 个 shader 拆分到 render/shaders/
- 将 IShader 与 Triangle 公共定义移动到 render/shader.hpp
- main.cpp 仅保留 shader 使用，不再保留 shader 定义

## 当前仍存在的问题
- shader 仍直接依赖 gl.hh 中的全局矩阵
- varying_w_recip 和 TBN 仍放在 IShader 中
- rasterize 与全局状态仍耦合

## 下一步
- 引入 RenderContext，开始收全局状态
