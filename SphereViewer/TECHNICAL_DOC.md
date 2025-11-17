# 3D球体查看器 - 技术文档

## 一、系统架构

### 1.1 类结构

```
CSphereViewerApp (应用程序类)
    └── CSphereViewerDlg (主对话框类)
            └── Sphere3D (球体3D类)
                    ├── Point3D (3D点结构)
                    └── Face (面结构)
```

### 1.2 主要类说明

#### Point3D 结构
```cpp
struct Point3D {
    double x, y, z;  // 三维坐标
};
```
- 表示三维空间中的一个点
- 用于存储顶点坐标、法向量等

#### Face 结构
```cpp
struct Face {
    std::vector<int> vertices;  // 顶点索引列表
    Point3D normal;              // 面法向量
};
```
- 表示一个多边形面（通常是三角形）
- 存储构成面的顶点索引
- 存储预计算的法向量用于消隐判断

#### Sphere3D 类
核心3D球体类，负责：
- 球体几何生成
- 坐标变换（旋转）
- 投影计算
- 消隐判断
- 渲染输出
- 数据文件I/O

## 二、核心算法

### 2.1 地理划分法（Geographic Subdivision）

#### 原理
使用经纬度参数化方法生成球面上的点：
```
x = r * sin(θ) * cos(φ)
y = r * cos(θ)
z = r * sin(θ) * sin(φ)
```
其中：
- r: 球体半径
- θ (theta): 纬度角，从北极(0)到南极(π)
- φ (phi): 经度角，从0到2π

#### 实现步骤

1. **生成北极点**
```cpp
vertices.push_back(Point3D(0, radius, 0));
```

2. **生成中间纬线上的点**
```cpp
for (int lat = 1; lat < latitudes; lat++) {
    double theta = M_PI * lat / latitudes;
    double sinTheta = sin(theta);
    double cosTheta = cos(theta);
    
    for (int lon = 0; lon < longitudes; lon++) {
        double phi = 2.0 * M_PI * lon / longitudes;
        double x = radius * sinTheta * cos(phi);
        double z = radius * sinTheta * sin(phi);
        double y = radius * cosTheta;
        
        vertices.push_back(Point3D(x, y, z));
    }
}
```

3. **生成南极点**
```cpp
vertices.push_back(Point3D(0, -radius, 0));
```

4. **构建三角形网格**
- 北极三角形扇：连接北极点和第一纬线圈
- 中间四边形带：每个四边形分解为两个三角形
- 南极三角形扇：连接最后纬线圈和南极点

#### 复杂度分析
- 顶点数：`2 + (latitudes - 1) × longitudes`
- 三角形数：`2 × longitudes + 2 × (latitudes - 2) × longitudes`
- 对于20×24配置：402个顶点，800个三角形

### 2.2 旋转变换

#### 三维旋转矩阵

**绕X轴旋转**：
```
[1    0         0    ]   [x]
[0  cos(α)  -sin(α) ] × [y]
[0  sin(α)   cos(α) ]   [z]
```

**绕Y轴旋转**：
```
[ cos(β)  0  sin(β)]   [x]
[   0     1    0   ] × [y]
[-sin(β)  0  cos(β)]   [z]
```

**绕Z轴旋转**：
```
[cos(γ)  -sin(γ)  0]   [x]
[sin(γ)   cos(γ)  0] × [y]
[  0        0     1]   [z]
```

#### 实现代码
```cpp
void Sphere3D::RotatePoint(Point3D& p, double ax, double ay, double az) {
    // X轴旋转
    if (ax != 0) {
        double cosX = cos(ax);
        double sinX = sin(ax);
        double y = p.y * cosX - p.z * sinX;
        double z = p.y * sinX + p.z * cosX;
        p.y = y;
        p.z = z;
    }
    
    // Y轴旋转
    if (ay != 0) {
        double cosY = cos(ay);
        double sinY = sin(ay);
        double x = p.x * cosY + p.z * sinY;
        double z = -p.x * sinY + p.z * cosY;
        p.x = x;
        p.z = z;
    }
    
    // Z轴旋转
    if (az != 0) {
        double cosZ = cos(az);
        double sinZ = sin(az);
        double x = p.x * cosZ - p.y * sinZ;
        double y = p.x * sinZ + p.y * cosZ;
        p.x = x;
        p.y = y;
    }
}
```

#### 旋转顺序
采用XYZ欧拉角顺序：先绕X轴，再绕Y轴，最后绕Z轴

