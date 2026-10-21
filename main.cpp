/*
 * 3D Computer Graphics Learning Simulator
 * OpenGL fixed pipeline + GLUT/freeglut
 */

#if defined(_WIN32)
#include <windows.h>
#endif

#if defined(_WIN32)
#include <GL/freeglut.h>
#elif defined(__APPLE__)
#include <GLUT/glut.h>
#include "mac_drawable.h"
#else
#include <GL/glut.h>
#endif

#include "objects.h"
#include "transformations.h"

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const int WIN_W = 1600;
static const int WIN_H = 1000;
static int gWinW = WIN_W;
static int gWinH = WIN_H;
/* GLUT event coords (may differ from pixel drawable on Retina macOS). */
static int gGlutW = WIN_W;
static int gGlutH = WIN_H;

/* Layout: window split into header, side menus, 3D center, learning panel, status bar. */
static const int TOP_H = 72;
static const int LEARN_H = 240;
static const int STATUS_H = 28;
static const int LEFT_W = 240;
static const int RIGHT_W = 280;
static const int UI_PAD = 16;       /* margin from panel edges */
static const int UI_LINE = 20;      /* vertical space between text lines */
static const int UI_SECTION = 16;   /* extra gap before a new section title */
static const int UI_KEY_X = 8;      /* key column offset inside a sidebar */
static const int UI_LABEL_X = 36;   /* label column (keys line up vertically) */

/* Bottom stack height (learning + status); 3D viewport sits above this. */
static const int BOT_H = LEARN_H + STATUS_H;

/* Panel background colors (RGB 0..1). */
static const float COL_TOP[] = {0.12f, 0.14f, 0.18f};
static const float COL_SIDE[] = {0.10f, 0.11f, 0.14f};
static const float COL_LEARN[] = {0.08f, 0.09f, 0.12f};
static const float COL_STATUS[] = {0.06f, 0.07f, 0.10f};
static const float COL_BORDER[] = {0.28f, 0.32f, 0.40f};
static const float COL_TITLE[] = {0.92f, 0.94f, 0.98f};
static const float COL_TEXT[] = {0.82f, 0.85f, 0.90f};
static const float COL_MUTED[] = {0.62f, 0.66f, 0.72f};
static const float COL_ACCENT[] = {0.45f, 0.82f, 1.00f};
static const float COL_HINT[] = {0.35f, 0.95f, 1.00f};
static const float COL_OK[] = {0.55f, 0.90f, 0.60f};

struct UiLayout {
    int winW, winH;
    int midTop;    /* y (top-down): first row below header */
    int learnTop;  /* y (top-down): learning panel */
    int statusTop; /* y (top-down): status bar */
    int midH;      /* height of center row (3D + side panels) */
    int vpBottom;  /* OpenGL viewport y for 3D (origin bottom-left) */
};

/* Match OpenGL drawing size to the real framebuffer (fixes black bands on Retina). */
static void syncWindowSize() {
    gGlutW = glutGet(GLUT_WINDOW_WIDTH);
    gGlutH = glutGet(GLUT_WINDOW_HEIGHT);
    if (gGlutW < 1) gGlutW = 1;
    if (gGlutH < 1) gGlutH = 1;

#if defined(__APPLE__)
    int pw = 0, ph = 0;
    macDrawablePixels(&pw, &ph);
    if (pw > 0 && ph > 0) {
        gWinW = pw;
        gWinH = ph;
        return;
    }
#endif
    gWinW = gGlutW;
    gWinH = gGlutH;
}

static void mouseToPixels(int& x, int& y) {
    if (gGlutW < 1 || gGlutH < 1) return;
    x = (int)(x * (gWinW / (double)gGlutW));
    y = (int)(y * (gWinH / (double)gGlutH));
}

static UiLayout uiLayout() {
    UiLayout u;
    u.winW = gWinW;
    u.winH = gWinH;
    u.midTop = TOP_H;
    u.learnTop = gWinH - LEARN_H - STATUS_H;
    u.statusTop = gWinH - STATUS_H;
    u.midH = u.learnTop - u.midTop;
    if (u.midH < 1) u.midH = 1;
    u.vpBottom = BOT_H; /* height of bottom stack; 3D viewport starts above it */
    return u;
}

static struct AppState {
    ObjectType objectType;
    TransformMode mode;
    AxisChoice axis;
    int shearKind; /* 0=XY,1=XZ,2=YX,3=YZ,4=ZX,5=ZY */

    bool wireframeOn;
    bool solidOn;
    bool showBoth;          /* original + transformed */
    bool wireLearnMode;     /* emphasize wireframe inspection */
    bool compositeMode;

    double tx, ty, tz;
    double sx, sy, sz;
    double rx, ry, rz;
    ReflectPlane reflect;
    double shXY, shXZ, shYX, shYZ, shZX, shZY;

    Mat4 compositeMatrix;
    char compositeLog[512];

    Mesh baseMesh;
    Mesh transformedMesh;

