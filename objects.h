#ifndef OBJECTS_H
#define OBJECTS_H

typedef struct {
    double x, y, z;
} Vertex3;

typedef struct {
    int v0, v1, v2;
} Triangle;

typedef struct {
    int a, b;
} Edge;

typedef struct {
    Vertex3* vertices;
    int vertexCount;
    Triangle* triangles;
    int triangleCount;
    Edge* edges;
    int edgeCount;
    const char* name;
} Mesh;

typedef enum {
    OBJ_CUBE = 0,
    OBJ_PYRAMID,
    OBJ_SPHERE,
    OBJ_HUMAN,
    OBJ_COUNT
} ObjectType;

void meshFree(Mesh* mesh);
void meshBuild(ObjectType type, Mesh* mesh);

void meshDrawWireframe(const Mesh* mesh, float r, float g, float b, int showVertices);
void meshDrawSolid(const Mesh* mesh, float r, float g, float b);

void meshCopyTransformed(const Mesh* src, Mesh* dst, const double matrix[16]);

const char* objectName(ObjectType type);

#endif
