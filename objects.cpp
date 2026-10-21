#include "objects.h"
#include "transformations.h"

#if defined(_WIN32)
#include <GL/freeglut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <cmath>
#include <cstdlib>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void addVertex(Mesh* m, double x, double y, double z) {
    m->vertices[m->vertexCount++] = {x, y, z};
}

static void addTri(Mesh* m, int a, int b, int c) {
    m->triangles[m->triangleCount++] = {a, b, c};
}

static void addEdge(Mesh* m, int a, int b) {
    if (a > b) { int t = a; a = b; b = t; }
    for (int i = 0; i < m->edgeCount; ++i) {
        if (m->edges[i].a == a && m->edges[i].b == b) return;
    }
    m->edges[m->edgeCount++] = {a, b};
}

static void addTriEdges(Mesh* m, int a, int b, int c) {
    addEdge(m, a, b);
    addEdge(m, b, c);
    addEdge(m, c, a);
}

static void appendBox(Mesh* m, double cx, double cy, double cz, double sx, double sy, double sz) {
    int base = m->vertexCount;
    double hx = sx * 0.5, hy = sy * 0.5, hz = sz * 0.5;
    addVertex(m, cx - hx, cy - hy, cz - hz);
    addVertex(m, cx + hx, cy - hy, cz - hz);
    addVertex(m, cx + hx, cy + hy, cz - hz);
    addVertex(m, cx - hx, cy + hy, cz - hz);
    addVertex(m, cx - hx, cy - hy, cz + hz);
    addVertex(m, cx + hx, cy - hy, cz + hz);
    addVertex(m, cx + hx, cy + hy, cz + hz);
    addVertex(m, cx - hx, cy + hy, cz + hz);
    int v = base;
    addTri(m, v, v + 1, v + 2); addTri(m, v, v + 2, v + 3);
    addTri(m, v + 4, v + 6, v + 5); addTri(m, v + 4, v + 7, v + 6);
    addTri(m, v, v + 4, v + 5); addTri(m, v, v + 5, v + 1);
    addTri(m, v + 2, v + 6, v + 7); addTri(m, v + 2, v + 7, v + 3);
    addTri(m, v, v + 3, v + 7); addTri(m, v, v + 7, v + 4);
    addTri(m, v + 1, v + 5, v + 6); addTri(m, v + 1, v + 6, v + 2);
    for (int i = 0; i < 12; ++i) {
        Triangle t = m->triangles[m->triangleCount - 12 + i];
        addTriEdges(m, t.v0, t.v1, t.v2);
    }
}

static void buildCube(Mesh* m) {
    m->name = "Cube";
    appendBox(m, 0, 0, 0, 1.6, 1.6, 1.6);
}

static void buildPyramid(Mesh* m) {
    m->name = "Pyramid";
    addVertex(m, -1, -0.8, -1);
    addVertex(m,  1, -0.8, -1);
    addVertex(m,  1, -0.8,  1);
    addVertex(m, -1, -0.8,  1);
    addVertex(m,  0,  1.2,  0);
    addTri(m, 0, 1, 2); addTri(m, 0, 2, 3);
    addTri(m, 0, 4, 1);
    addTri(m, 1, 4, 2);
    addTri(m, 2, 4, 3);
    addTri(m, 3, 4, 0);
    for (int i = 0; i < m->triangleCount; ++i) {
        Triangle t = m->triangles[i];
        addTriEdges(m, t.v0, t.v1, t.v2);
    }
}

static void buildSphere(Mesh* m, int stacks, int slices) {
    m->name = "Sphere";
    for (int i = 0; i <= stacks; ++i) {
        double phi = M_PI * i / stacks;
        double y = std::cos(phi);
        double r = std::sin(phi);
        for (int j = 0; j <= slices; ++j) {
            double theta = 2.0 * M_PI * j / slices;
            double x = r * std::cos(theta);
            double z = r * std::sin(theta);
            addVertex(m, x * 1.2, y * 1.2, z * 1.2);
        }
    }
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int a = i * (slices + 1) + j;
            int b = a + slices + 1;
            addTri(m, a, b, a + 1);
            addTri(m, b, b + 1, a + 1);
        }
    }
    for (int i = 0; i < m->triangleCount; ++i) {
        Triangle t = m->triangles[i];
        addTriEdges(m, t.v0, t.v1, t.v2);
    }
}