    double camAz, camEl, camDist;
    bool autoSpin;
    bool dragging;
    int lastX, lastY;

    char statusMsg[128];
} g;

static void resetTransforms() {
    g.tx = g.ty = g.tz = 0;
    g.sx = g.sy = g.sz = 1;
    g.rx = g.ry = g.rz = 0;
    g.reflect = REFLECT_YZ;
    g.shXY = g.shXZ = g.shYX = g.shYZ = g.shZX = g.shZY = 0;
    matIdentity(g.compositeMatrix);
    std::snprintf(g.compositeLog, sizeof(g.compositeLog), "(empty sequence)");
    std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Transforms reset.");
}

static void resetAll() {
    resetTransforms();
    g.camAz = 35;
    g.camEl = 18;
    g.camDist = 9;
    g.autoSpin = false;
}

static void rebuildMatrix(Mat4 m) {
    matIdentity(m);
    if (g.compositeMode) {
        matCopy(g.compositeMatrix, m);
        return;
    }
    switch (g.mode) {
        case MODE_TRANSLATE:
            matTranslate(m, g.tx, g.ty, g.tz);
            break;
        case MODE_SCALE:
            matScale(m, g.sx, g.sy, g.sz);
            break;
        case MODE_ROTATE:
            matRotateX(m, g.rx);
            matRotateY(m, g.ry);
            matRotateZ(m, g.rz);
            break;
        case MODE_REFLECT:
            matReflectPlane(m, g.reflect);
            break;
        case MODE_SHEAR:
            if (g.shXY) matShear(m, 0, 1, g.shXY);
            if (g.shXZ) matShear(m, 0, 2, g.shXZ);
            if (g.shYX) matShear(m, 1, 0, g.shYX);
            if (g.shYZ) matShear(m, 1, 2, g.shYZ);
            if (g.shZX) matShear(m, 2, 0, g.shZX);
            if (g.shZY) matShear(m, 2, 1, g.shZY);
            break;
        default:
            break;
    }
}

static void appendCompositeStep(const char* label) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s%s", g.compositeLog[0] == '(' ? "" : " -> ", label);
    if (std::strlen(g.compositeLog) + std::strlen(buf) < sizeof(g.compositeLog) - 1)
        std::strncat(g.compositeLog, buf, sizeof(g.compositeLog) - std::strlen(g.compositeLog) - 1);
}

static bool modeAdjustsTransform() {
    return g.mode != MODE_VIEW;
}

static void setTransformMode(TransformMode m, const char* msg) {
    g.mode = m;
    std::snprintf(g.statusMsg, sizeof(g.statusMsg), "%s", msg);
}

static void adjustActive(double delta) {
    if (g.compositeMode && modeAdjustsTransform()) {
        Mat4 step, tmp;
        matIdentity(step);
        switch (g.mode) {
            case MODE_TRANSLATE:
                if (g.axis == AXIS_X) g.tx += delta;
                else if (g.axis == AXIS_Y) g.ty += delta;
                else g.tz += delta;
                matIdentity(step);
                matTranslate(step, g.axis == AXIS_X ? delta : 0, g.axis == AXIS_Y ? delta : 0, g.axis == AXIS_Z ? delta : 0);
                matMultiply(g.compositeMatrix, step, tmp);
                matCopy(tmp, g.compositeMatrix);
                appendCompositeStep("Translate");
                break;
            case MODE_SCALE: {
                double f = 1.0 + delta * 0.05;
                if (g.axis == AXIS_X) g.sx *= f;
                else if (g.axis == AXIS_Y) g.sy *= f;
                else g.sz *= f;
                matIdentity(step);
                matScale(step, g.axis == AXIS_X ? f : 1, g.axis == AXIS_Y ? f : 1, g.axis == AXIS_Z ? f : 1);
                matMultiply(g.compositeMatrix, step, tmp);
                matCopy(tmp, g.compositeMatrix);
                appendCompositeStep("Scale");
                break;
            }
            case MODE_ROTATE:
                if (g.axis == AXIS_X) g.rx += delta * 5;
                else if (g.axis == AXIS_Y) g.ry += delta * 5;
                else g.rz += delta * 5;
                matIdentity(step);
                if (g.axis == AXIS_X) matRotateX(step, delta * 5);
                else if (g.axis == AXIS_Y) matRotateY(step, delta * 5);
                else matRotateZ(step, delta * 5);
                matMultiply(g.compositeMatrix, step, tmp);
                matCopy(tmp, g.compositeMatrix);
                appendCompositeStep("Rotate");
                break;
            case MODE_REFLECT:
                if (delta > 0) g.reflect = (ReflectPlane)((g.reflect + 1) % 3);
                else g.reflect = (ReflectPlane)((g.reflect + 2) % 3);
                matIdentity(step);
                matReflectPlane(step, g.reflect);
                matMultiply(g.compositeMatrix, step, tmp);
                matCopy(tmp, g.compositeMatrix);
                appendCompositeStep("Reflect");
                break;
            case MODE_SHEAR: {
                int target = 0, shear = 1;
                switch (g.shearKind) {
                    case 0: target = 0; shear = 1; break;
                    case 1: target = 0; shear = 2; break;
                    case 2: target = 1; shear = 0; break;
                    case 3: target = 1; shear = 2; break;
                    case 4: target = 2; shear = 0; break;
                    default: target = 2; shear = 1; break;
                }
                double* p = &g.shXY;
                p += g.shearKind;
                *p += delta * 0.1;
                matIdentity(step);
                matShear(step, target, shear, delta * 0.1);
                matMultiply(g.compositeMatrix, step, tmp);
                matCopy(tmp, g.compositeMatrix);
                appendCompositeStep("Shear");
                break;
            }
            default:
                break;
        }
        return;
    }

    switch (g.mode) {
        case MODE_TRANSLATE:
            if (g.axis == AXIS_X) g.tx += delta;
            else if (g.axis == AXIS_Y) g.ty += delta;
            else g.tz += delta;
            break;
        case MODE_SCALE: {
            double f = 1.0 + delta * 0.05;
            if (g.axis == AXIS_X) g.sx *= f;
            else if (g.axis == AXIS_Y) g.sy *= f;
            else g.sz *= f;
            break;
        }
        case MODE_ROTATE:
            if (g.axis == AXIS_X) g.rx += delta * 5;
            else if (g.axis == AXIS_Y) g.ry += delta * 5;
            else g.rz += delta * 5;
            break;
        case MODE_SHEAR: {
            double* p = &g.shXY;
            p += g.shearKind;
            *p += delta * 0.1;
            break;
        }
        case MODE_REFLECT:
            if (delta > 0) g.reflect = (ReflectPlane)((g.reflect + 1) % 3);
            else g.reflect = (ReflectPlane)((g.reflect + 2) % 3);
            break;
        default:
            break;
    }
}

