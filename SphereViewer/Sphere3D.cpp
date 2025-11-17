#include "pch.h"
#include "Sphere3D.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Sphere3D::Sphere3D() 
    : radius(1.0), latitudes(20), longitudes(20), 
      angleX(0), angleY(0), angleZ(0) {
}

Sphere3D::~Sphere3D() {
}

void Sphere3D::Initialize(double r, int lats, int longs) {
    radius = r;
    latitudes = lats;
    longitudes = longs;
    
    vertices.clear();
    faces.clear();
    
    // Generate vertices using geographic subdivision (latitude/longitude)
    // North pole
    vertices.push_back(Point3D(0, radius, 0));
    
    // Generate vertices for each latitude circle
    for (int lat = 1; lat < latitudes; lat++) {
        double theta = M_PI * lat / latitudes; // latitude angle from north pole
        double sinTheta = sin(theta);
        double cosTheta = cos(theta);
        
        for (int lon = 0; lon < longitudes; lon++) {
            double phi = 2.0 * M_PI * lon / longitudes; // longitude angle
            double x = radius * sinTheta * cos(phi);
            double z = radius * sinTheta * sin(phi);
            double y = radius * cosTheta;
            
            vertices.push_back(Point3D(x, y, z));
        }
    }
    
    // South pole
    vertices.push_back(Point3D(0, -radius, 0));
    
    // Generate faces
    // North pole triangles
    for (int lon = 0; lon < longitudes; lon++) {
        std::vector<int> verts;
        verts.push_back(0); // north pole
        verts.push_back(1 + lon);
        verts.push_back(1 + (lon + 1) % longitudes);
        Face face(verts);
        faces.push_back(face);
    }
    
    // Middle quads (split into triangles)
    for (int lat = 0; lat < latitudes - 2; lat++) {
        for (int lon = 0; lon < longitudes; lon++) {
            int current = 1 + lat * longitudes + lon;
            int next = 1 + lat * longitudes + (lon + 1) % longitudes;
            int currentBelow = 1 + (lat + 1) * longitudes + lon;
            int nextBelow = 1 + (lat + 1) * longitudes + (lon + 1) % longitudes;
            
            // First triangle
            std::vector<int> verts1;
            verts1.push_back(current);
            verts1.push_back(next);
            verts1.push_back(nextBelow);
            Face face1(verts1);
            faces.push_back(face1);
            
            // Second triangle
            std::vector<int> verts2;
            verts2.push_back(current);
            verts2.push_back(nextBelow);
            verts2.push_back(currentBelow);
            Face face2(verts2);
            faces.push_back(face2);
        }
    }
    
    // South pole triangles
    int southPoleIndex = (int)vertices.size() - 1;
    int lastRingStart = 1 + (latitudes - 2) * longitudes;
    for (int lon = 0; lon < longitudes; lon++) {
        std::vector<int> verts;
        verts.push_back(lastRingStart + lon);
        verts.push_back(southPoleIndex);
        verts.push_back(lastRingStart + (lon + 1) % longitudes);
        Face face(verts);
        faces.push_back(face);
    }
    
    // Compute normals for all faces
    for (size_t i = 0; i < faces.size(); i++) {
        ComputeFaceNormal(faces[i]);
    }
}

void Sphere3D::RotateX(double angle) {
    angleX += angle;
}

void Sphere3D::RotateY(double angle) {
    angleY += angle;
}

void Sphere3D::RotateZ(double angle) {
    angleZ += angle;
}

void Sphere3D::ResetRotation() {
    angleX = angleY = angleZ = 0;
}

