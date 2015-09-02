//Copyright and Disclaimer:
//This code is copyright Vienna University of Technology, 2004.

#ifndef SceneDataH
#define SceneDataH

#include "MathStuff.h"

//Axis-Aligned Bounding Box of the scene
extern const struct AABox sceneAABox;

typedef GLfloat Vector4[4];
//initialize Vector4 with for float values
extern void setVector4(Vector4, const float, const float, const float, const float);

//define two colors: black & white
extern const Vector4 black;
extern const Vector4 white;

//draws the scene :)
extern void drawScene();

//draws a volume with 8 corner points, like a box or a frustum (GL_TRIANGLE_STRIP)
extern void drawBoxVolume(const Vector3x8);
//draws the edges of a volume with 8 corner points, like a box or a frustum (GL_LINES)
extern void drawLineBoxVolume(const Vector3x8);

#endif