static void adjustCameraAzimuth(double delta) {
    g.camAz += delta * 3.0;
}

static void adjustCameraElevation(double delta) {
    g.camEl += delta * 2.0;
    if (g.camEl > 89.0) g.camEl = 89.0;
    if (g.camEl < -89.0) g.camEl = -89.0;
}

static const char* shearComponentName(int k) {
    static const char* names[] = {"XY", "XZ", "YX", "YZ", "ZX", "ZY"};
    if (k < 0 || k > 5) return "?";
    return names[k];
}

static void setColor(const float c[3]) { glColor3f(c[0], c[1], c[2]); }

static void drawText(int x, int y, const char* s, void* font = GLUT_BITMAP_HELVETICA_12) {
    glRasterPos2i(x, y);
    for (const char* c = s; *c; ++c)
        glutBitmapCharacter(font, *c);
}

static int textWidth(void* font, const char* s) {
    int w = 0;
    for (const char* c = s; *c; ++c)
        w += glutBitmapWidth(font, *c);
    return w;
}

static void drawTextRight(int rightEdge, int y, const char* s, void* font = GLUT_BITMAP_HELVETICA_12) {
    drawText(rightEdge - textWidth(font, s), y, s, font);
}

static void fillRect(int x, int y, int w, int h, float r, float gr, float b, float a = 1.f) {
    glColor4f(r, gr, b, a);
    glBegin(GL_QUADS);
    glVertex2i(x, y);
    glVertex2i(x + w, y);
    glVertex2i(x + w, y + h);
    glVertex2i(x, y + h);
    glEnd();
}

static void fillRectColor(int x, int y, int w, int h, const float c[3]) {
    fillRect(x, y, w, h, c[0], c[1], c[2]);
}

/* 1-pixel line helpers (coordinates: y down from top). */
static void drawHLine(int x, int y, int w) {
    setColor(COL_BORDER);
    fillRect(x, y, w, 1, COL_BORDER[0], COL_BORDER[1], COL_BORDER[2]);
}

static void drawVLine(int x, int y, int h) {
    setColor(COL_BORDER);
    fillRect(x, y, 1, h, COL_BORDER[0], COL_BORDER[1], COL_BORDER[2]);
}

static void beginScreenUI() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, gWinW, gWinH, 0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
}

static void endScreenUI() {
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
}

/* Draw section title; returns y for the first item below it. */
static int drawSectionTitle(int x, int y, const char* title) {
    setColor(COL_ACCENT);
    drawText(x, y, title);
    return y + UI_LINE;
}

struct KeyLabel { char key; const char* label; };

/* One row: key in column 1, description in column 2 (same on left and right panels). */
static int drawKeyLabelRow(int panelX, int y, char key, const char* label) {
    char keyStr[2] = {key, '\0'};
    setColor(COL_ACCENT);
    drawText(panelX + UI_KEY_X, y, keyStr);
    setColor(COL_TEXT);
    drawText(panelX + UI_LABEL_X, y, label);
    return y + UI_LINE;
}

