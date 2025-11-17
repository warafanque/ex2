#pragma once
#include "framework.h"

// 3D Point structure
struct Point3D {
    double x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double _x, double _y, double _z) : x(_x), y(_y), z(_z) {}
};

// Face structure (polygon)
struct Face {
    std::vector<int> vertices; // indices to vertex list
    Point3D normal; // face normal for hidden surface removal
    
    Face() {}
    Face(const std::vector<int>& verts) : vertices(verts) {}
};

// 3D Sphere class
class Sphere3D {
private:
    std::vector<Point3D> vertices;  // vertex table
    std::vector<Face> faces;        // face table
    double radius;
    int latitudes;   // number of latitude divisions
    int longitudes;  // number of longitude divisions
    
    // Rotation angles
    double angleX;
    double angleY;
    double angleZ;
    
    // Transform and projection
    void RotatePoint(Point3D& p, double ax, double ay, double az);
    Point3D Transform(const Point3D& p);
    CPoint Project(const Point3D& p, CRect clientRect);
    
    // Hidden surface removal
    void ComputeFaceNormal(Face& face);
    bool IsFaceVisible(const Face& face, const Point3D& viewPoint);

public:
    Sphere3D();
    ~Sphere3D();
    
    // Initialize sphere with geographic subdivision
    void Initialize(double r, int lats, int longs);
    
    // Rotation
    void RotateX(double angle);
    void RotateY(double angle);
    void RotateZ(double angle);
    void ResetRotation();
    
    // File I/O
    bool SaveToFile(const CString& filename);
    bool LoadFromFile(const CString& filename);
    
    // Rendering
    void Draw(CDC* pDC, CRect clientRect, bool useHiddenSurfaceRemoval);
    
    // Getters
    int GetVertexCount() const { return (int)vertices.size(); }
    int GetFaceCount() const { return (int)faces.size(); }
};