### 2.3 投影变换

#### 正交投影
本程序使用正交投影（Orthographic Projection）：
```cpp
CPoint Sphere3D::Project(const Point3D& p, CRect clientRect) {
    int centerX = clientRect.Width() / 2;
    int centerY = clientRect.Height() / 2;
    double scale = min(centerX, centerY) * 0.8;
    
    int screenX = centerX + (int)(p.x * scale);
    int screenY = centerY - (int)(p.y * scale);  // Y轴翻转
    
    return CPoint(screenX, screenY);
}
```

特点：
- 平行线保持平行
- 不考虑深度透视
- 适合查看物体整体结构
- 计算简单高效

#### 坐标系转换
- 3D世界坐标 → 2D屏幕坐标
- Y轴需要翻转（屏幕Y轴向下）
- 中心对齐到客户区中心

### 2.4 消隐算法（Hidden Surface Removal）

#### 背面剔除（Back-Face Culling）

**原理**：对于凸多面体，背向观察者的面不可见。

**判断方法**：
1. 计算面法向量 **n**
2. 计算视线向量 **v**（从面中心指向观察点）
3. 计算点积 **n · v**
4. 若 **n · v > 0**，面可见；否则不可见

#### 法向量计算

使用叉积计算：
```cpp
void Sphere3D::ComputeFaceNormal(Face& face) {
    Point3D v0 = vertices[face.vertices[0]];
    Point3D v1 = vertices[face.vertices[1]];
    Point3D v2 = vertices[face.vertices[2]];
    
    // 边向量
    Point3D edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
    Point3D edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
    
    // 叉积
    face.normal.x = edge1.y * edge2.z - edge1.z * edge2.y;
    face.normal.y = edge1.z * edge2.x - edge1.x * edge2.z;
    face.normal.z = edge1.x * edge2.y - edge1.y * edge2.x;
    
    // 归一化
    double length = sqrt(normal.x² + normal.y² + normal.z²);
    face.normal /= length;
}
```

#### 可见性测试
```cpp
bool Sphere3D::IsFaceVisible(const Face& face, const Point3D& viewPoint) {
    // 计算面中心
    Point3D center = ComputeFaceCenter(face);
    
    // 应用旋转变换到法向量
    Point3D normal = face.normal;
    RotatePoint(normal, angleX, angleY, angleZ);
    
    // 视线向量
    Point3D toView(viewPoint.x - center.x,
                   viewPoint.y - center.y,
                   viewPoint.z - center.z);
    
    // 点积判断
    double dot = normal.x * toView.x + 
                 normal.y * toView.y + 
                 normal.z * toView.z;
    
    return dot > 0;
}
```

#### 观察点位置
```cpp
Point3D viewPoint(0, 0, radius * 5);
```
观察点位于球心前方，距离为半径的5倍。

### 2.5 双缓冲技术

#### 实现流程
```cpp
void CSphereViewerDlg::OnPaint() {
    CDC* pDC = pRenderWnd->GetDC();
    CRect rect;
    pRenderWnd->GetClientRect(&rect);
    
    // 创建内存DC
    CDC memDC;
    memDC.CreateCompatibleDC(pDC);
    CBitmap memBitmap;
    memBitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&memBitmap);
    
    // 在内存中绘图
    memDC.FillSolidRect(&rect, RGB(255, 255, 255));
    DrawCoordinateSystem(&memDC);
    m_sphere.Draw(&memDC, rect, m_bUseHiddenSurfaceRemoval);
    
    // 一次性复制到屏幕
    pDC->BitBlt(0, 0, rect.Width(), rect.Height(), &memDC, 0, 0, SRCCOPY);
    
    // 清理
    memDC.SelectObject(pOldBitmap);
    pRenderWnd->ReleaseDC(pDC);
}
```

优点：
- 消除闪烁
- 提供流畅视觉体验
- 所有绘图在后台完成

## 三、数据结构

### 3.1 顶点表（Vertex Table）
```cpp
std::vector<Point3D> vertices;
```
- 存储所有顶点的3D坐标
- 索引从0开始
- 用于几何变换和渲染

### 3.2 面表（Face Table）
```cpp
std::vector<Face> faces;
```
- 存储所有面的顶点索引
- 每个面通常是三角形（3个顶点）
- 预存储法向量用于消隐

### 3.3 数据关系
```
Face[i].vertices[j] → 顶点索引 → Vertices[index] → Point3D坐标
```

## 四、性能优化

### 4.1 已实现的优化

