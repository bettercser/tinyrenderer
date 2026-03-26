## Shader 与 Rasterizer 的职责约定

在当前项目中，统一采用如下职责划分：

- vertex() 返回裁剪空间（clip space）顶点
- Rasterizer 负责透视除法与视口变换
- fragment() 负责材质采样与光照计算

这样可以保证不同 shader 的输入输出约定一致，便于后续增加
Phong、Blinn-Phong、Depth、Debug 等 shader。



## 当前仍需继续统一的点

- 部分 shader 的 vertex() 返回空间仍需统一
- 光照方向、法线、TBN 所在空间需要进一步明确
- Depth pass 与主 pass 的顶点输出约定仍可继续收敛



# 当前 tinyrenderer 的一帧数据流

main.cpp
- 加载模型资源
- 配置相机、光源、投影 视口
- 保留 渲染上下文
- 驱动主渲染pass

Render
- 遍历三角形面
- 生成三角形顶点
- 交给Rasterizer


Rasterizer
- 做透视除法以及视口变换
- 计算包围盒
- 遍历像素，判断是否在三角形内
- 深度测试 （这种顺序与 GPU 管线中的 early depth test 思想相似，但当前实现本质上仍然是软件光栅化中的手工深度测试流程）
- 插值
- 片元着色
