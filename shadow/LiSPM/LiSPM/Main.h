//Copyright and Disclaimer:
//This code is copyright Vienna University of Technology, 2004.

#ifndef MainH
#define MainH

#include <Windows.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "MathStuff.h"
#include "glInterface.h"
#include "SceneData.h"

extern int useLispSM; // toggle between LispSM and Uniform shadow mapping
//CHANGED
extern int useBodyVec;

extern double nearDist; // eye near plane distance (often 1)
extern Vector3 eyePos;  // eye position 
extern Vector3 viewDir;  // eye view dir 
extern Vector3 lightDir;  // light dir 

extern Matrix4x4 eyeView; // eye view matrix
extern Matrix4x4 eyeProjection; // eye projection matrix
extern Matrix4x4 eyeProjView; //= eyeProjection*eyeView
extern Matrix4x4 invEyeProjView; //= eyeProjView^(-1)

extern Matrix4x4 lightView; // light view matrix
extern Matrix4x4 lightProjection; // light projection matrix

extern void updateLightMtx(void); // updates lightView & lightProjection

#endif
