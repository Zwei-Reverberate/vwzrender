---
title: 从 Vulkan 开始的现代图形渲染学习
date: 2026-10-18 17:36:47
tags: Graphics
toc: true
summary: "从 Vulkan 开始的现代图形渲染学习"
---

*[配套工程实践：vwzrender](https://github.com/Zwei-Reverberate/vwzrender)*

# 1. 图形渲染概述

## 1. 成像原理

我们看到物体,是因为光源(太阳、灯泡)发出的光照射到物体上,物体与光发生交互后,一部分光朝向我们的眼睛传播,眼睛把它转换成了视觉信号。

光沿直线传播，可以被物体改变方向和形态。物体与光的交互方式决定了我们所看到的物体的外观：

- 自发光：物体自身发光；
- 吸收：光被物体吸收掉；
- 反射：光在物体表面弹开，粗糙表面产生漫反射，光滑表面产生镜面反射。

&nbsp;

图形渲染所研究的问题是：给定一个虚拟三维场景(几何、材质、光源)和一台虚拟相机,计算出成像平面上每个点的颜色。其中：

- 场景中每个点的颜色，并非其本身的颜色。而是沿着光路最终到达相机的所有光的叠加，涉及光照、阴影、多次反射等全局现象
- 成像平面上的每个点在计算机里是离散的像素，这要求连续的光学过程必须被离散化。而离散化又会引入误差，需要采样理论和抗锯齿来解决

&nbsp;

## 2. 图像

把成像平面上的每个位置 $(x, y)$ 对应的光和颜色记为 $I(x, y)$。一个理想(连续、无限精细)的图像就是这样一个二维函数。渲染的本质,就是计算这个函数在各个位置上的值。

现代显示器由数百万个可独立控制亮度的发光单元组成。每个单元就是一个像素（像素包含红、绿、蓝三个子像素）。

>光线是不同波长电磁波的混合。人眼中有三种锥细胞，对红、绿、蓝三段的敏感度不同，任何光谱分布落到人眼中都可以压缩成三个数。
>
>因此显示器只需要用红、绿、蓝三个子像素就可以让视觉合成出相应的色感，而不需要还原真实完整的光谱。渲染器中的颜色通常是三个分量，代表红、绿、蓝三种基色光的亮度贡献：
>$$
>color = (R, G, B) \quad \quad R,G,B \in [0, 1](\text{或} [0, 255])
>$$

显示器无法直接处理上文提及的连续二维函数，它只有有限个像素，每个像素只能被赋予一组红、绿、蓝的亮度值。因此，渲染必须先把 $I(x, y)$ 离散化：将成像平面划分成 $W × H$ 的网格， 每个格子对应一个像素。对第 $(i, j)$ 个像素，通常取它的中心 $(x_i, y_i)$，计算 $C_{ij} = I(x_i, y_i)$，更准确的做法是对该像素覆盖的小区域做平均或积分 $C_{ij} = \frac{1}{|P_{i, j}|} \int_{P_{ij}}I(x, y)dxdy$。这样连续的二维函数 $I(x,y)$ 就变成了一个有限的离散像素数组 $\{ C_{ij} \}^{W, H}_{i = 1, j = 1}$。

&nbsp;

## 3. 光栅化与光线追踪

现在回到核心问题：对屏幕上的每一个像素，怎样知道达到它的光是什么颜色？

模拟所有光子是物理正确但不可用的，因为计算量过于庞大。所有的实时渲染算法都是本质上都是对光的传输过程做近似。这些方法可以归纳为光线追踪和光栅化两大流派。

&nbsp;

### 1. 光线追踪

光路是可逆的。与其追踪概率极低的从光源出发，最终恰好落进眼睛的光，不如反向追踪：

- 从相机出发，朝每个像素方向投射一条射线，找出它与场景的第一个交点，计算交点处的光照；
- 如果光线在交点处发生镜面反射或折射，就继续递归追踪下去；
- 如果是漫反射则按照材质的 BRDF 进行重要性采样，用蒙特卡洛积分估计间接光照，并可通过俄罗斯轮盘赌等策略适时终止路径。

>虽然表述上是朝着画布的像素方向投射射线，但画布本身并无物理实体，不参与任何遮挡关系。像素先于射线存在。

&nbsp;

光线追踪与物理过程一一对应。反射，折射，阴影等全局效果天然正确，不用为每种效果使用特例算法。

但每条射线都要在场景中做求交，代价与像素数量和递归深度成正比。传统光线追踪慢到只能离线渲染，虽然现代 GPU 带上了专用的光追单元，但实时渲染中光追依然只是混合渲染的一部分。

>当今渲染的主流实践是混合渲染，为保证实时性，光栅化负责主体几何与着色，光追单元负责反射、阴影、全局光照等部分。

&nbsp;

### 2. 光栅化

光线追踪以像素为中心。从画布上的像素出发，向场景发射射线，求出交点。

光栅化以物体为中心。将场景的物体投影到画布上，求出覆盖了哪些像素。

&nbsp;

光栅化对每个三角形判定覆盖了哪些像素这一步非常适合硬件并行。但遮挡关系这类全局信息在局部判定中天然缺失，需要额外手段如深度缓冲，阴影贴图等模拟全局效果。

&nbsp;

## 4. 渲染管线

光栅化“把三角形投影到像素网格”这一思路可拆解成一条流水线。

流水线思想与现代工厂一致：把复杂过程划分成若干阶段，每个阶段只做一件小而确定的事，阶段间用数据流连接，再配合并行执行，大幅提高效率。

从逻辑上可划分为应用程序 → 几何 → 光栅化 → 像素处理 四个阶段。

&nbsp;

### 1. 应用程序阶段

这是渲染管线的起点，主要在 CPU 上执行，由应用程序主导，图形 API 作为与 GPU 通信的接口。该阶段决定了当前帧要渲染什么，以及如何准备数据：

- 管理与更新场景数据：维护场景图（如层级关系、空间划分结构）、几何模型（顶点/索引）、材质与纹理、光源以及相机参数，并处理动画、物理模拟或碰撞检测等业务逻辑；
- 执行粗粒度可见性剔除：在 CPU 端进行视锥剔除（Frustum Culling）和潜在的遮挡剔除（Occlusion Culling），快速剔除完全不在相机视野内或被遮挡的物体，从而减少后续需要提交给 GPU 的几何数据量；
- 准备渲染状态与资源：计算物体的模型、视图、投影矩阵；绑定所需的着色器程序、纹理资源和常量缓冲区；设置图元装配和光栅化状态；
- 提交绘制命令：将经过剔除和排序的物体放入渲染队列，通过图形 API 向 GPU 发送绘制调用（Draw Call）。

该阶段最终输出的是包含渲染状态、顶点数据及绘制指令的命令列表，供后续的 GPU 管线（几何处理、光栅化等）消费。

&nbsp;

### 2. 几何阶段

几何阶段主要在 GPU 上执行，其处理的基本对象是顶点数据，每个顶点携带若干属性，如位置、法线、纹理坐标、颜色等。

>图元结构决定了以哪种基本的几何表示来进行渲染。一般有：三角形，线段，点。通常使用三角形

该阶段的核心任务是将三维空间中的顶点坐标变换到二维屏幕空间，并将顶点组织成图元，为后续的光栅化做准备。它内部包含以下几个子步骤：

&nbsp;

**1. 顶点着色器**

这是对每个顶点独立执行的一段程序，可编程在 GPU 中并行执行。

顶点之间互相独立，顶点着色器无法访问其他顶点的信息。顶点着色器不能创建或销毁顶点。

主要工作是将顶点从模型空间通过 MVP 矩阵变换到齐次裁剪空间：

- 模型变换：将顶点从各自的局部坐标系转换到统一的世界坐标系

  [在变换法线数据时需要注意，如果存在非均匀缩放变换则要使用矩阵的逆的转置来变换法线](https://www.songho.ca/opengl/gl_normaltransform.html)

- 观察变换：将顶点从世界坐标系转换到观察坐标系，由虚拟相机的位置与朝向决定

- 投影变换：将顶点从观察空间转换到裁剪空间

  视锥体：由上下左右远近六个平面定义。透视投影的视锥体是一个四棱台，近大远小。正交投影的视锥体是一个长方体。

  裁剪空间：为了方便计算，投影矩阵结合齐次坐标 $(x, y, z, w)$ 将可视范围映射到齐次裁剪空间中。裁剪空间内坐标的 $x, y , z$ 分量的范围都在 $[-w, w]$ 之间。透视投影下 $w$ 通常与观察空间深度相关，用于透视除法；正交投影下 $w$ 常为 1。
  
  >OpenGL 风格为 $[-w, w]$，Vulkan/D3D 中 $z$ 为 $[0, w]$。

&nbsp;

顶点着色：在顶点着色器中同样可以进行着色计算。这种着色是逐顶点的，随后在光栅化阶段通过插值得到三角形的内部光照。

该方法效果并不理想，因为各片段的着色信息可能非线性变换的，通过插值不能准确反映。此方法称为高洛德着色。不过计算代价低，一个复杂模型可能只有几万个顶点，在顶点着色器中计算光照计算量只有几万次；如果放在片元着色器中就有上百万次。

&nbsp;

**2. 曲面细分着色器和几何着色器**

这两个阶段属于可编程但非必选环节。

曲面细分着色器利用镶嵌化技术对三角面进行细分，以此来增加三角形面数。常用于生成平滑的曲线曲面，以及实现动态细节层次（LOD）。

几何着色器以完整的图元作为输入（如以三角形的三个顶点作为输入）然后输出对应的图元。可以创建或销毁几何图元，亦可根据输入图元类型扩展为其他更多类型的图元。

&nbsp;

**3. 图元装配**

图元装配 (Primitive Assembly) 将顶点数据组装成指定的图元。这一阶段由 GPU 硬件固定管线执行，不可编程。

>几何阶段典型的执行顺序：顶点着色器 → (曲面细分着色器) → 图元装配 → (几何着色器) → 裁剪 → 透视除法 → 屏幕映射

&nbsp;

**4. 裁剪**

裁剪发生在投影变换之后，透视除法之前，即在齐次裁剪空间中进行。

完全在裁剪空间内的图元保留；完全在外的舍弃；部分在内的则被切掉空间外的部分，并生成新的顶点。

&nbsp;

**5. 透视除法**

顶点坐标除以 $w$ 分量，将齐次坐标转化为归一化设备坐标 (NDC)。此时的空间被称为标准视体 (CVV)，坐标范围通常在 $[−1,1]$ 之间。

>OpenGL 中为 $[-1, 1]$，Vulkan/D3D 中 $z$ 为 $[0, 1]$。

&nbsp;

**6. 屏幕映射**

将 NDC 的 $x, y$ 从 $[−1,1]$ 映射到屏幕像素坐标 $[0,w]$ 和 $[0,h]$（$w, h$ 为屏幕宽高）。

屏幕映射不对 $z$ 坐标作处理，NDC 中的 $z$ 值将用于后续的深度测试。

&nbsp;

**7. 背面剔除**

剔除背对摄像机的图元，利用三角形顶点点序来确定正面和背面。OpenGL 默认认为 ccw(counter-clockwise) 是正面，D3D 默认 CW 正面，Vulkan 需要显式配置。

渲染透明或者半透明物体时，使用背面剔除会出现问题。因为此时可以看到物体的背面。

发生在图元装配之后，光栅化之前。而具体阶段没有固定，它会因图形API和硬件实现的不同而有所变化。在 Vulkan 中由光栅化状态配置。

>背面剔除是基于图元的朝向（顶点环绕顺序）进行的，因此它必然发生在图元被组装成三角形之后。

&nbsp;

### 3. 光栅化阶段

光栅化阶段接收来自几何阶段的图元数据，将连续的几何图元离散化为屏幕上的片元 (Fragment)。

片元只是像素的候选者。它包含了颜色、深度以及纹理坐标等插值后的数据，只有通过了后续的各项测试，才会真正成为屏幕上的像素。

过程会遍历三角形，寻找三角形所覆盖的所有像素中心点，对覆盖的像素，利用三角形的顶点信息进行插值，生成该像素对应的片元数据。遍历完成后，输出一个片元序列。由于物体重叠，同一屏幕像素位置可能会生成多个来自不同三角形的片元。

例如一个三角形覆盖了 10 个像素中心，就生成约 10 个片元。每个片元携带在屏幕上对应的二维坐标，深度，三角形顶点属性在该位置插值后的结果等。

通常来说片元的数量大大多于原始顶点数，且高分辨率、overdraw 严重时会更多，所以高洛德着色通常更廉价。但并非绝对，低分辨率、高模、小三角形、大量实例化时，顶点数量也可能会更多。

&nbsp;

### 4. 像素处理阶段

接收来自光栅化阶段的片元序列，最终决定屏幕每个像素的颜色。通常包括片元着色器、测试与混合。

&nbsp;

**片元着色器**

接收插值后的片元数据，逐片元计算最终着色（如纹理采样、光照计算、阴影等）。片元之间互不影响，可高效并行执行。

&nbsp;

**测试与混合**

测试：包括深度测试（判断片元是否被前方物体遮挡）、模板测试（控制渲染区域）、Alpha 测试（透明度丢弃）等。未通过测试的片元将被直接丢弃。

混合 ：通过的片元，如果是半透明的，会将其颜色与帧缓存中已有的像素颜色按一定规则进行混合；如果是不透明的，则直接覆盖。

完成所有处理后，颜色值写入帧缓存，最终显示在屏幕上。

&nbsp;

## 5. GPU 与可编程管线

### 1. 固定管线

图形硬件最初以固定功能管线实现渲染：顶点变换，光照等计算由专用硬件逻辑执行，开发者只能调整矩阵、光源、材质等参数，表达范围受限。

2001年 GeForce 3 首次引入可编程顶点着色器，随后数年可编程着色器逐步支持流控制和高级语言的通用编程性。当今，渲染管线中的顶点处理、像素着色等已经完全可编程；而裁剪、三角形设置与遍历、屏幕映射，以及深度、模板测试、混合等难以程序化的操作仍由固定功能或可配置的专用硬件执行。

&nbsp;

### 2. SIMT

GPU 有数千个小型运算核心，并行度极高但每个核心的能力如分支预测、乱序执行等被大幅简化。

GPU 的执行模型为 SIMT（单指令多线程）。GPU 以 warp (NVIDIA, 32 线程) 或 wavefront/wave (AMD, 通常 32 或 64 线程) 为单位调度执行。

一个 warp 内的所有线程共享同一个程序计数器和取指/译码单元，在同一时刻只能执行同一条指令。所以当一个 warp 内部发生 `if else` 分歧时，如果两条路径都要执行，那么分歧路径会被串行地各执行一遍，每次只有对应的部分线程真正生效，其余线程空转。执行时间逻辑上是两条路径耗时之和，而非 CPU 中的 max。

尽管 2017 年后 Volta 架构引入了独立线程调度（ITS），`if` 和 `else` 可以交错执行，而不必严格串行。但无法让不同线程同一时刻执行不同指令这一根本限制没有改变，执行资源仍然按 warp 粒度分配，分歧路径降低效率。

&nbsp;

因此，在图形着色器中需要注意以下原则：

- 顶点、片元之间互相独立。着色器不能访问邻接顶点的数据，邻接信息要么提前烘焙进属性，要么放在 texture 中读取
- 分支昂贵。shader 优化中，常见手段就是减少分支、把分支改成无分支的算术（如 mix/step/掩码乘法），或让相邻数据尽量走同一路径。
- 注意数据规整。连续线程访问连续内存时,显存带宽利用率最高。

&nbsp;

## 6. 图形 API 

图形 API 是一套预定义的函数、数据结构、协议和工具的集合。它是连接应用程序和图形硬件的桥梁。

图形 API 规定了应用程序如何向 GPU 发出命令，以及 GPU 需要提供哪些功能。基于这份相同的协议， GPU 制造商和应用程序的开发者开发的产品就能兼容。如果没有图形 API，那么意味着开发者不得不为每一款特定的 GPU 编写机器码，成本巨大。

&nbsp;

### 1. 演进

**1. 固定功能 API**：渲染管线功能固定，开发者者只能通过 API 调整一些预设参数，但无法改变 GPU 实际执行的计算。如 OpenGL 1.x / Direct3D 7 (1993 - 2001)。

**2.隐式管理 API **：着色器可编程，开发者可以自定义出复杂的渲染效果。驱动负责分配内存、管理状态、翻译指令、校验错误、同步等诸多职责。这种隐式设计虽然让开发者使用简单，但使得驱动需要猜测应用意图，且只能进行单线程提交，应用侧难以充分利用多核。如 OpenGL 2-4 / D3D9 - 11 (2001 - 2015)。

**3. 显式控制 API**：将内存管理、多线程同步等责任交还给开发者，从而换取更低的 CPU 开销、更可预测的性能和更好的多核利用效果。如 DirectX12(2015), Vulkan (2016)。

&nbsp;

### 2. Vulkan 的显式控制

Vulkan 的显式控制和更老的图形 API 对比主要体现在以下层面：

1. 内存管理：从自动分配到手动绑定

   在 OpenGL 中，创建一个纹理，驱动会为其自动分配显存，开发者无需关心细节；

   在 Vulkan 中，首先需要查询设备支持的内存类型，如仅 GPU 可见的 `DEVICE_LOCAL` 或 CPU 可见的 `HOST_VISIBLE`，然后显式分配一大块原生内存。之后，还需要将这块内存显式地绑定到图像或者缓冲区上。

   这样的显式控制让开发者能够精确控制内存布局，实现内存别名（Aliasing）等高级优化，降低内存占用。

2. 同步机制：从隐式处理到手动同步

   OpenGL 会自动处理资源间的读写依赖，保证命令按正确顺序执行。

   在 Vulkan 中，同步主要是应用程序的责任，因为命令在 GPU 上的执行顺序默认是未定义的。Vulkan 中提供五种显式同步原语使得开发者能够精确控制：

   - Fences（栅栏）：用于 CPU 与 GPU 之间的同步，让 CPU 等待 GPU 完成工作；
   - Semaphores（信号量）：用于多个 GPU 队列之间的同步，控制不同队列的访问顺序；
   - Events（事件）：提供细粒度的同步，可在命令缓冲内外发出信号或等待，用于单个队列内的控制；
   - Pipeline Barriers（管线屏障）：在命令缓冲内部的特定点进行同步，主要用于控制同一队列内资源的读写依赖和图像布局转换；
   - Render Pass Objects（渲染通道对象）：提供内置的同步框架，可以更高效地表达附件（如颜色、深度缓冲）的依赖关系。

3. 状态管理：从动态修改到静态管线对象

   OpenGL 是一个巨大的状态机。可以随时修改混合、深度测试等状态，驱动会为此付出状态切换的开销。

   Vulkan 则采用静态管线对象的概念。在创建管线时，一次性、完整地指定所有渲染状态（着色器、顶点格式、混合模式等），之后这个管线对象就是不可变的。要改变状态，必须创建一个新的管线对象。虽然增加了前期的工作，但避免了运行时的状态切换开销，性能更可预测。

4. 命令提交：从即时执行到预录制与提交

   OpenGL 的 API 调用（如 `glDrawArrays`）会直接触发绘制，驱动在后台管理命令流。

   Vulkan 则采用预录制、后提交的模式，所有的绘制、内存传输等命令，都需要显式地录制到一个命令缓冲区中；录制完成后，需要在合适的时机将这个命令缓冲区显式地提交到一个队列，等待 GPU 执行。

   这种模式原生支持多线程，可以让多个线程并行地录制不同的命令缓冲区，从而充分利用多核 CPU。

5. 渲染通道与帧缓冲：显式定义与绑定

   在 OpenGL 中，可以直接将纹理附加到帧缓冲，其布局转换由驱动隐式管理。

   Vulkan 则要求先定义一个渲染通道，用它来声明本次渲染使用哪些附件、加载与存储行为以及子通道的依赖关系。然后创建帧缓冲对象。并将具体的图像视图显式地绑定到渲染通道定义的附件槽位上。

&nbsp;

### 3. Vulkan 对象模型

<img src = "https://raw.githubusercontent.com/Zwei-Reverberate/Blog-Images/refs/heads/master/Vulkan%20%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0/1.1.png">

`VkInstance` 进程级唯一，代表应用与 Vulkan 建立的会话。

Vulkan 中所有对象都有明确的生命周期归属关系，父对象销毁前必须先销毁子对象。

&nbsp;

## 7. 现代渲染引擎架构分层

一个典型的引擎分层：

<img src = "https://raw.githubusercontent.com/Zwei-Reverberate/Blog-Images/refs/heads/master/Vulkan%20%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0/1.2.png"  height = 600>

在 vwzrender 中，也会采用此种分层方式。

&nbsp;

# 2. 变换

在 vwzrender 中，使用 glm 进行向量和矩阵运算。

&nbsp;

## 1. 向量

向量是既有大小又有方向的量。在坐标定义中，向量表示为一组有序实数。如图形学中最常使用的三维向量 $\mathbf{v} = (x, y , z)$。glm 中使用 3 个 `float` 表示：

```c++
const glm::vec3 a(1.0f, 2.0f, 3.0f);
```

&nbsp;

**长度与归一化**

向量的长度，亦称为向量的模：$|\mathbf{v}| = \sqrt{x^2 + y^2 + z^2}$。

把一个向量除以自身长度，得到同方向、长度为 1 的单位向量，此操作名为归一化：$\hat{\mathbf{v}} = \frac{\mathbf{v}}{|\mathbf{v}|}$。

&nbsp;

**点积**

设向量 $\mathbf{a} = (a_x, a_y, a_z)$，$\mathbf{b} = (b_x, b_y, b_z)$，则它们的点积为：
$$
\mathbf{a} \cdot \mathbf{b} = a_x b_x + a_y b_y + a_z b_z = |\mathbf{a}| |\mathbf{b}| \cos\theta
$$
两单位向量的点乘为两向量夹角的余弦，值范围在 $[-1, 1]$，描述两单位向量的接近程度。

&nbsp;

**叉积**

向量叉积通常只在三维欧氏空间中讨论，设向量 $\mathbf{a} = (a_x, a_y, a_y)$，$\mathbf{b} = (b_x, b_y, b_y)$，则它们的叉积为：
$$
\mathbf{a} \times \mathbf{b} = \begin{pmatrix} a_y b_z - a_z b_y \\ a_z b_x - a_x b_z \\ a_x b_y - a_y b_x \end{pmatrix}
$$
叉积结果仍为三维向量，同时有以下性质：

- $\mathbf{a} \times \mathbf{b}$ 同时垂直于 $\mathbf{a}$ 和 $\mathbf{b}$
- $\mathbf{a} \times \mathbf{b} = -(\mathbf{b} \times \mathbf{a})$
- $|\mathbf{a} \times \mathbf{b}| = |\mathbf{a}| |\mathbf{b}| \sin\theta$，其中 $θ∈[0,π]$ 是两向量夹角
- $\mathbf{a} \times \mathbf{a} = \mathbf{0}$

&nbsp;

叉积是赝向量；它的轴向量表示和方向定则依赖空间定向。右手系和左手系结果不同。

&nbsp;

## 2. 矩阵

图形学中，矩阵是变换的数学表示。

可以通俗理解为输入一个向量，经过矩阵变换后输出一个向量。本文约定使用列向量，变换写为左乘：
$$
\mathbf{v}' = \mathbf{M}\mathbf{v}
$$
&nbsp;

## 3. 齐次坐标

齐次坐标是不同于欧式坐标的坐标表示法，它在欧式坐标的基础上再增广一个 $w$ 维度。本质上是把欧氏空间嵌入射影空间，从而统一表示点、方向、无穷远点以及透视投影。

对于三维欧式坐标 $(x, y, z)$，其齐次坐标为 $(x, y, z, w)$：

- $w = 0$ 时，该坐标表示方向，或无穷远点，不受平移影响
- $w = 1$ 时，该坐标表示点，受平移影响
- $ w \neq 0$ 时，可归一化为 $(\frac{x}{w}, \frac{y}{w}, \frac{z}{w}, 1)$

>齐次坐标的数学来源是射影几何。欧氏空间中两条平行线没有交点；在射影平面中，它们交于一个无穷远点。

&nbsp;

**齐次坐标的重要意义在于能够使用矩阵乘法统一处理仿射变换和透视投影**。

&nbsp;

### 1. 线性变换

线性变换是保持向量空间结构的变换，保持向量加法和数乘运算，对于任意向量 $\mathbf{u}$ 和 $\mathbf{v}$，以及线性变换 $\mathbf{A}$，有：
$$
A(\mathbf{u} + \mathbf{v}) = A(\mathbf{u}) + A(\mathbf{v})
$$

$$
A(c \cdot \mathbf{u}) = c \cdot A(\mathbf{u})
$$

线性变换的几何特征的原点不动。缩放、旋转、反射等是线性变换。这些变换可以使用矩阵乘法来表示。

>原点不动是指在线性变换下，向量空间的原点被映射到它自己，即 $\mathbf{A} \mathbf{0} = \mathbf{0}$；例如设平移变换为 $\mathbf{T(x)} = \mathbf{x} + \mathbf{b}$，则 $\mathbf{T(0)} = \mathbf{b} \neq \mathbf{0}$，显然也不满足加法封闭和数乘封闭性，所以平移不是线性变换。

 &nbsp;

### 2. 仿射变换

仿射变换是一类包括线性变换和平移变换的变换。仿射变换可以用以下形式表示：
$$
\mathbf{v}' = A\mathbf{v} + \mathbf{T}
$$
其中，$A$ 是线性变换矩阵，$\mathbf{T}$ 是平移向量。仿射变换在几何意义上包括了缩放、旋转、平移和剪切变换。

为了使得所有仿射变换都可以用矩阵乘法来统一处理，将 $\mathbf{v}$ 升维成齐次坐标 $(x, y, z, 1)^T$，仿射变换可以表示为 4x4 矩阵：
$$
M = \begin{pmatrix}
A & \mathbf{T} \\
\mathbf{0}^T & 1
\end{pmatrix}
$$
应用该矩阵到齐次坐标上：
$$
\begin{pmatrix}
A & \mathbf{T} \\
\mathbf{0}^T & 1
\end{pmatrix}
\begin{pmatrix}
x \\
y \\
z \\
1
\end{pmatrix}
=
\begin{pmatrix}
A \begin{pmatrix}
x \\
y \\
z
\end{pmatrix} + \mathbf{T} \\
1
\end{pmatrix}
$$
&nbsp;

## 4. 模型变换

建模时常常以自身原点为中心，为局部坐标。模型变换是将对象从其本地坐标系转换到世界坐标系的过程。

变换主要包括平移、旋转和缩放。

&nbsp;

###  1. 缩放

缩放变换是通过改变对象的大小来实现的。对于一个三维点 $(x, y, z)$ ，如果要在 $x$, $y$, $z$ 方向分别缩放 $s_x$、$s_y$、$s_z$ 倍，则其缩放矩阵 $\mathbf{S}$ 为：
$$
\mathbf{S} = \begin{pmatrix}
s_x & 0 & 0 & 0 \\
0 & s_y & 0 & 0 \\
0 & 0 & s_z & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
应用该缩放矩阵后，点 $(x, y, z, 1)^T$变为：
$$
\begin{pmatrix}
s_x & 0 & 0 & 0 \\
0 & s_y & 0 & 0 \\
0 & 0 & s_z & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
\begin{pmatrix}
x \\
y \\
z \\
1
\end{pmatrix}
=
\begin{pmatrix}
s_x \cdot x \\
s_y \cdot y \\
s_z \cdot z \\
1
\end{pmatrix}
$$
&nbsp;

### 2. 旋转

先考虑二维旋转，三维旋转是二维旋转的推广。

二维旋转矩阵用于表示平面内的旋转变换。假设我们要将一个点 $(x, y)$  绕原点逆时针旋转一个角度 $\theta$。为了推导旋转矩阵，我们可以使用基本的三角函数关系。

考虑一个点 $P(x, y)$  在平面上绕原点逆时针旋转 $\theta$ 后的新坐标 $P'(x', y')$。根据旋转的定义和三角函数关系，可以写出以下方程：
$$
x' = x \cos\theta - y \sin\theta \\
y' = x \sin\theta + y \cos\theta
$$

----

**推导**

设原始点 $P(x, y)$  绕原点 $O$  旋转角度 $\theta$ 后到达新点 $P'(x', y')$。

设点 $P(x, y)$ 的极坐标表示为 $(r, \phi)$，其中 $r = \sqrt{x^2 + y^2}$ 是点 $P$ 到原点的距离，$\phi$ 是点 $P$ 与 $x$ 轴正方向的夹角。

则旋转角度 $\theta$ 后，新点 $P'$ 的极坐标表示为 $(r, \phi + \theta)$

使用极坐标与直角坐标的转换关系，有：
$$
x' = r \cos(\phi + \theta) \\
y' = r \sin(\phi + \theta)
$$
展开，得：
$$
x' = x \cos\theta - y \sin\theta \\
y' = x \sin\theta + y \cos\theta
$$

----

写成矩阵乘法形式：
$$
\begin{pmatrix}
x' \\
y'
\end{pmatrix}
=
\begin{pmatrix}
\cos\theta & -\sin\theta \\
\sin\theta & \cos\theta
\end{pmatrix}
\begin{pmatrix}
x \\
y
\end{pmatrix}
$$
二维旋转矩阵 $\mathbf{R(\theta)}$ 即：
$$
\mathbf{R(\theta)} = \begin{pmatrix}
\cos\theta & -\sin\theta \\
\sin\theta & \cos\theta
\end{pmatrix}
$$
&nbsp;

三维旋转变换是通过绕某个轴旋转一个角度来实现的，分别推导绕 $x$ 轴、$y$ 轴、 $z$ 轴和任意轴的旋转矩阵。

&nbsp;

**绕 $x$ 轴旋转**

对于绕 $x$ 轴旋转一个角度 $\theta$，旋转矩阵 $\mathbf{R_x}$ 为：
$$
R_x = \begin{pmatrix}
1 & 0 & 0 & 0 \\
0 & \cos\theta & -\sin\theta & 0 \\
0 & \sin\theta & \cos\theta & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
应用该旋转矩阵后，点 $(x, y, z, 1)^T$ 变换为：
$$
\begin{pmatrix}
1 & 0 & 0 & 0 \\
0 & \cos\theta & -\sin\theta & 0 \\
0 & \sin\theta & \cos\theta & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
\begin{pmatrix}
x \\
y \\
z \\
1
\end{pmatrix} 
= \begin{pmatrix}
x \\
y \cdot \cos\theta - z \cdot \sin\theta \\
y \cdot \sin\theta + z \cdot \cos\theta \\
1
\end{pmatrix}
$$
&nbsp;

**绕 $y$ 轴旋转**

对于绕 $y$ 轴旋转一个角度 $\theta$，旋转矩阵 $\mathbf{R_y}$ 为：
$$
\mathbf{R_y} = \begin{pmatrix}
\cos\theta & 0 & \sin\theta & 0 \\
0 & 1 & 0 & 0 \\
-\sin\theta & 0 & \cos\theta & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
应用该旋转矩阵后，点 $(x, y, z, 1)^T$ 变换为：
$$
\begin{pmatrix}
\cos\theta & 0 & \sin\theta & 0 \\
0 & 1 & 0 & 0 \\
-\sin\theta & 0 & \cos\theta & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
\begin{pmatrix}
x \\
y \\
z \\
1
\end{pmatrix}
=
\begin{pmatrix}
x \cdot \cos\theta + z \cdot \sin\theta \\
y \\
-x \cdot \sin\theta + z \cdot \cos\theta \\
1
\end{pmatrix}
$$
&nbsp;

**绕 $z$ 轴旋转**

对于绕 $z$ 轴旋转一个角度 $\theta$，旋转矩阵 $\mathbf{R_z}$ 为：
$$
\mathbf{R_z} = \begin{pmatrix}
\cos\theta & -\sin\theta & 0 & 0 \\
\sin\theta & \cos\theta & 0 & 0 \\
0 & 0 & 1 & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
应用该旋转矩阵后，点 $(x, y, z, 1)^T$ 变为：
$$
\begin{pmatrix}
\cos\theta & -\sin\theta & 0 & 0 \\
\sin\theta & \cos\theta & 0 & 0 \\
0 & 0 & 1 & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
\begin{pmatrix}
x \\
y \\
z \\
1
\end{pmatrix}=
\begin{pmatrix}
x \cdot \cos\theta - y \cdot \sin\theta \\
x \cdot \sin\theta + y \cdot \cos\theta \\
z \\
1
\end{pmatrix}
$$
&nbsp;

**绕任意轴旋转**

要研究的问题为：绕经过原点且方向为单位向量 $\mathbf{u} = (u_x, u_y, u_z)$ 的轴旋转一个角度 $\theta$，计算出其变换矩阵 $\mathbf{R}$。

>如果旋转轴不经过原点，设 $A$ 为旋转轴上一点，则可先将 $A$ 平移到原点，其平移矩阵为 $\mathbf{T}(-A)$，再进行旋转，最后平移回来，即变换矩阵为：$\mathbf{H} = \mathbf{T}(A) · \mathbf{R} ·\mathbf{T}(-A)$。

对于 $\mathbf{R}$ 有两种计算方案，一种是将其分解为基本的三轴旋转，二是直接使用罗德里格斯公式。后者计算量更小。

----

**三轴旋转**

首先将旋转轴对齐到某一个坐标轴，这里以 $z$ 轴为例。而将旋转轴 $\mathbf{u}$ 对齐 $z$ 轴，可以通过两个旋转实现：

- 先绕 $y$ 轴旋转角度 $\beta$，使得旋转轴落在 $xz$ 平面上，$\beta = \arctan\left(\frac{u_y}{u_x}\right)$，旋转矩阵为：
  $$
  \mathbf{R}_y(\beta) = \begin{pmatrix}
  \cos\beta & 0 & \sin\beta \\
  0 & 1 & 0 \\
  -\sin\beta & 0 & \cos\beta
  \end{pmatrix}
  $$

- 再绕 $x$ 轴旋转角度 $\alpha$，使得旋转轴与 $z$ 轴对齐，$\alpha = \arctan\left(\frac{u_x}{u_z}\right)$，旋转矩阵为：
  $$
  \mathbf{R}_x(\alpha) = \begin{pmatrix}
      1 & 0 & 0 \\
      0 & \cos\alpha & -\sin\alpha \\
      0 & \sin\alpha & \cos\alpha
      \end{pmatrix}
  $$

组合上述两步旋转，得到将旋转轴对齐 $z$ 轴的矩阵：$\mathbf{R}_{align} = \mathbf{R}_x(\alpha) \mathbf{R}_y(\beta)$。

现在绕 $z$ 轴旋转角度 $\theta$，旋转矩阵为：
$$
\mathbf{R}_z(\theta) = \begin{pmatrix}
\cos\theta & -\sin\theta & 0 \\
\sin\theta & \cos\theta & 0 \\
0 & 0 & 1
\end{pmatrix}
$$
最后，为了恢复旋转轴到原始位置，需要逆转之前的对齐操作，通过应用逆矩阵来实现。最终的结果为：
$$
\mathbf{R} = \mathbf{R}_y(-\beta) \mathbf{R}_x(-\alpha) \mathbf{R}_z(\theta) \mathbf{R}_x(\alpha) \mathbf{R}_y(\beta)
$$

----

**罗德里格斯公式**

罗德里格斯公式是一种直接计算绕任意轴旋转的旋转矩阵的方法。公式如下：
$$
\mathbf{R(u, \theta)} = \mathbf{I} + \sin\theta \mathbf{K} + (1 - \cos\theta) \mathbf{K}^2
$$
其中 $\mathbf{I}$ 是 3 x 3 单位矩阵。$\mathbf{K}$ 是根据旋转轴 $\mathbf{u} = (u_x, u_y, u_z)$ 构造的反对称矩阵：
$$
\mathbf{K} = \begin{pmatrix}
    0 & -u_z & u_y \\
    u_z & 0 & -u_x \\
    -u_y & u_x & 0
    \end{pmatrix}
$$

带入公式，可直接得到旋转矩阵：
$$
\mathbf{R} = \begin{pmatrix}
 \cos\theta + u_x^2(1 - \cos\theta) & u_x u_y (1 - \cos\theta) - u_z \sin\theta & u_x u_z (1 - \cos\theta) + u_y \sin\theta \\
 u_y u_x (1 - \cos\theta) + u_z \sin\theta & \cos\theta + u_y^2 (1 - \cos\theta) & u_y u_z (1 - \cos\theta) - u_x \sin\theta \\
 u_z u_x (1 - \cos\theta) - u_y \sin\theta & u_z u_y (1 - \cos\theta) + u_x \sin\theta & \cos\theta + u_z^2 (1 - \cos\theta)
 \end{pmatrix}
$$

----

### 3. 平移

平移变换是通过移动对象的位置来实现的。对于一个三维点 $(x, y, z)$，如果要在 $x、y、z$ 方向分别平移 $t_x$、$t_y$、$t_z$，则其平移矩阵 $\mathbf{T}$ 为：
$$
\mathbf{T} = \begin{pmatrix}
1 & 0 & 0 & t_x \\
0 & 1 & 0 & t_y \\
0 & 0 & 1 & t_z \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
应用该平移矩阵后，点 $(x, y, z, 1)^T$ 变为：
$$
\begin{pmatrix}
1 & 0 & 0 & t_x \\
0 & 1 & 0 & t_y \\
0 & 0 & 1 & t_z \\
0 & 0 & 0 & 1
\end{pmatrix}
\begin{pmatrix}
x \\
y \\
z \\
1
\end{pmatrix}
=
\begin{pmatrix}
x + t_x \\
y + t_y \\
z + t_z \\
1
\end{pmatrix}
$$
&nbsp;

&nbsp;

常见的模型变换通常按照缩放、旋转、平移的顺序进行：$\mathbf{M} = \mathbf{T} \cdot \mathbf{R} \cdot \mathbf{S}$（列向量与左乘约定），但并非绝对，具体的顺序取决于实际的应用场景。

&nbsp;

## 5. 视图变换

视图变换是将顶点从世界坐标系转换到观察坐标系（视图坐标系）的过程。由一次平移变换和一次旋转变换组成。

&nbsp;

**平移变换**

假设相机的位置在世界坐标系中为 $C = (c_x, c_y, c_z)$。需要的平移变换矩阵 $\mathbf{T}$ 将世界坐标系原点平移到相机位置，使相机位置成为新的原点：
$$
\mathbf{T} = \begin{pmatrix}
1 & 0 & 0 & -c_x \\
0 & 1 & 0 & -c_y \\
0 & 0 & 1 & -c_z \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
&nbsp;

**旋转变换**

将世界坐标系旋转到相机方向，使相机方向与视图坐标系的轴对齐。

此处采用右手系的视图坐标系约定：$+x$ 轴对应相机的右方向，$+y$ 轴对应相机的上方向，相机看向 $-z$ 轴。

相机的朝向由三个相互正交的单位向量决定：

- 视线方向（forward）$\mathbf{f}$：
  $$
  \mathbf{f} = \frac{C_{target} - C}{\|C_{target} - C\|}
  $$
  其中 $C_{target}$ 是相机看向的目标点。

- 相机右方向（right）$\mathbf{r}$：
  $$
  \mathbf{r} = \frac{\mathbf{f} \times \mathbf{u_{world}}}{\|\mathbf{f} \times \mathbf{u_{world}}\|}
  $$
  其中 $\mathbf{u_{world}}$ 是世界坐标系的上方向（通常取 $(0, 1, 0)$）。

- 相机上方向（up）$\mathbf{u}$：
  $$
  \mathbf{u} = \mathbf{r} \times \mathbf{f}
  $$

> 另一种等价写法是取 $\mathbf{g} = C - C_{target}$（指向相机后方），此时配套公式为 $\mathbf{r} = \mathbf{u_{world}} \times \mathbf{g}$、$\mathbf{u} = \mathbf{g} \times \mathbf{r}$，旋转矩阵的第三行直接放 $\mathbf{g}$，两种写法得到的矩阵完全一致。
>
> 但两套公式不能混用。例如 $\mathbf{f}$ 指向目标时，若 $\mathbf{r}$ 误按 $\mathbf{u_{world}} \times \mathbf{f}$ 计算，得到的是相机的左方向；再加上第三行放的是 $\mathbf{f}$ 而非 $-\mathbf{f}$，相机看向了 $+z$——整个视图坐标系相比正确结果差一个绕 $y$ 轴的 $180^\circ$ 旋转，与后续投影变换的约定不再匹配，渲染结果会前后、左右颠倒。此外，DirectX 的传统左手约定中相机看向 $+z$，基向量的计算顺序也与此处不同，不能直接照搬。
>
> $\mathbf{f}$ 与 $\mathbf{u_{world}}$ 不能共线，否则叉乘得到零向量无法归一化，即相机不能正好垂直俯视或仰视，遇到这类朝向时需要另行指定一个不共线的参考上方向。

----

**推导**

所求的旋转矩阵 $\mathbf{R}$ 应把相机的三个朝向向量分别映射到视图坐标系的对应坐标轴上，所以 $\mathbf{R}$ 需满足：
$$
\mathbf{R} ·\mathbf{r} = \begin{pmatrix} 1 \\ 0 \\ 0 \end{pmatrix}, \quad
\mathbf{R} · \mathbf{u} = \begin{pmatrix} 0 \\ 1 \\ 0 \end{pmatrix}, \quad
\mathbf{R}· (-\mathbf{f}) = \begin{pmatrix} 0 \\ 0 \\ 1 \end{pmatrix}
$$
将 $\mathbf{r}$、$\mathbf{u}$、$-\mathbf{f}$ 按行排成矩阵。

例如当该矩阵乘以 $\mathbf{r}$ 时，三个分量分别为各行与 $\mathbf{r}$ 的点乘，由三个向量的单位正交性：
$$
\begin{pmatrix}
\mathbf{r} \cdot \mathbf{r} \\
\mathbf{u} \cdot \mathbf{r} \\
(-\mathbf{f}) \cdot \mathbf{r}
\end{pmatrix}
=
\begin{pmatrix}
1 \\
0 \\
0
\end{pmatrix}
$$
对 $\mathbf{u}$ 与 $-\mathbf{f}$ 同理。

因此将三个基向量按行排列（写成齐次坐标下的 4 x 4 形式）即得旋转矩阵：
$$
\mathbf{R} = \begin{pmatrix}
r_x & r_y & r_z & 0 \\
u_x & u_y & u_z & 0 \\
-f_x & -f_y & -f_z & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
$$

----

视图变换矩阵 $\mathbf{V}$ 是先应用平移、再应用旋转的组合（列向量与左乘约定）：
$$
\mathbf{V} = \mathbf{R} \cdot \mathbf{T}
$$
即：
$$
\mathbf{V} = \begin{pmatrix}
r_x & r_y & r_z & 0 \\
u_x & u_y & u_z & 0 \\
-f_x & -f_y & -f_z & 0 \\
0 & 0 & 0 & 1
\end{pmatrix}
\begin{pmatrix}
1 & 0 & 0 & -c_x \\
0 & 1 & 0 & -c_y \\
0 & 0 & 1 & -c_z \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
展开得：
$$
\mathbf{V} = \begin{pmatrix}
r_x & r_y & r_z & -(r_x c_x + r_y c_y + r_z c_z) \\
u_x & u_y & u_z & -(u_x c_x + u_y c_y + u_z c_z) \\
-f_x & -f_y & -f_z & f_x c_x + f_y c_y + f_z c_z \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
&nbsp;

## 6. 投影变换

投影变换是将顶点从视图空间转换到齐次裁剪空间的过程，有正交投影和透视投影两种。

&nbsp;

### 1. 裁剪空间

上文已述及，投影变换后的顶点位于齐次裁剪空间 $(x, y, z, w)$，只有各分量满足 $-w \le x, y, z \le w$ 的顶点才位于可视范围内，超出的部分会在透视除法之前被裁剪掉。

对透视投影而言，$w$ 分量保留了视图空间中的深度信息（即到相机的远近），这正是透视除法与深度测试的基础；正交投影下 $w$ 恒为 1。

&nbsp;

### 2. 视锥体

视锥体是投影变换中相机的可视范围，投影变换的本质就是把视锥体内的顶点映射到裁剪空间中。正交投影的视锥体是一个长方体，透视投影的视锥体是一个四棱台。

沿用视图变换一节的约定：视图空间为右手系，相机看向 $-z$。定义视锥体的参数为：

- 左右上下边界 $l$、$r$、$b$、$t$：均为近平面上的坐标
- 近平面与远平面距离 $n$、$f$（$0 < n < f$）：近平面位于 $z = -n$，远平面位于 $z = -f$

> 一些资料采用相机看向 $+z$、近平面位于 $z = +n$ 的约定（DirectX 的传统左手约定即如此），推导出的投影矩阵符号与本节不同。本节的视图变换让相机看向 $-z$，因此投影也必须使用这里的一套约定，二者不能混用。

对于透视投影的视锥体，也可以使用垂直视场角 $fovY$ 与纵横比 $Aspect \ Ratio$ 来定义，它们与边界的关系为：
$$
t = n \tan{\frac{fovY}{2}} \ \ \ \ \ \ r = t \cdot aspect
$$

<img src = "https://raw.githubusercontent.com/Zwei-Reverberate/Blog-Images/refs/heads/master/Vulkan%20%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0/2.1.jpg">

&nbsp;

### 3. 正交投影

正交投影保持平行线与比例，对象不会因远近而改变大小。其视锥体是长方体 $[l, r] \times [b, t] \times [-f, -n]$。

为了方便说明，先忽略裁剪空间的 $w$ 分量，将其视作范围在 $[-1, 1]$ 的标准立方体。为了将长方体内的顶点 $(x, y, z)$（$-f \le z \le -n$）映射到标准立方体中，需要两步操作：

1. 平移：将长方体的中心平移到原点

   长方体的中心在三个方向上分别为 $\frac{r+l}{2}$、$\frac{t+b}{2}$、$-\frac{n+f}{2}$，所以平移矩阵为：
   $$
   \mathbf{T} = \begin{pmatrix}
   1 & 0 & 0 & -\frac{r+l}{2} \\
   0 & 1 & 0 & -\frac{t+b}{2} \\
   0 & 0 & 1 & \frac{n+f}{2} \\
   0 & 0 & 0 & 1
   \end{pmatrix}
   $$

2. 缩放：将三个方向的范围伸缩到 $[-1, 1]$
   $$
   \mathbf{S} = \begin{pmatrix}
   \frac{2}{r-l} & 0 & 0 & 0 \\
   0 & \frac{2}{t-b} & 0 & 0 \\
   0 & 0 & \frac{2}{n-f} & 0 \\
   0 & 0 & 0 & 1
   \end{pmatrix}
   $$

   以左右边界为例：视锥体中左右范围长度为 $r-l$，标准立方体则为 $1-(-1)=2$，所以伸缩因子为 $\frac{2}{r-l}$。注意 $z$ 方向的伸缩因子 $\frac{2}{n-f}$ 是负数，因为 $z$ 值为负且远平面（$z=-f$）比近平面（$z=-n$）离原点更远；这个负号同时翻转了 $z$ 轴，使近平面映射到 $-1$、远平面映射到 $+1$。

将二者组合起来，正交投影矩阵为：
$$
\mathbf{P}_{ortho} = \mathbf{S} \cdot \mathbf{T} = \begin{pmatrix}
\frac{2}{r-l} & 0 & 0 & -\frac{r+l}{r-l} \\
0 & \frac{2}{t-b} & 0 & -\frac{t+b}{t-b} \\
0 & 0 & \frac{2}{n-f} & \frac{n+f}{n-f} \\
0 & 0 & 0 & 1
\end{pmatrix}
$$

由于正交投影矩阵的第四行为 $(0, 0, 0, 1)$，变换后的 $w$ 恒为 1，不存在透视除法的问题——正交投影得到的裁剪坐标就是 NDC 坐标。

&nbsp;

### 4. 透视投影

透视投影根据物体距离观察点的远近改变其大小，近大远小，模拟人眼的视觉效果。

我们的思路是先将四棱台挤压成一个长方体，再应用上面的正交投影矩阵。挤压时保持以下事实成立：

- 近平面上的点坐标不变
- 远平面上的点 $z$ 值不变
- 远平面中心点 $(0, 0, -f)$ 坐标不变

&nbsp;

**挤压矩阵**

<img src = "https://raw.githubusercontent.com/Zwei-Reverberate/Blog-Images/refs/heads/master/Vulkan%20%E5%AD%A6%E4%B9%A0%E7%AC%94%E8%AE%B0/2.2.jpg">

设视锥体内的点 $C(x, y, z)$（$z < 0$）。从相机（原点）出发经过 $C$ 的射线与近平面 $z = -n$ 交于 $T(x', y', -n)$，由相似三角形：
$$
\frac{x'}{x} = \frac{n}{-z} \ \ \ \ \ \ \frac{y'}{y} = \frac{n}{-z}
$$
挤压后 $C$ 的 $x, y$ 坐标与 $T$ 相同，即挤压后的点为 $S(\frac{nx}{-z}, \frac{ny}{-z}, z_0)$。利用齐次坐标 $(x, y, z, w)$ 与 $(xw, yw, zw, w)$ 表示同一个点的性质，给 $S$ 整体乘上 $-z$（这个 $-z$ 承担了 $w$ 的角色，透视除法时除以它即可还原）：
$$
S = \begin{pmatrix}
\frac{nx}{-z} \\
\frac{ny}{-z} \\
z_0 \\
1
\end{pmatrix}
=
\begin{pmatrix}
nx \\
ny \\
-z_0 z \\
-z
\end{pmatrix}
$$
$x, y, w$ 三个分量的形式都是已知的，可以直接写出挤压矩阵的第一、二、四行；第三行暂用未知数 $A, B$ 占位：
$$
\mathbf{M} = \begin{pmatrix}
n & 0 & 0 & 0 \\
0 & n & 0 & 0 \\
0 & 0 & A & B \\
0 & 0 & -1 & 0
\end{pmatrix}
$$
利用两个约束确定 $A, B$：

- 近平面上的点挤压后不变：$z = -n$ 时 $z_0 = -n$，第三分量 $-z_0 z = -n^2$，得
  $$
  -An + B = -n^2
  $$

- 远平面上的点 $z$ 值不变：$z = -f$ 时 $z_0 = -f$，第三分量 $-z_0 z = -f^2$，得
  $$
  -Af + B = -f^2
  $$

两式相减解得 $A = n+f$，回代得 $B = nf$。于是挤压矩阵为：
$$
\mathbf{M} = \begin{pmatrix}
n & 0 & 0 & 0 \\
0 & n & 0 & 0 \\
0 & 0 & n+f & nf \\
0 & 0 & -1 & 0
\end{pmatrix}
$$

> 可以验证：近平面上的点 $(x, y, -n, 1)$ 经 $\mathbf{M}$ 变换为 $(nx, ny, -n^2, n)$，除以 $w$ 后仍为 $(x, y, -n)$，坐标不变；远平面中心点 $(0, 0, -f, 1)$ 变换为 $(0, 0, -f^2, f)$，除以 $w$ 后仍为 $(0, 0, -f)$。

最终的透视投影矩阵是挤压矩阵再复合正交投影矩阵：
$$
\mathbf{P}_{persp} = \mathbf{P}_{ortho} \cdot \mathbf{M} =
\begin{pmatrix}
\frac{2}{r-l} & 0 & 0 & -\frac{r+l}{r-l} \\
0 & \frac{2}{t-b} & 0 & -\frac{t+b}{t-b} \\
0 & 0 & \frac{2}{n-f} & \frac{n+f}{n-f} \\
0 & 0 & 0 & 1
\end{pmatrix}
\begin{pmatrix}
n & 0 & 0 & 0 \\
0 & n & 0 & 0 \\
0 & 0 & n+f & nf \\
0 & 0 & -1 & 0
\end{pmatrix}
= \begin{pmatrix}
\frac{2n}{r-l} & 0 & \frac{r+l}{r-l} & 0 \\
0 & \frac{2n}{t-b} & \frac{t+b}{t-b} & 0 \\
0 & 0 & \frac{n+f}{n-f} & \frac{2nf}{n-f} \\
0 & 0 & -1 & 0
\end{pmatrix}
$$
对于左右上下对称（$l=-r$，$b=-t$）的视锥体，代入 $t = n\tan\frac{fovY}{2}$、$r = t \cdot aspect$ 后可以写成使用 $fovY$ 与纵横比的形式：
$$
\mathbf{P}_{persp} = \begin{pmatrix}
\frac{1}{aspect \cdot \tan\frac{fovY}{2}} & 0 & 0 & 0 \\
0 & \frac{1}{\tan\frac{fovY}{2}} & 0 & 0 \\
0 & 0 & \frac{n+f}{n-f} & \frac{2nf}{n-f} \\
0 & 0 & -1 & 0
\end{pmatrix}
$$

注意得到的是范围在 $[-w, w]$ 的裁剪坐标而非 NDC，透视除法后才得到 NDC：$w = -z$ 保留了视图空间中的深度信息。$z$ 方向上近平面映射到 $-1$、远平面映射到 $+1$，这是 OpenGL 风格的约定。

Vulkan 的 NDC 中 $z$ 范围为 $[0, 1]$ 且 $y$ 轴向下，通常的做法仍是先构造上述矩阵，再复合一个 $z$ 重映射与 $y$ 翻转的修正矩阵：
$$
\mathbf{C} = \begin{pmatrix}
1 & 0 & 0 & 0 \\
0 & -1 & 0 & 0 \\
0 & 0 & \frac{1}{2} & \frac{1}{2} \\
0 & 0 & 0 & 1
\end{pmatrix}
\ \ \ \ \ \mathbf{P}_{vk} = \mathbf{C} \cdot \mathbf{P}_{persp}
$$
$\mathbf{C}$ 直接作用在裁剪坐标上：第二行翻转 $y$；第三行使 $z_c' = \frac{z_c + w_c}{2}$，于是透视除法后 $z_{ndc}' = \frac{z_c/w_c + 1}{2} = \frac{z_{ndc}+1}{2}$，恰好把 $[-1,1]$ 重映射到 $[0,1]$；$w$ 保持不变，不影响裁剪与透视除法的其余部分。

顺带地，近平面与远平面的裁剪条件 $-w_c \le z_c \le w_c$ 在该变换后恰好变为 $0 \le z_c' \le w_c$，与 Vulkan 对 $z$ 的裁剪约定一致。若改用视口变换中的负视口高度翻转 $y$，则 $\mathbf{C}$ 的第二行改为恒等，只保留 $z$ 的重映射。

&nbsp;

## 7. 透视除法

透视除法位于渲染管线中裁剪之后、屏幕映射之前。投影变换得到的只是裁剪空间坐标，透视除法才是投影的最后一步。

&nbsp;

透视投影矩阵的第四行 $(0, 0, -1, 0)$，它把视图空间的深度放进了 $w$。在本章的约定下 $w = -z$，相机前方的点 $z < 0$，因此 $w > 0$。透视除法将裁剪坐标的四个分量全部除以 $w$：
$$
\begin{pmatrix}
x_c \\
y_c \\
z_c \\
w_c
\end{pmatrix}
\rightarrow
\begin{pmatrix}
x_c / w_c \\
y_c / w_c \\
z_c / w_c \\
1
\end{pmatrix}
$$
得到归一化设备坐标（NDC），此时顶点位于范围 $[-1, 1]$ 的标准立方体中。透视除法正是透视投影推导中“给 $S$ 整体乘上 $-z$”的逆操作——先整体乘上去，透视除法再整体除回来。

正交投影矩阵的第四行为 $(0, 0, 0, 1)$，$w$ 恒为 1，透视除法是恒等操作，正交投影的裁剪坐标就是 NDC 坐标。

&nbsp;

**透视校正插值**

实现上，透视除法后原始的 $w$ 会被保留下来，因为光栅化阶段的属性插值需要它。

透视投影近大远小，屏幕上的等距分段并不对应三维空间中的等距分段：设三角形两顶点处的某属性（如纹理坐标）为 $a_0$、$a_1$，直接在屏幕空间对它们线性插值，得到的不等于该属性在三维空间中的线性取值——经典症状是贴在地面上的条纹纹理中远处被压弯。

正确做法是对 $\frac{a}{w}$ 与 $\frac{1}{w}$ 分别在屏幕空间做线性插值，再相除恢复，这称为透视校正插值。

&nbsp;

>裁剪发生在齐次裁剪空间而非 NDC 中，有两个原因：
>
>第一，相机背后的点 $z > 0$，故 $w = -z < 0$。如果先做透视除法，$x_c$ 与 $w_c$ 同时为负时其商为正，$\left| x_c / w_c \right| \le 1$ 仍可能成立——相机背后的点会伪装成 NDC 立方体内的点。而在裁剪空间中判断 $\left| x_c \right| \le w_c$ 时，$w \le 0$ 的点天然不满足条件，直接被排除。
>
>第二，在裁剪空间中，$-w \le x \le w$ 等价于 $x + w \ge 0$、$x - w \le 0$ 等线性约束，它们恰好对应视锥体的六个边界面。在齐次坐标上裁剪时，图元与边界的交点可以直接线性插值求出；若先除以 $w$ 再裁剪，插值与交点计算都会出错。

&nbsp;

**深度的非线性**

将透视投影矩阵的第三行代入透视除法（设 $d = -z$ 为点到相机的距离）：
$$
z_{ndc} = \frac{z_c}{w_c} = \frac{\frac{(n+f)d - 2fn}{f-n}}{d} = \frac{(n+f)d - 2nf}{(f-n)d}
$$
这是一个关于 $d$ 的双曲线函数而非线性函数。

以 $n = 1$、$f = 101$ 为例：

- $d = 1$ 时 $z_{ndc} = -1$；$d = 2$ 时 $z_{ndc} = 0.01$；$d = 101$ 时 $z_{ndc} = 1$；

- $[-1, 0.01]$ 这一半的深度范围被耗费在 $[1, 2]$ 这一小段距离上，剩下的 $[2, 101]$ 都挤在 $[0.01, 1]$ 中。

因此深度缓冲的精度在近处高、远处低，远处物体更容易发生深度冲突（z-fighting）。实践中常把近平面取得尽量远来缓解这个问题。

&nbsp;

## 8. 屏幕映射

屏幕映射（Screen Mapping，也称视口变换）是几何阶段的最后一步：将透视除法得到的 NDC 映射到屏幕坐标系中的窗口坐标，之后顶点数据连同其属性进入光栅化阶段。

&nbsp;

**视口**

视口（Viewport）是屏幕上的一个矩形区域，用左下角坐标 $(x_0, y_0)$ 与宽高 $w, h$ 描述。屏幕映射将 NDC 的 $x, y$ 从 $[-1, 1]$ 线性映射到 $[x_0, x_0 + w]$、$[y_0, y_0 + h]$：
$$
x_s = \frac{w}{2} x_{ndc} + x_0 + \frac{w}{2} \ \ \ \ \ \ y_s = \frac{h}{2} y_{ndc} + y_0 + \frac{h}{2}
$$
同时 $z$ 从 $[-1, 1]$ 映射到深度缓冲范围（默认 $[0, 1]$）：
$$
z_d = \frac{1}{2} z_{ndc} + \frac{1}{2}
$$
深度范围可以通过相应的状态设置调整（OpenGL 的 `glDepthRange`，Vulkan 视口中的 `minDepth/maxDepth`），非默认范围常用于分层渲染等技巧。

写成矩阵形式，与本章的其他变换统一：
$$
\mathbf{M}_{viewport} = \begin{pmatrix}
\frac{w}{2} & 0 & 0 & x_0 + \frac{w}{2} \\
0 & \frac{h}{2} & 0 & y_0 + \frac{h}{2} \\
0 & 0 & \frac{1}{2} & \frac{1}{2} \\
0 & 0 & 0 & 1
\end{pmatrix}
$$
不过与前文的变换不同，屏幕映射由固定功能硬件执行，视口参数通过 API 设置，不需要在着色器中构造该矩阵。

&nbsp;

## 9. 代码解读

在 `demo3.cpp` 中进行了一系列变换验证。

&nbsp;

**$z$ 重映射**

项目中并没有复合上文所说的 $z$ 重映射矩阵，而是通过 `addglm.cmake` 中：

```cmake
 # Vulkan 的规范化设备坐标深度范围为 [0, 1](OpenGL 传统为 [-1, 1])
 # 该宏让 glm::perspective / glm::ortho 直接产出 Vulkan 风格深度
 # 挂在 INTERFACE 目标上,所有链接 glm::glm 的翻译单元看到同一套数学约定(ODR 安全)
 target_compile_definitions(glm INTERFACE GLM_FORCE_DEPTH_ZERO_TO_ONE)
```

定义了 `GLM_FORCE_DEPTH_ZERO_TO_ONE`，能够直接产出 Vulkan 的 `z∈[0,1]` 深度约定。

&nbsp;

**$y$ 翻转**

项目中使用如下方式进行 $y$ 翻转：

```c++
glm::mat4 projFlipY = proj;
projFlipY[1][1] *= -1.0f;
```

这并非唯一方案，以下两种方案同样能解决问题：

- 负视口高度

  Vulkan 允许 `VkViewport.height` 为负值（需 `VK_KHR_maintenance1` 扩展支持），表示“y 向下翻转的视口”。此时 CPU 侧矩阵保持 OpenGL 习惯不变，y 翻转由视口变换阶段吸收。

- 着色器翻转

  直接在顶点着色器最后一行 `gl_Position.y = -gl_Position.y;`。

&nbsp;

# 3. Vulkan 初始化

## 1. Instance

`VkInstance` 是 Vulkan 初始化流程的起点。与传统图形 API 不同，它并非全局单例。可以在同一进程内创建多个独立的 `VkInstance`，它们互不干扰。

创建 `VkInstance` 需要填充 `VkInstanceCreateInfo`：

```c++
typedef struct VkInstanceCreateInfo {
    VkStructureType          sType;
    const void*              pNext;
    VkInstanceCreateFlags    flags;
    const VkApplicationInfo* pApplicationInfo;
    uint32_t                 enabledLayerCount;
    const char* const*       ppEnabledLayerNames;
    uint32_t                 enabledExtensionCount;
    const char* const*       ppEnabledExtensionNames;
} VkInstanceCreateInfo;
```

&nbsp;

**Layer**

Vulkan 的 Loader 在应用与驱动（ICD）之间维护了一条调用链。层（Layer）就是被动态加载并插入这条链中的共享库，能够拦截、检查甚至修改流经的 Vulkan 函数调用。

需要显式启用的层如 Validation，用于 API 捕获/追踪、扩展实现；

默认自动启用的层如 Overlay，用于后处理、录屏、物理设备重排序。

>`VK_LAYER_KHRONOS_validation` 是 Khronos 官方提供的标准验证层。启用它能获得全面的参数校验、对象生命周期检查、线程安全检查等能力，帮助避免 API 误用。它的开销只存在于开发调试阶段，发布版本中禁用即可，不会影响最终性能。

&nbsp;

**Extension**

扩展（Extension）是在核心规范之外增加新功能的机制。它可以定义全新的命令、结构体、枚举值，也可以修改某些既有行为。

按照作用域可分为实例拓展和设备拓展：

- 实例拓展绑定整个 `VkInstance`，如 `VK_KHR_surface`，`VK_EXT_debug_utils`。
- 设备拓展绑定单个 `VkDevice`，如 `VK_KHR_swapchain`，特定硬件特性扩展。

扩展名称的前缀通常指示其来源与成熟度。如`KHR`由 Khronos 主导，通常具有跨厂商的广泛支持。而 `EXT` 代表多厂商协作，但不一定进入核心。厂商前缀如 `AMD`、`NV` 是特定厂商的硬件或驱动特性。

&nbsp;

|          |                          层 (Layer)                          |                       扩展 (Extension)                       |
| -------- | :----------------------------------------------------------: | :----------------------------------------------------------: |
| 本质     |          拦截器 / 中间件，是位于应用和驱动之间的库           |              功能模块，是 Vulkan API 规范的延伸              |
| 作用     | 改变 API 调用的执行过程（检查、记录、修改），不能增加新的 API 命令 |   增加新的 API 命令、结构体、枚举，扩展 Vulkan 能做的事情    |
| 提供者   |     通常由第三方（如 LunarG、GPU 厂商）提供，独立于驱动      |          通常由驱动（ICD）实现，代表硬件或平台能力           |
| 启用时机 | 实例层在 `vkCreateInstance` 启用，设备层/全局层参与后续设备链的构建 | 实例扩展在 `vkCreateInstance` 启用，设备扩展在 `vkCreateDevice` 启用 |

&nbsp;

**pNext**

`pNext` 是 Vulkan API 中一个核心的可扩展性机制，它的本质是一个指向单向链表的指针。在创建实例时，可将多个来自扩展的结构体串联起来，从而传递标准结构体字段无法覆盖的额外信息。

`pNext` 约定，所有可链入 `pNext` 链的 Vulkan 结构体，其前两个成员的类型和顺序都是固定的：

1. `VkStructureType sType`

   一个枚举值，用于标识这个结构体是什么类型。如 `VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT`

2. `const void*`（或 `void*`）

   指针，用于指向链中的下一个结构体

这条链可以不断链接，任意长度。

对于 instance 创建而言，`pNext` 链主要用于传递实例级别的扩展配置，如 `VkDebugUtilsMessengerCreateInfoEXT` 或 `VkDebugReportCallbackCreateInfoEXT`，用于捕获实例创建和销毁期间产生的调试信息。`VkDirectDriverLoadingListLUNARG`，允许应用指定额外的驱动加载路径。

通过 `pNext` 链入的拓展，必须同时被添加到 `ppEnabledExtensionNames` 数组中。

&nbsp;





