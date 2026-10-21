#include "transformations.h"
#include <cmath>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double deg2rad(double d) { return d * M_PI / 180.0; }

void matIdentity(Mat4 m) {
    std::memset(m, 0, sizeof(Mat4));
    m[0] = m[5] = m[10] = m[15] = 1.0;
}

void matCopy(const Mat4 src, Mat4 dst) {
    std::memcpy(dst, src, sizeof(Mat4));
}

void matMultiply(const Mat4 a, const Mat4 b, Mat4 out) {
    Mat4 r;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            r[col * 4 + row] =
                a[0 * 4 + row] * b[col * 4 + 0] +
                a[1 * 4 + row] * b[col * 4 + 1] +
                a[2 * 4 + row] * b[col * 4 + 2] +
                a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
    matCopy(r, out);
}

void matTranslate(Mat4 m, double tx, double ty, double tz) {
    m[12] += m[0] * tx + m[4] * ty + m[8] * tz;
    m[13] += m[1] * tx + m[5] * ty + m[9] * tz;
    m[14] += m[2] * tx + m[6] * ty + m[10] * tz;
    m[15] += m[3] * tx + m[7] * ty + m[11] * tz;
}

void matScale(Mat4 m, double sx, double sy, double sz) {
    for (int i = 0; i < 4; ++i) {
        m[i]     *= sx;
        m[4 + i] *= sy;
        m[8 + i] *= sz;
    }
}

static void matRotateAxis(Mat4 m, char axis, double degrees) {
    double c = std::cos(deg2rad(degrees));
    double s = std::sin(deg2rad(degrees));
    Mat4 r;
    matIdentity(r);
    if (axis == 'X') {
        r[5] = c;  r[6] = s;
        r[9] = -s; r[10] = c;
    } else if (axis == 'Y') {
        r[0] = c;  r[2] = -s;
        r[8] = s;  r[10] = c;
    } else {
        r[0] = c;  r[1] = s;
        r[4] = -s; r[5] = c;
    }
    matMultiply(m, r, m);
}

void matRotateX(Mat4 m, double degrees) { matRotateAxis(m, 'X', degrees); }
void matRotateY(Mat4 m, double degrees) { matRotateAxis(m, 'Y', degrees); }
void matRotateZ(Mat4 m, double degrees) { matRotateAxis(m, 'Z', degrees); }

void matReflectPlane(Mat4 m, ReflectPlane plane) {
    Mat4 r;
    matIdentity(r);
    if (plane == REFLECT_YZ) r[0] = -1.0;
    else if (plane == REFLECT_XZ) r[5] = -1.0;
    else r[10] = -1.0;
    matMultiply(m, r, m);
}

/* targetAxis: axis being modified (0=X,1=Y,2=Z); shearAxis: axis multiplied in */
void matShear(Mat4 m, int targetAxis, int shearAxis, double factor) {
    Mat4 r;
    matIdentity(r);
    int row = targetAxis;
    int col = shearAxis;
    r[col * 4 + row] = factor;
    matMultiply(m, r, m);
}

void transformPoint(const Mat4 m, double x, double y, double z, double* ox, double* oy, double* oz) {
    double w = m[3] * x + m[7] * y + m[11] * z + m[15];
    if (w == 0.0) w = 1.0;
    *ox = (m[0] * x + m[4] * y + m[8] * z + m[12]) / w;
    *oy = (m[1] * x + m[5] * y + m[9] * z + m[13]) / w;
    *oz = (m[2] * x + m[6] * y + m[10] * z + m[14]) / w;
}

const char* modeName(TransformMode mode) {
    switch (mode) {
        case MODE_VIEW: return "View / Inspect";
        case MODE_TRANSLATE: return "Translation";
        case MODE_SCALE: return "Scaling";
        case MODE_ROTATE: return "Rotation";
        case MODE_REFLECT: return "Reflection";
        case MODE_SHEAR: return "Shearing";
        case MODE_COMPOSITE: return "Composite";
        default: return "Unknown";
    }
}

const char* reflectPlaneName(ReflectPlane plane) {
    switch (plane) {
        case REFLECT_YZ: return "YZ plane (x' = -x)";
        case REFLECT_XZ: return "XZ plane (y' = -y)";
        case REFLECT_XY: return "XY plane (z' = -z)";
        default: return "";
    }
}