static void buildHuman(Mesh* m) {
    m->name = "3D Human Figure";
    appendBox(m, 0, 2.0, 0, 0.55, 0.55, 0.55);
    appendBox(m, 0, 1.0, 0, 1.0, 1.2, 0.45);
    appendBox(m, -0.85, 1.15, 0, 0.35, 1.0, 0.35);
    appendBox(m,  0.85, 1.15, 0, 0.35, 1.0, 0.35);
    appendBox(m, -0.35, -0.35, 0, 0.4, 1.3, 0.4);
    appendBox(m,  0.35, -0.35, 0, 0.4, 1.3, 0.4);
}

void meshFree(Mesh* mesh) {
    std::free(mesh->vertices);
    std::free(mesh->triangles);
    std::free(mesh->edges);
    std::memset(mesh, 0, sizeof(Mesh));
}

void meshBuild(ObjectType type, Mesh* mesh) {
    meshFree(mesh);
    int maxV = 4096, maxT = 8192, maxE = 16384;
    mesh->vertices = (Vertex3*)std::malloc(sizeof(Vertex3) * maxV);
    mesh->triangles = (Triangle*)std::malloc(sizeof(Triangle) * maxT);
    mesh->edges = (Edge*)std::malloc(sizeof(Edge) * maxE);
    mesh->vertexCount = mesh->triangleCount = mesh->edgeCount = 0;

    switch (type) {
        case OBJ_CUBE: buildCube(mesh); break;
        case OBJ_PYRAMID: buildPyramid(mesh); break;
        case OBJ_SPHERE: buildSphere(mesh, 16, 24); break;
        case OBJ_HUMAN: buildHuman(mesh); break;
        default: buildCube(mesh); break;
    }
}

void meshDrawWireframe(const Mesh* mesh, float r, float g, float b, int showVertices) {
    glColor3f(r, g, b);
    glLineWidth(showVertices ? 2.2f : 1.6f);
    glBegin(GL_LINES);
    for (int i = 0; i < mesh->edgeCount; ++i) {
        const Vertex3& a = mesh->vertices[mesh->edges[i].a];
        const Vertex3& bv = mesh->vertices[mesh->edges[i].b];
        glVertex3d(a.x, a.y, a.z);
        glVertex3d(bv.x, bv.y, bv.z);
    }
    glEnd();
    if (showVertices) {
        glPointSize(4.0f);
        glColor3f(r * 0.7f + 0.3f, g, b);
        glBegin(GL_POINTS);
        for (int i = 0; i < mesh->vertexCount; ++i) {
            glVertex3d(mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z);
        }
        glEnd();
    }
}

void meshDrawSolid(const Mesh* mesh, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < mesh->triangleCount; ++i) {
        const Triangle& t = mesh->triangles[i];
        const Vertex3& a = mesh->vertices[t.v0];
        const Vertex3& b = mesh->vertices[t.v1];
        const Vertex3& c = mesh->vertices[t.v2];
        double ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z;
        double vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
        double nx = uy * vz - uz * vy;
        double ny = uz * vx - ux * vz;
        double nz = ux * vy - uy * vx;
        glNormal3d(nx, ny, nz);
        glVertex3d(a.x, a.y, a.z);
        glVertex3d(b.x, b.y, b.z);
        glVertex3d(c.x, c.y, c.z);
    }
    glEnd();
}

void meshCopyTransformed(const Mesh* src, Mesh* dst, const double matrix[16]) {
    if (!dst->vertices || dst->vertexCount != src->vertexCount) {
        meshFree(dst);
        dst->vertices = (Vertex3*)std::malloc(sizeof(Vertex3) * src->vertexCount);
        dst->triangles = (Triangle*)std::malloc(sizeof(Triangle) * src->triangleCount);
        dst->edges = (Edge*)std::malloc(sizeof(Edge) * src->edgeCount);
        dst->vertexCount = src->vertexCount;
        dst->triangleCount = src->triangleCount;
        dst->edgeCount = src->edgeCount;
        std::memcpy(dst->triangles, src->triangles, sizeof(Triangle) * src->triangleCount);
        std::memcpy(dst->edges, src->edges, sizeof(Edge) * src->edgeCount);
        dst->name = src->name;
    }
    Mat4 m;
    matCopy((const double*)matrix, m);
    for (int i = 0; i < src->vertexCount; ++i) {
        transformPoint(m, src->vertices[i].x, src->vertices[i].y, src->vertices[i].z,
                       &dst->vertices[i].x, &dst->vertices[i].y, &dst->vertices[i].z);
    }
}

const char* objectName(ObjectType type) {
    switch (type) {
        case OBJ_CUBE: return "Cube";
        case OBJ_PYRAMID: return "Pyramid";
        case OBJ_SPHERE: return "Sphere";
        case OBJ_HUMAN: return "3D Human Figure";
        default: return "Object";
    }
}