void Sphere3D::RotatePoint(Point3D& p, double ax, double ay, double az) {
    // Rotate around X axis
    if (ax != 0) {
        double cosX = cos(ax);
        double sinX = sin(ax);
        double y = p.y * cosX - p.z * sinX;
        double z = p.y * sinX + p.z * cosX;
        p.y = y;
        p.z = z;
    }
    
    // Rotate around Y axis
    if (ay != 0) {
        double cosY = cos(ay);
        double sinY = sin(ay);
        double x = p.x * cosY + p.z * sinY;
        double z = -p.x * sinY + p.z * cosY;
        p.x = x;
        p.z = z;
    }
    
    // Rotate around Z axis
    if (az != 0) {
        double cosZ = cos(az);
        double sinZ = sin(az);
        double x = p.x * cosZ - p.y * sinZ;
        double y = p.x * sinZ + p.y * cosZ;
        p.x = x;
        p.y = y;
    }
}

Point3D Sphere3D::Transform(const Point3D& p) {
    Point3D result = p;
    RotatePoint(result, angleX, angleY, angleZ);
    return result;
}

CPoint Sphere3D::Project(const Point3D& p, CRect clientRect) {
    // Simple orthographic projection
    // Map 3D coordinates to 2D screen coordinates
    int centerX = clientRect.Width() / 2;
    int centerY = clientRect.Height() / 2;
    
    // Scale factor
    double scale = min(centerX, centerY) * 0.8;
    
    int screenX = centerX + (int)(p.x * scale);
    int screenY = centerY - (int)(p.y * scale); // Invert Y for screen coordinates
    
    return CPoint(screenX, screenY);
}