static int drawKeyLabelRows(int panelX, int y, const KeyLabel* rows, int count) {
    for (int i = 0; i < count; ++i)
        y = drawKeyLabelRow(panelX, y, rows[i].key, rows[i].label);
    return y;
}

static int drawTextRows(int x, int y, const char* const* lines, int count) {
    setColor(COL_TEXT);
    for (int i = 0; i < count; ++i) {
        drawText(x, y, lines[i]);
        y += UI_LINE;
    }
    return y;
}

static void drawPanelBackgrounds(const UiLayout& u) {
    fillRectColor(0, 0, u.winW, TOP_H, COL_TOP);
    fillRectColor(0, u.midTop, LEFT_W, u.midH, COL_SIDE);
    fillRectColor(u.winW - RIGHT_W, u.midTop, RIGHT_W, u.midH, COL_SIDE);
    fillRectColor(0, u.learnTop, u.winW, LEARN_H, COL_LEARN);
    fillRectColor(0, u.statusTop, u.winW, STATUS_H, COL_STATUS);

    drawHLine(0, TOP_H - 1, u.winW);
    drawHLine(0, u.learnTop - 1, u.winW);
    drawHLine(0, u.statusTop - 1, u.winW);
    drawVLine(LEFT_W - 1, u.midTop, u.midH);
    drawVLine(u.winW - RIGHT_W, u.midTop, u.midH);
}

static void drawHeaderBar(const UiLayout& u) {
    const int row1 = 26;
    const int row2 = 50;

    setColor(COL_TITLE);
    drawText(UI_PAD, row1, "3D Computer Graphics Learning Simulator", GLUT_BITMAP_HELVETICA_18);

    char topic[180];
    const char* modeLabel = g.compositeMode ? "Composite (accumulating)" : modeName(g.mode);
    std::snprintf(topic, sizeof(topic), "Topic: %s  |  Object: %s  |  Mode: %s  |  Axis: %c",
                  g.wireLearnMode ? "Wireframe Learning" : "Transformations",
                  objectName(g.objectType), modeLabel,
                  g.axis == AXIS_X ? 'X' : (g.axis == AXIS_Y ? 'Y' : 'Z'));
    setColor(COL_MUTED);
    drawText(UI_PAD, row2, topic);

    setColor(COL_ACCENT);
    drawTextRight(u.winW - UI_PAD, row1, "Reset: 0");
    drawTextRight(u.winW - UI_PAD, row2, "Exit: Esc");
}

static void drawLeftSidebar(const UiLayout& u) {
    const int panelX = UI_PAD;
    int y = u.midTop + 20;

    y = drawSectionTitle(panelX, y, "Object Selection");
    static const KeyLabel objects[] = {
        {'1', "Cube"}, {'2', "Pyramid"}, {'3', "Sphere"}, {'4', "Human Figure"}
    };
    y = drawKeyLabelRows(panelX, y, objects, 4);

    y += UI_SECTION;
    y = drawSectionTitle(panelX, y, "Display");
    static const KeyLabel display[] = {
        {'W', "Wireframe toggle"}, {'S', "Solid toggle"}, {'O', "Original only"},
        {'T', "Original + Transformed"}, {'V', "View / camera mode"}
    };
    y = drawKeyLabelRows(panelX, y, display, 5);

    y += UI_SECTION;
    setColor(COL_HINT);
    drawText(panelX + UI_LABEL_X, y, "Cyan  = Original");
    drawText(panelX + UI_LABEL_X, y + UI_LINE, "Orange = Transformed");
}

static void drawRightSidebar(const UiLayout& u) {
    const int panelX = u.winW - RIGHT_W + UI_PAD;
    int y = u.midTop + 20;

    y = drawSectionTitle(panelX, y, "Transformation Modes");
    static const KeyLabel modes[] = {
        {'G', "Translation"}, {'C', "Scaling"}, {'R', "Rotation"},
        {'F', "Reflection"}, {'H', "Shearing"}, {'M', "Composite mode"}
    };
    y = drawKeyLabelRows(panelX, y, modes, 6);

    y += UI_SECTION;
    y = drawSectionTitle(panelX, y, "Controls");
    static const char* controls[] = {
        "X / Y / Z     pick axis",
        "+ / -         adjust amount",
        "Arrow keys    camera / adjust",
        "PgUp / PgDn   zoom",
        "Space         auto-rotate",
        "Mouse drag    orbit camera",
        "Mouse wheel   zoom",
        "Right click   menu"
    };
    drawTextRows(panelX + UI_LABEL_X, y, controls, 8);
}

static void drawViewportHint(const UiLayout& u) {
    int x = LEFT_W + UI_PAD;
    int y = u.midTop + UI_PAD;
    setColor(COL_MUTED);
    drawText(x, y, "Axes: X red  |  Y green  |  Z blue");
}

