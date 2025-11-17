# ex2 - 3D球体查看器实验

这是一个使用Visual Studio 2022和MFC实现的3D球体可视化实验项目。

## 项目说明

本项目实现了以下6个目标：

1. **三维坐标系**：在屏幕客户区中心建立右手坐标系（X轴水平向右、Y轴垂直向上、Z轴垂直屏幕指向观察者）
2. **球体线框模型**：使用地理划分法（经纬线网格）绘制球体
3. **数据文件读写**：采用点表和面表构造球体数据文件（可读写）
4. **消隐算法**：对球体线框模型应用凸多面体消隐算法
5. **键盘交互**：使用键盘方向键交互旋转球体
6. **动画功能**：实现"动画"按钮，播放/停止球体旋转动画

## 项目位置

完整的项目代码位于 `SphereViewer/` 目录中。

详细的使用说明和技术文档请参阅：[SphereViewer/README.md](SphereViewer/README.md)

## 快速开始

1. 使用Visual Studio 2022打开 `SphereViewer/SphereViewer.sln`
2. 确保已安装MFC组件
3. 选择x64平台
4. 点击"生成解决方案"
5. 运行程序

## 技术栈

- Visual Studio 2022
- MFC (Microsoft Foundation Classes)
- C++17
- Windows SDK 10.0