void Sphere3D::ComputeFaceNormal(Face& face) {
    if (face.vertices.size() < 3) return;
    
    // Get three vertices of the face
    Point3D v0 = vertices[face.vertices[0]];
    Point3D v1 = vertices[face.vertices[1]];
    Point3D v2 = vertices[face.vertices[2]];
    
    // Compute two edge vectors
    Point3D edge1(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z);
    Point3D edge2(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
    
    // Compute cross product (normal)
    face.normal.x = edge1.y * edge2.z - edge1.z * edge2.y;
    face.normal.y = edge1.z * edge2.x - edge1.x * edge2.z;
    face.normal.z = edge1.x * edge2.y - edge1.y * edge2.x;
    
    // Normalize
    double length = sqrt(face.normal.x * face.normal.x + 
                        face.normal.y * face.normal.y + 
                        face.normal.z * face.normal.z);
    if (length > 0) {
        face.normal.x /= length;
        face.normal.y /= length;
        face.normal.z /= length;
    }
}

bool Sphere3D::IsFaceVisible(const Face& face, const Point3D& viewPoint) {
    // Get the center of the face
    Point3D center(0, 0, 0);
    for (size_t i = 0; i < face.vertices.size(); i++) {
        Point3D v = Transform(vertices[face.vertices[i]]);
        center.x += v.x;
        center.y += v.y;
        center.z += v.z;
    }
    center.x /= face.vertices.size();
    center.y /= face.vertices.size();
    center.z /= face.vertices.size();
    
    // Compute transformed normal
    Point3D normal = face.normal;
    RotatePoint(normal, angleX, angleY, angleZ);
    
    // Vector from face center to view point
    Point3D toView(viewPoint.x - center.x, 
                   viewPoint.y - center.y, 
                   viewPoint.z - center.z);
    
    // Dot product determines visibility
    double dot = normal.x * toView.x + normal.y * toView.y + normal.z * toView.z;
    
    return dot > 0;
}

void Sphere3D::Draw(CDC* pDC, CRect clientRect, bool useHiddenSurfaceRemoval) {
    if (vertices.empty()) return;
    
    // View point (observer position - along positive Z axis)
    Point3D viewPoint(0, 0, radius * 5);
    
    // Set up drawing
    CPen pen(PS_SOLID, 1, RGB(0, 0, 255));
    CPen* oldPen = pDC->SelectObject(&pen);
    
    if (useHiddenSurfaceRemoval) {
        // Draw only visible faces
        for (size_t i = 0; i < faces.size(); i++) {
            if (IsFaceVisible(faces[i], viewPoint)) {
                // Draw face edges
                for (size_t j = 0; j < faces[i].vertices.size(); j++) {
                    int idx1 = faces[i].vertices[j];
                    int idx2 = faces[i].vertices[(j + 1) % faces[i].vertices.size()];
                    
                    Point3D p1 = Transform(vertices[idx1]);
                    Point3D p2 = Transform(vertices[idx2]);
                    
                    CPoint screen1 = Project(p1, clientRect);
                    CPoint screen2 = Project(p2, clientRect);
                    
                    pDC->MoveTo(screen1);
                    pDC->LineTo(screen2);
                }
            }
        }
    } else {
        // Draw all edges (wireframe)
        for (size_t i = 0; i < faces.size(); i++) {
            for (size_t j = 0; j < faces[i].vertices.size(); j++) {
                int idx1 = faces[i].vertices[j];
                int idx2 = faces[i].vertices[(j + 1) % faces[i].vertices.size()];
                
                Point3D p1 = Transform(vertices[idx1]);
                Point3D p2 = Transform(vertices[idx2]);
                
                CPoint screen1 = Project(p1, clientRect);
                CPoint screen2 = Project(p2, clientRect);
                
                pDC->MoveTo(screen1);
                pDC->LineTo(screen2);
            }
        }
    }
    
    pDC->SelectObject(oldPen);
}

bool Sphere3D::SaveToFile(const CString& filename) {
    // Convert CString to std::string for file operations
    CT2A pszConvertedAnsiString(filename);
    std::string str(pszConvertedAnsiString);
    std::ofstream file(str, std::ios::out);
    if (!file.is_open()) return false;
    
    // Write header
    file << "# Sphere 3D Model Data File\n";
    file << "# Format: Vertex and Face tables\n\n";
    
    // Write vertices
    file << "VERTICES " << vertices.size() << "\n";
    for (size_t i = 0; i < vertices.size(); i++) {
        file << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << "\n";
    }
    
    // Write faces
    file << "\nFACES " << faces.size() << "\n";
    for (size_t i = 0; i < faces.size(); i++) {
        file << faces[i].vertices.size();
        for (size_t j = 0; j < faces[i].vertices.size(); j++) {
            file << " " << faces[i].vertices[j];
        }
        file << "\n";
    }
    
    file.close();
    return true;
}

bool Sphere3D::LoadFromFile(const CString& filename) {
    // Convert CString to std::string for file operations
    CT2A pszConvertedAnsiString(filename);
    std::string str(pszConvertedAnsiString);
    std::ifstream file(str, std::ios::in);
    if (!file.is_open()) return false;
    
    vertices.clear();
    faces.clear();
    
    std::string line;
    
    // Skip comment lines
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.find("VERTICES") == 0) break;
    }
    
    // Read vertex count
    int vertexCount;
    sscanf_s(line.c_str(), "VERTICES %d", &vertexCount);
    
    // Read vertices
    for (int i = 0; i < vertexCount; i++) {
        double x, y, z;
        file >> x >> y >> z;
        vertices.push_back(Point3D(x, y, z));
    }
    
    // Read face count
    std::getline(file, line); // consume newline
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line.find("FACES") == 0) break;
    }
    
    int faceCount;
    sscanf_s(line.c_str(), "FACES %d", &faceCount);
    
    // Read faces
    for (int i = 0; i < faceCount; i++) {
        int numVerts;
        file >> numVerts;
        
        std::vector<int> verts;
        for (int j = 0; j < numVerts; j++) {
            int idx;
            file >> idx;
            verts.push_back(idx);
        }
        
        Face face(verts);
        faces.push_back(face);
    }
    
    file.close();
    
    // Compute normals
    for (size_t i = 0; i < faces.size(); i++) {
        ComputeFaceNormal(faces[i]);
    }
    
    return true;
}