static void drawLearningPanel(const UiLayout& u) {
    Mat4 M;
    rebuildMatrix(M);
    double vx = g.baseMesh.vertices[0].x;
    double vy = g.baseMesh.vertices[0].y;
    double vz = g.baseMesh.vertices[0].z;
    double ox, oy, oz;
    transformPoint(M, vx, vy, vz, &ox, &oy, &oz);

    const int x = UI_PAD;
    int y = u.learnTop + 18;
    int my = y + UI_LINE;

    setColor(COL_TITLE);
    drawText(x, y, "Learning Zone", GLUT_BITMAP_HELVETICA_18);
    y += UI_LINE + 6;

    char line[256];
    const char* explain = "";
    switch (g.mode) {
        case MODE_TRANSLATE:
            explain = "Translation moves every vertex by the same offset. Shape and size stay the same.";
            break;
        case MODE_SCALE:
            explain = "Scaling multiplies coordinates per axis. Uniform scaling preserves proportions.";
            break;
        case MODE_ROTATE:
            explain = "Rotation turns the object about the origin using trigonometric rotation matrices.";
            break;
        case MODE_REFLECT:
            explain = "Reflection mirrors coordinates across a plane (one axis sign flips).";
            break;
        case MODE_SHEAR:
            explain = "Shearing slides faces along an axis; angles change and the shape distorts.";
            break;
        case MODE_VIEW:
        default:
            explain = g.compositeMode
                ? "Composite ON: pick G/C/R/F/H, then +/- or arrows to append matrix steps."
                : "View mode: arrow keys orbit camera. Pick G/C/R/F/H to transform.";
            break;
    }
    setColor(COL_TEXT);
    drawText(x, y, explain);
    y += UI_LINE;

    if (g.mode == MODE_REFLECT) {
        std::snprintf(line, sizeof(line), "Reflection plane: %s", reflectPlaneName(g.reflect));
        drawText(x, y, line);
        y += UI_LINE;
    }
    if (g.mode == MODE_SHEAR) {
        std::snprintf(line, sizeof(line), "Active shear: %s (Tab to cycle)", shearComponentName(g.shearKind));
        drawText(x, y, line);
        y += UI_LINE;
    }

    std::snprintf(line, sizeof(line),
        "Translation  (%.2f, %.2f, %.2f)", g.tx, g.ty, g.tz);
    drawText(x, y, line);
    y += UI_LINE;
    std::snprintf(line, sizeof(line),
        "Scale        (%.2f, %.2f, %.2f)", g.sx, g.sy, g.sz);
    drawText(x, y, line);
    y += UI_LINE;
    std::snprintf(line, sizeof(line),
        "Rotation deg (%.1f, %.1f, %.1f)", g.rx, g.ry, g.rz);
    drawText(x, y, line);
    y += UI_LINE;
    std::snprintf(line, sizeof(line),
        "Sample vertex (%.2f, %.2f, %.2f) -> (%.2f, %.2f, %.2f)", vx, vy, vz, ox, oy, oz);
    drawText(x, y, line);

    const int matrixX = u.winW - RIGHT_W - 360;
    setColor(COL_MUTED);
    drawText(matrixX, my, "Matrix (column-major 4x4)");
    my += UI_LINE;
    setColor(COL_TEXT);
    for (int col = 0; col < 4; ++col) {
        std::snprintf(line, sizeof(line), "| %8.3f %8.3f %8.3f %8.3f |",
                      M[col * 4 + 0], M[col * 4 + 1], M[col * 4 + 2], M[col * 4 + 3]);
        drawText(matrixX, my, line);
        my += UI_LINE;
    }
    if (g.compositeMode) {
        std::snprintf(line, sizeof(line), "Composite steps:%s", g.compositeLog);
        setColor(COL_ACCENT);
        drawText(x, y + UI_LINE, line);
    }
}

static void drawStatusBar(const UiLayout& u) {
    setColor(COL_OK);
    drawText(UI_PAD, u.statusTop + STATUS_H - 10, g.statusMsg);
}

static void drawPanelUI() {
    UiLayout u = uiLayout();
    beginScreenUI();
    drawPanelBackgrounds(u);
    drawHeaderBar(u);
    drawLeftSidebar(u);
    drawRightSidebar(u);
    drawViewportHint(u);
    drawLearningPanel(u);
    drawStatusBar(u);
    endScreenUI();
}

