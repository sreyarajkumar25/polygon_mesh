#ifndef TRANSFORMATIONS_H
#define TRANSFORMATIONS_H

/* 4x4 column-major matrix (OpenGL style) */
typedef double Mat4[16];

typedef enum {
    MODE_VIEW = 0,
    MODE_TRANSLATE,
    MODE_SCALE,
    MODE_ROTATE,
    MODE_REFLECT,
    MODE_SHEAR,
    MODE_COMPOSITE
} TransformMode;

typedef enum {
    REFLECT_YZ = 0,  /* x' = -x */
    REFLECT_XZ,      /* y' = -y */
    REFLECT_XY       /* z' = -z */
} ReflectPlane;

typedef enum {
    AXIS_X = 0,
    AXIS_Y,
    AXIS_Z
} AxisChoice;

void matIdentity(Mat4 m);
void matCopy(const Mat4 src, Mat4 dst);
void matMultiply(const Mat4 a, const Mat4 b, Mat4 out);

void matTranslate(Mat4 m, double tx, double ty, double tz);
void matScale(Mat4 m, double sx, double sy, double sz);
void matRotateX(Mat4 m, double degrees);
void matRotateY(Mat4 m, double degrees);
void matRotateZ(Mat4 m, double degrees);

void matReflectPlane(Mat4 m, ReflectPlane plane);
void matShear(Mat4 m, int targetAxis, int shearAxis, double factor);

void transformPoint(const Mat4 m, double x, double y, double z, double* ox, double* oy, double* oz);

const char* modeName(TransformMode mode);
const char* reflectPlaneName(ReflectPlane plane);

#endif