1. **预计算法向量**
   - 初始化时计算所有面的法向量
   - 避免每帧重复计算

2. **双缓冲绘图**
   - 减少屏幕刷新次数
   - 消除闪烁

3. **选择性绘制**
   - 消隐算法跳过不可见面
   - 减少约50%的绘图操作

4. **定点数组**
   - 使用连续内存存储顶点
   - 提高缓存命中率

### 4.2 可能的改进方向

1. **使用OpenGL/DirectX**
   - 硬件加速渲染
   - 支持更复杂的模型

2. **四叉树/八叉树**
   - 空间分区加速可见性判断
   - 适用于大型场景

3. **多线程渲染**
   - 并行计算变换和投影
   - 提高帧率

4. **LOD技术**
   - 根据距离调整细节级别
   - 降低远处物体的多边形数

## 五、坐标系统

### 5.1 右手坐标系

定义：
- X轴：向右为正
- Y轴：向上为正
- Z轴：向前（朝向观察者）为正

验证右手法则：
```
右手拇指指向X正方向
食指指向Y正方向
中指指向Z正方向
```

### 5.2 坐标系变换链

```
本地坐标（模型空间）
    ↓ 旋转变换
世界坐标（世界空间）
    ↓ 视图变换（省略）
相机坐标（视图空间）
    ↓ 投影变换
屏幕坐标（屏幕空间）
```

本程序简化：本地坐标 → 旋转 → 投影 → 屏幕坐标

## 六、文件格式规范

### 6.1 格式定义

```
<comment_lines>          # 以#开头的注释行（可选）
VERTICES <count>         # 顶点数声明
<x> <y> <z>             # 每个顶点的坐标（count行）
<blank_lines>           # 空行（可选）
<comment_lines>         # 注释（可选）
FACES <count>           # 面数声明
<n> <v1> <v2> ... <vn>  # 每个面的顶点数和索引（count行）
```

### 6.2 约束条件

1. 顶点索引从0开始
2. 每个面至少3个顶点（三角形）
3. 顶点索引必须有效（< 顶点总数）
4. 坐标为浮点数
5. 文件编码：ASCII或UTF-8

### 6.3 错误处理

程序对文件读取进行了基本的容错处理：
- 跳过注释行和空行
- 检查文件打开状态
- 验证数据完整性

## 七、扩展建议

### 7.1 功能扩展

1. **添加光照模型**
   - Phong光照模型
   - 环境光、漫反射、镜面反射

2. **支持纹理贴图**
   - UV坐标生成
   - 纹理采样

3. **透视投影**
   - 更真实的3D效果
   - 近大远小

4. **多种投影模式**
   - 正交投影
   - 透视投影
   - 等轴测投影

### 7.2 算法改进

1. **Z-buffer深度测试**
   - 更准确的消隐
   - 支持复杂模型

2. **Painter's算法**
   - 从远到近排序
   - 后绘制覆盖先绘制

3. **BSP树**
   - 空间二分
   - 快速可见性判断

4. **光线追踪**
   - 真实光照效果
   - 阴影和反射

## 八、调试技巧

### 8.1 常用调试方法

1. **可视化法向量**
   ```cpp
   // 绘制法向量箭头
   void DrawNormals(CDC* pDC);
   ```

2. **显示顶点编号**
   ```cpp
   // 在顶点位置显示索引
   pDC->TextOut(screenPos, CString::Format("%d", index));
   ```

3. **线框/实体切换**
   ```cpp
   bool m_bWireframe;  // 控制渲染模式
   ```

4. **单步旋转**
   ```cpp
   // 精确控制旋转角度
   const double STEP = 0.01;  // 小步长
   ```

### 8.2 性能分析

使用Windows性能计数器：
```cpp
LARGE_INTEGER start, end, freq;
QueryPerformanceFrequency(&freq);
QueryPerformanceCounter(&start);
// ... 待测代码 ...
QueryPerformanceCounter(&end);
double timeMs = (end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
```

## 九、参考资料

### 书籍
1. 《计算机图形学》- Hearn & Baker
2. 《3D游戏编程大师技巧》- André LaMothe
3. 《实时渲染》- Tomas Akenine-Möller

### 在线资源
1. LearnOpenGL - 现代OpenGL教程
2. Scratchapixel - 图形学基础
3. OpenGL Tutorial - 入门教程

### 相关技术
1. MFC编程基础
2. Windows GDI绘图
3. 线性代数和矩阵变换
4. 3D几何算法