static void drawGridAndAxes() {
    glDisable(GL_LIGHTING);

    /* XZ ground grid at y = 0 */
    glLineWidth(1.f);
    glColor3f(0.22f, 0.26f, 0.34f);
    glBegin(GL_LINES);
    for (int i = -10; i <= 10; ++i) {
        glVertex3f((float)i, 0.f, -10.f);
        glVertex3f((float)i, 0.f, 10.f);
        glVertex3f(-10.f, 0.f, (float)i);
        glVertex3f(10.f, 0.f, (float)i);
    }
    glEnd();

    const float axisLen = 3.5f;
    glLineWidth(3.f);
    glBegin(GL_LINES);
    /* +X red, +Y green, +Z blue (right-handed, Y up) */
    glColor3f(1.f, 0.15f, 0.15f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(axisLen, 0.f, 0.f);
    glColor3f(0.15f, 1.f, 0.25f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, axisLen, 0.f);
    glColor3f(0.2f, 0.45f, 1.f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, axisLen);
    glEnd();

    /* Negative axis ticks for orientation */
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glColor3f(0.55f, 0.12f, 0.12f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(-axisLen * 0.35f, 0.f, 0.f);
    glColor3f(0.12f, 0.55f, 0.15f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, -axisLen * 0.35f, 0.f);
    glColor3f(0.12f, 0.25f, 0.55f);
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, -axisLen * 0.35f);
    glEnd();
}

static void drawReflectionPlaneHint() {
    if (g.mode != MODE_REFLECT) return;
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.95f, 0.35f, 0.85f, 0.22f);
    glBegin(GL_QUADS);
    if (g.reflect == REFLECT_YZ) {
        glVertex3f(0, -3, -3); glVertex3f(0, 3, -3); glVertex3f(0, 3, 3); glVertex3f(0, -3, 3);
    } else if (g.reflect == REFLECT_XZ) {
        glVertex3f(-3, 0, -3); glVertex3f(3, 0, -3); glVertex3f(3, 0, 3); glVertex3f(-3, 0, 3);
    } else {
        glVertex3f(-3, -3, 0); glVertex3f(3, -3, 0); glVertex3f(3, 3, 0); glVertex3f(-3, 3, 0);
    }
    glEnd();
    glDisable(GL_BLEND);
}

static void drawMeshObject(const Mesh* mesh, float solidR, float solidG, float solidB,
                           float wireR, float wireG, float wireB) {
    if (g.solidOn)
        meshDrawSolid(mesh, solidR, solidG, solidB);
    if (g.wireframeOn || g.wireLearnMode)
        meshDrawWireframe(mesh, wireR, wireG, wireB, g.wireLearnMode);
}

static void draw3DViewport() {
    UiLayout u = uiLayout();
    const int vpW = u.winW - LEFT_W - RIGHT_W;
    const int vpH = u.midH;
    if (vpW < 1 || vpH < 1) return;
    glViewport(LEFT_W, u.vpBottom, vpW, vpH);
    glScissor(LEFT_W, u.vpBottom, vpW, vpH);
    glEnable(GL_SCISSOR_TEST);

    glClearColor(0.05f, 0.06f, 0.08f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)vpW / (double)vpH, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const double lookY = 0.45;
    const double el = g.camEl * M_PI / 180.0;
    const double az = g.camAz * M_PI / 180.0;
    const double ex = g.camDist * std::cos(el) * std::sin(az);
    const double ey = lookY + g.camDist * std::sin(el);
    const double ez = g.camDist * std::cos(el) * std::cos(az);
    gluLookAt(ex, ey, ez, 0.0, lookY, 0.0, 0.0, 1.0, 0.0);

    GLfloat lightPos[] = {4.f, 6.f, 5.f, 1.f};
    GLfloat lightAmb[] = {0.25f, 0.25f, 0.28f, 1.f};
    GLfloat lightDiff[] = {0.85f, 0.85f, 0.88f, 1.f};
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);

    drawGridAndAxes();
    drawReflectionPlaneHint();

    Mat4 M;
    rebuildMatrix(M);
    meshCopyTransformed(&g.baseMesh, &g.transformedMesh, M);

    int sideBySide = (g.objectType == OBJ_HUMAN && g.showBoth);
    float off = sideBySide ? 2.2f : 0.f;

    if (!g.showBoth || sideBySide) {
        glPushMatrix();
        if (sideBySide) glTranslated(-off, 0, 0);
        /* Original: teal solid + bright cyan wire */
        drawMeshObject(&g.baseMesh, 0.15f, 0.72f, 0.88f, 0.1f, 1.0f, 1.0f);
        glPopMatrix();
    }

    if (g.showBoth) {
        glPushMatrix();
        if (sideBySide) glTranslated(off, 0, 0);
        /* Transformed: orange solid + yellow wire */
        drawMeshObject(&g.transformedMesh, 0.95f, 0.42f, 0.08f, 1.0f, 0.88f, 0.15f);
        glPopMatrix();
    } else if (g.mode != MODE_VIEW) {
        drawMeshObject(&g.transformedMesh, 0.95f, 0.42f, 0.08f, 1.0f, 0.88f, 0.15f);
    }

    glDisable(GL_SCISSOR_TEST);
}

static void display() {
    syncWindowSize();
    glViewport(0, 0, gWinW, gWinH);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.05f, 0.06f, 0.08f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw3DViewport();
    drawPanelUI();
    glutSwapBuffers();
}

