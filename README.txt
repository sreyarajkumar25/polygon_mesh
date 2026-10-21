3D Computer Graphics Learning Simulator
=========================================

Overview
--------
Interactive educational OpenGL/GLUT application for learning 3D transformations
using custom polygon meshes (vertices, edges, faces). Objects: cube, pyramid,
sphere (latitude/longitude mesh), and a multi-part 3D human figure.

Works on macOS (Terminal + Makefile) and Windows (Code::Blocks + MinGW).


================================================================================
macOS — BUILD AND RUN (recommended on your Mac)
================================================================================

Requirements
------------
- macOS with Xcode Command Line Tools (includes clang and Apple GLUT/OpenGL)

One-time setup (if you have not compiled C++ before):
  Open Terminal and run:
    xcode-select --install
  Click Install when prompted.


Quick start
-----------
1. Open Terminal.
2. Go to the project folder, for example:
     cd ~/Desktop/CG
3. Build:
     chmod +x build-mac.sh
     ./build-mac.sh
4. Run the app:
     ./bin/ComputerGraphicsLab

Build and run in one step:
     ./build-mac.sh --run

Alternative (using make directly):
     make
     make run

If the window is blank or behind other windows, click the dock icon titled
"3D Computer Graphics Learning Simulator".


macOS notes
-----------
- Uses Apple's built-in GLUT and OpenGL frameworks (no Homebrew required).
- Mouse wheel zoom is not available in Apple's GLUT; use Page Up / Page Down
  or left-drag to orbit the camera.
- You may see compiler warnings about deprecated OpenGL on Mac; that is normal
  for fixed-function GLUT labs. GL_SILENCE_DEPRECATION is set in the Makefile.


================================================================================
Windows — Code::Blocks
================================================================================

Requirements
------------
- Code::Blocks with MinGW
- freeglut (recommended) or GLUT for MinGW
- OpenGL and GLU (included with MinGW)

Typical freeglut layout:
  C:\Program Files\CodeBlocks\MinGW\include\GL\freeglut.h
  C:\Program Files\CodeBlocks\MinGW\lib\libfreeglut.a

Steps
-----
1. Open ComputerGraphicsLab.cbp in Code::Blocks.
2. Build (Ctrl+F9), then Run (Ctrl+F10).

If linking fails, add the folder containing libfreeglut.a under:
  Settings -> Compiler -> Search directories -> Linker


================================================================================
Keyboard controls (Mac and Windows)
================================================================================

1 / 2 / 3 / 4     Select cube / pyramid / sphere / human figure
W                 Toggle wireframe (wireframe learning mode)
S                 Toggle solid shading
O                 Show original object only
T                 Show original and transformed together
X / Y / Z         Select active axis
G                 Translation mode
C                 Scaling mode
R                 Rotation mode
F                 Reflection mode
H                 Shearing mode
M                 Toggle composite transformation mode
Tab (in shear)    Cycle shear component (XY, XZ, YX, YZ, ZX, ZY)
+ / -             Increase / decrease active parameter
Arrow keys        Adjust parameter; Up/Down also tilt camera
Page Up / Down    Zoom in / out (especially on Mac)
Space             Auto-rotate camera
0                 Reset transforms and camera
Esc               Exit

Mouse
-----
Left drag         Orbit camera
Mouse wheel       Zoom (Windows / freeglut)
Right click       Context menu


Learning panel
--------------
Bottom of the window: explanation, 4x4 matrix, sample vertex, composite log.

Human figure: press 4 and T to see original and transformed side by side.


Project files
-------------
main.cpp                 Application and UI
objects.cpp / .h         Mesh geometry
transformations.cpp/.h   Matrix math
Makefile                 macOS / Linux build
build-mac.sh             macOS helper script
ComputerGraphicsLab.cbp  Windows Code::Blocks project
README.txt               This file