static void timer(int) {
    if (g.autoSpin) {
        g.camAz += 0.6;
        if (g.mode == MODE_ROTATE && !g.compositeMode)
            g.ry += 0.8;
    }
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

static void reshape(int w, int h) {
    (void)w;
    (void)h;
    syncWindowSize();
    glutPostRedisplay();
}

static void setObject(ObjectType t) {
    g.objectType = t;
    meshBuild(g.objectType, &g.baseMesh);
    std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Selected object: %s", objectName(t));
}

static void pickShearForAxis(AxisChoice ax) {
    if (ax == AXIS_X) g.shearKind = 0;
    else if (ax == AXIS_Y) g.shearKind = 2;
    else g.shearKind = 4;
}

static void pickReflectForAxis(AxisChoice ax) {
    if (ax == AXIS_X) g.reflect = REFLECT_YZ;
    else if (ax == AXIS_Y) g.reflect = REFLECT_XZ;
    else g.reflect = REFLECT_XY;
}

static void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 27: exit(0); break;
        case '1': setObject(OBJ_CUBE); break;
        case '2': setObject(OBJ_PYRAMID); break;
        case '3': setObject(OBJ_SPHERE); break;
        case '4': setObject(OBJ_HUMAN); break;
        case 'w': case 'W':
            g.wireframeOn = !g.wireframeOn;
            g.wireLearnMode = g.wireframeOn;
            if (!g.wireframeOn && !g.solidOn) g.solidOn = true;
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Wireframe %s", g.wireframeOn ? "ON" : "OFF");
            break;
        case 's': case 'S':
            g.solidOn = !g.solidOn;
            if (!g.wireframeOn && !g.solidOn) g.wireframeOn = true;
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Solid shading %s", g.solidOn ? "ON" : "OFF");
            break;
        case 'o': case 'O':
            g.showBoth = false;
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Showing original mesh only");
            break;
        case 't': case 'T':
            g.showBoth = true;
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Showing original + transformed");
            break;
        case 'x': case 'X':
            g.axis = AXIS_X;
            if (g.mode == MODE_REFLECT) pickReflectForAxis(AXIS_X);
            if (g.mode == MODE_SHEAR) pickShearForAxis(AXIS_X);
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Active axis: X");
            break;
        case 'y': case 'Y':
            g.axis = AXIS_Y;
            if (g.mode == MODE_REFLECT) pickReflectForAxis(AXIS_Y);
            if (g.mode == MODE_SHEAR) pickShearForAxis(AXIS_Y);
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Active axis: Y");
            break;
        case 'z': case 'Z':
            g.axis = AXIS_Z;
            if (g.mode == MODE_REFLECT) pickReflectForAxis(AXIS_Z);
            if (g.mode == MODE_SHEAR) pickShearForAxis(AXIS_Z);
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Active axis: Z");
            break;
        case 'r': case 'R':
            setTransformMode(MODE_ROTATE, "Rotation mode (R). +/- or Left/Right adjust angle.");
            break;
        case 'g': case 'G':
            setTransformMode(MODE_TRANSLATE, "Translation mode (G). +/- or arrows move along axis.");
            break;
        case 'c': case 'C':
            setTransformMode(MODE_SCALE, "Scaling mode (C). +/- or arrows scale along axis.");
            break;
        case 'f': case 'F':
            setTransformMode(MODE_REFLECT, "Reflection mode (F). X/Y/Z picks plane; +/- cycles plane.");
            break;
        case 'h': case 'H':
            setTransformMode(MODE_SHEAR, "Shearing mode (H). Tab cycles component; +/- adjusts factor.");
            break;
        case 'm': case 'M':
            g.compositeMode = !g.compositeMode;
            if (g.compositeMode) {
                matIdentity(g.compositeMatrix);
                std::snprintf(g.compositeLog, sizeof(g.compositeLog), "(start)");
                std::snprintf(g.statusMsg, sizeof(g.statusMsg),
                              "Composite ON — use G/C/R/F/H then +/- to stack transforms");
            } else {
                std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Composite OFF — single transform preview");
            }
            break;
        case 'v': case 'V':
            setTransformMode(MODE_VIEW, "View mode (V). Arrow keys orbit camera.");
            break;
        case '+': case '=':
            adjustActive(1.0);
            break;
        case '-': case '_':
            adjustActive(-1.0);
            break;
        case '0':
            resetAll();
            break;
        case ' ':
            g.autoSpin = !g.autoSpin;
            std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Auto-rotate %s", g.autoSpin ? "ON" : "OFF");
            break;
        case '\t':
            if (g.mode == MODE_SHEAR) {
                g.shearKind = (g.shearKind + 1) % 6;
                std::snprintf(g.statusMsg, sizeof(g.statusMsg),
                              "Shear component: %s", shearComponentName(g.shearKind));
            }
            break;
        default: break;
    }
    glutPostRedisplay();
}

static void special(int key, int, int) {
    if (modeAdjustsTransform()) {
        switch (key) {
            case GLUT_KEY_LEFT: adjustActive(-1.0); break;
            case GLUT_KEY_RIGHT: adjustActive(1.0); break;
            case GLUT_KEY_UP: adjustActive(1.0); break;
            case GLUT_KEY_DOWN: adjustActive(-1.0); break;
            default: break;
        }
    } else {
        switch (key) {
            case GLUT_KEY_LEFT: adjustCameraAzimuth(-1.0); break;
            case GLUT_KEY_RIGHT: adjustCameraAzimuth(1.0); break;
            case GLUT_KEY_UP: adjustCameraElevation(1.0); break;
            case GLUT_KEY_DOWN: adjustCameraElevation(-1.0); break;
            default: break;
        }
    }
    switch (key) {
        case GLUT_KEY_PAGE_UP:
            g.camDist -= 0.35;
            if (g.camDist < 2.5) g.camDist = 2.5;
            break;
        case GLUT_KEY_PAGE_DOWN:
            g.camDist += 0.35;
            break;
        default: break;
    }
    glutPostRedisplay();
}

static void mouseButton(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        mouseToPixels(x, y);
        g.dragging = (state == GLUT_DOWN);
        g.lastX = x;
        g.lastY = y;
    }
}

static void mouseMotion(int x, int y) {
    if (!g.dragging) return;
    mouseToPixels(x, y);
    g.camAz += (x - g.lastX) * 0.4;
    g.camEl += (y - g.lastY) * 0.3;
    g.lastX = x;
    g.lastY = y;
    glutPostRedisplay();
}

#if defined(_WIN32) && !defined(__APPLE__)
static void mouseWheel(int wheel, int dir, int, int) {
    (void)wheel;
    g.camDist -= dir * 0.4;
    if (g.camDist < 2) g.camDist = 2;
    glutPostRedisplay();
}
#endif

static void menuPick(int id) {
    switch (id) {
        case 1: setObject(OBJ_CUBE); break;
        case 2: setObject(OBJ_PYRAMID); break;
        case 3: setObject(OBJ_SPHERE); break;
        case 4: setObject(OBJ_HUMAN); break;
        case 10: g.mode = MODE_TRANSLATE; break;
        case 11: g.mode = MODE_SCALE; break;
        case 12: g.mode = MODE_ROTATE; break;
        case 13: g.mode = MODE_REFLECT; break;
        case 14: g.mode = MODE_SHEAR; break;
        case 20: g.wireframeOn = true; break;
        case 21: g.solidOn = true; break;
        case 22: g.showBoth = true; break;
        case 30: resetAll(); break;
        default: break;
    }
    glutPostRedisplay();
}

static int buildMenu() {
    int obj = glutCreateMenu(menuPick);
    glutAddMenuEntry("Cube", 1);
    glutAddMenuEntry("Pyramid", 2);
    glutAddMenuEntry("Sphere", 3);
    glutAddMenuEntry("Human Figure", 4);
    int trans = glutCreateMenu(menuPick);
    glutAddMenuEntry("Translation", 10);
    glutAddMenuEntry("Scaling", 11);
    glutAddMenuEntry("Rotation", 12);
    glutAddMenuEntry("Reflection", 13);
    glutAddMenuEntry("Shearing", 14);
    int disp = glutCreateMenu(menuPick);
    glutAddMenuEntry("Wireframe", 20);
    glutAddMenuEntry("Solid", 21);
    glutAddMenuEntry("Original + Transformed", 22);
    int root = glutCreateMenu(menuPick);
    glutAddSubMenu("Object", obj);
    glutAddSubMenu("Transformation", trans);
    glutAddSubMenu("Display", disp);
    glutAddMenuEntry("Reset", 30);
    return root;
}

int main(int argc, char** argv) {
    std::memset(&g, 0, sizeof(g));
    g.objectType = OBJ_CUBE;
    g.mode = MODE_VIEW;
    g.axis = AXIS_X;
    g.wireframeOn = true;
    g.solidOn = true;
    g.showBoth = true;
    g.wireLearnMode = true;
    resetAll();

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    int screenW = glutGet(GLUT_SCREEN_WIDTH);
    int screenH = glutGet(GLUT_SCREEN_HEIGHT);
    int startW = WIN_W;
    int startH = WIN_H;
    if (screenW > 0 && screenH > 0) {
        startW = screenW * 92 / 100;
        startH = screenH * 90 / 100;
        if (startW < WIN_W) startW = WIN_W;
        if (startH < WIN_H) startH = WIN_H;
    }
    glutInitWindowSize(startW, startH);
    glutInitWindowPosition((screenW - startW) / 2, (screenH - startH) / 2);
    glutCreateWindow("3D Computer Graphics Learning Simulator");
    glutIgnoreKeyRepeat(1);
    syncWindowSize();

    meshBuild(g.objectType, &g.baseMesh);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutMouseFunc(mouseButton);
    glutMotionFunc(mouseMotion);
#if defined(_WIN32) && !defined(__APPLE__)
    glutMouseWheelFunc(mouseWheel);
#endif
    glutTimerFunc(16, timer, 0);

    glutSetMenu(buildMenu());
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    std::snprintf(g.statusMsg, sizeof(g.statusMsg), "Ready. Right-click for menu.");

    glutMainLoop();
    return 0;
}
