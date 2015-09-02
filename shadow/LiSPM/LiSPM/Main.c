//Copyright and Disclaimer:
//This code is copyright Vienna University of Technology, 2004.

#include "Main.h"

int useLispSM = 0;
int usePolygonOffset = 1;
int showSceneInLightView = 1;
int useBodyVec = 1;

Matrix4x4 eyeView;
Matrix4x4 eyeProjection;
Matrix4x4 eyeProjView; //= eyeProjection*eyeView
Matrix4x4 invEyeProjView; //= eyeProjView^(-1)

Matrix4x4 lightView;
Matrix4x4 lightProjection;

double nearDist = 1.0;
Vector3 eyePos = {0.0, 3.0, 0.0};  // eye position 
Vector3 viewDir = {0.0, 0.0, -1.0};  // eye view dir 
Vector3 lightDir = {0.0, -0.99, 0.01};  // light dir 

int winWidth, winHeight;

int depthMapSize = 512;
GLuint depthTexture;

int showHelp = 0;

//mouse navigation state
int xEyeBegin, yEyeBegin, movingEye = 0;
int xLightBegin, yLightBegin, movingLight = 0;

const GLfloat globalAmbient[] = {0.5, 0.5, 0.5, 1.0};

enum TView {
	EYE,
	EYE_SHADOWED,
	LIGHT,
	DEPTH_TEX
} view;

void prepareDepthTextureState(void) {
	const int border = 0;
	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(border,border,depthMapSize-border*2,depthMapSize-border*2);
	glScissor(border,border,depthMapSize-border*2,depthMapSize-border*2);
	glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
	glShadeModel(GL_FLAT);
	glDisable(GL_LIGHTING);
	glEnable(GL_ALPHA_TEST);
	glDisable(GL_FOG);
	if(usePolygonOffset) {
		glEnable(GL_POLYGON_OFFSET_FILL);
	}
	else {
		glCullFace(GL_FRONT);
	}
}

void unprepareDepthTextureState(void) {
	if(usePolygonOffset) {
		glDisable(GL_POLYGON_OFFSET_FILL);
	}
	else {
		glCullFace(GL_BACK);
	}
	glDisable(GL_ALPHA_TEST);
	glEnable(GL_FOG);
	glEnable(GL_LIGHTING);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	glShadeModel(GL_SMOOTH);
	glViewport(0,0,winWidth,winHeight);
	glScissor(0,0,winWidth,winHeight);
}

void saveToDepthTexture(void) {
	glBindTexture(GL_TEXTURE_2D,depthTexture);
	glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,depthMapSize,depthMapSize);
}

void prepareShadowMapping(void) {
	const GLdouble x[] = {1.0, 0.0, 0.0, 0.0};
	const GLdouble y[] = {0.0, 1.0, 0.0, 0.0};
	const GLdouble z[] = {0.0, 0.0, 1.0, 0.0};
	const GLdouble w[] = {0.0, 0.0, 0.0, 1.0};
	const GLdouble bias[] = {0.5, 0.0, 0.0, 0.0, 
				 			 0.0, 0.5, 0.0, 0.0,
							 0.0, 0.0, 0.5, 0.0,
							 0.5, 0.5, 0.5, 1.0};

	glActiveTextureARB(GL_TEXTURE1);

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
	
	glTexGendv(GL_S, GL_EYE_PLANE, x);
	glTexGendv(GL_T, GL_EYE_PLANE, y);
	glTexGendv(GL_R, GL_EYE_PLANE, z);
	glTexGendv(GL_Q, GL_EYE_PLANE, w);

	//here the different shadow algorithms are called
	updateLightMtx();

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixd(bias);
	glMultMatrixd(lightProjection);
	glMultMatrixd(lightView);
	glMatrixMode(GL_MODELVIEW);

	glBindTexture(GL_TEXTURE_2D,depthTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_COMPARE_R_TO_TEXTURE_ARB);

	glActiveTextureARB(GL_TEXTURE0);
}

void unprepareShadowMapping(void) {
	glActiveTextureARB(GL_TEXTURE1);
	//Disable textures and texgen
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_MODE_ARB,GL_NONE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
	glDisable(GL_TEXTURE_GEN_Q);
	glActiveTextureARB(GL_TEXTURE0);
}
void setupLightView(void) {
	updateLightMtx();

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(lightProjection);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(lightView);
}

void updateEyeMtx(void) {
	const Vector3 up = {0.0, 1.0, 0.0};
	look(eyeView,eyePos,viewDir,up);
	mult(eyeProjView,eyeProjection,eyeView); //eyeProjView = eyeProjection*eyeView
	invert(invEyeProjView,eyeProjView); //invert matrix
}

void setupEyeView(void) {
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(eyeProjection);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(eyeView);
}

void drawEyeView(void) {
	Vector4 lightPos;
	setVector4(lightPos, -lightDir[0], -lightDir[1], -lightDir[2], 0.0);
	glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
	drawScene();
}

void drawLightView(void) {
	Vector4 lightPos;
	Vector3 startPos;
	Vector3x8 v;
	setVector4(lightPos, -lightDir[0], -lightDir[1], -lightDir[2], 0.0);
	glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
	glDisable(GL_FOG);
	if(showSceneInLightView)
    	drawScene();

	glDisable(GL_LIGHTING);
	//draw view vector in red
	glColor3f(1,0,0);
	glBegin(GL_LINES);
		linCombVector3(startPos,eyePos,viewDir,nearDist);
		glVertex3dv(startPos);
		linCombVector3(startPos,eyePos,viewDir,nearDist*70.0);
		glVertex3dv(startPos);
	glEnd();

	//draw view frustum
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	
	glEnable(GL_BLEND);
	calcViewFrustumWorldCoord(v,invEyeProjView);
	glColor4f(1,1,1,0.5);
	drawBoxVolume(v);
	glDisable(GL_BLEND);

	glColor4f(0,0,0,1);
	glEnable(GL_POLYGON_OFFSET_LINE);
	drawLineBoxVolume(v);
	glDisable(GL_POLYGON_OFFSET_LINE);

	glEnable(GL_LIGHTING);
	glEnable(GL_FOG);
}

void generateDepthTexture(void) {
	setupLightView();
	prepareDepthTextureState();	
	glDisable(GL_LIGHTING);
	drawScene();
	glEnable(GL_LIGHTING);
	saveToDepthTexture();
	unprepareDepthTextureState();
}

void drawDepthTexture(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D,depthTexture);
	glDisable(GL_LIGHTING);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glBegin(GL_QUADS);    
		glColor3f(0,1,0);
		glTexCoord2f(0,1);
		glVertex2f(-1,1);
		glTexCoord2f(0,0);
		glVertex2f(-1,-1);
		glTexCoord2f(1,0);
		glVertex2f(1,-1);
		glTexCoord2f(1,1);
		glVertex2f(1,1);
	glEnd();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
}

void output(const int x, const int y, const char *string) {
	if(string != 0) {
		int len, i;
		glRasterPos2f(x,y);
		len = (int) strlen(string);
		for (i = 0; i < len; i++) {
			glutBitmapCharacter(GLUT_BITMAP_8_BY_13,string[i]);
		}
	}
}

void begin2D(void) {
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0,winWidth,winHeight,0);
}


void end2D(void) {
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);
}

void drawHelpMessage(void) {
	const char *message[] = {
		"Help information",
		"",
		"'MOUSE-LEFT'  - eye look up/down left/right",
		"'MOUSE-RIGHT' - light direction angle incr/decr rotate cw/ccw",
		"'W'           - move forward",
		"'S'           - move backward",
		"'+'           - move up",
		"'-'           - move down",
		"",
		"'F1'          - shows and dismisses this message",
		"'F2'          - show eye view with shadows",
		"'F3'          - show light view",
		"'F4'          - show depth texture",
		"'F5'          - show eye view without shadows",
		"'L'           - toggle usm/lispsm",
		"'P'           - toggle use polygon offset / render backfaces to depth texture",
		"'T'           - toggle show terrain in light view",
		"'B'           - toggle use body vector as up vector / us viewDir as up vector",
		0
	};
	int i;
	int x = 40, y = 42;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(0.0,1.0,0.0,0.2);  /* 20% green. */

	/* Drawn clockwise because the flipped Y axis flips CCW and CW. */
	glRecti(winWidth-30,30,30,winHeight-30);

	glDisable(GL_BLEND);

	glColor3f(1.0,1.0,1.0);
	for(i = 0; message[i] != 0; i++) {
		if(message[i][0] == '\0') {
			y += 7;
		} else {
			output(x,y,message[i]);
			y += 14;
		}
	}

}

void display(void) {
	char *msg1 = 0, *msg2 = 0, *msg3 = 0;
	updateEyeMtx(); //bring eye modelview matrix up-to-date
	if(useLispSM) {
		msg1 = useBodyVec ? "LispSM with body vector":"LispSM";
	}
	else {
		msg1 = useBodyVec ? "UNIFORM-SM with body vector":"UNIFORM-SM";
	}
	if(usePolygonOffset) {
		msg3 = "polygon offset";
	}
	else {
		msg3 = "back-side";
	}
	switch(view) {
	case LIGHT:
		setupLightView();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawLightView();
		msg2 = "light view";
		break;
	case EYE:
		setupEyeView();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawEyeView();
		msg2 = "eye view";
		break;
	case EYE_SHADOWED:
		generateDepthTexture();
		setupEyeView();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		prepareShadowMapping();
		drawEyeView();
		unprepareShadowMapping();
		msg2 = "shadowed eye view";
		break;
	case DEPTH_TEX:
		generateDepthTexture();
		drawDepthTexture();
		msg2 = "depth texture view";
		break;
	};

	begin2D();
	if(showHelp) {
		drawHelpMessage();
	}
	else {
		glColor3f(1.0,1.0,1.0);
		if(view != EYE) {
			output(10,winHeight-50,msg1);
			output(10,winHeight-30,msg3);
		}
		output(10,winHeight-10,msg2);
	}
	end2D();
	glutSwapBuffers();
}

#pragma warning( disable : 4100 )
void keyboard(const unsigned char c, const int x, const int y) {
	switch(c) {
	case 27:
		exit(0);
		break;
	case 'h':
	case 'H':
		showHelp = !showHelp;
		break;
	case 'l':
	case 'L':
		useLispSM = !useLispSM;
		break;
	case 'b':
	case 'B':
		useBodyVec = !useBodyVec;
		break;
	case 'p':
	case 'P':
		usePolygonOffset = !usePolygonOffset;
		break;
	case 't':
	case 'T':
		showSceneInLightView = !showSceneInLightView;
		break;
	case 'w':
	case 'W':
		linCombVector3(eyePos,eyePos,viewDir,0.3);
		break;
	case 's':
	case 'S':
		linCombVector3(eyePos,eyePos,viewDir,-0.3);
		break;
	case '+':
		eyePos[1] += 0.3;
		break;
	case '-':
		eyePos[1] -= 0.3;
		break;
	default:
		return;
	}
	glutPostRedisplay();
}

void special(const int c, const int x, const int y) {
	switch(c) {
	case GLUT_KEY_F1:
		showHelp = !showHelp;
		break;
	case GLUT_KEY_F2:
		view = EYE_SHADOWED;
		break;
	case GLUT_KEY_F3:
		view = LIGHT;
		break;
	case GLUT_KEY_F4:
		view = DEPTH_TEX;
		break;
	case GLUT_KEY_F5:
		view = EYE;
		break;
	default:
		return;
	}
	glutPostRedisplay();
}
#pragma warning( default : 4100 )

void reshape(const int w, const int h) {
	double winAspectRatio;

	winWidth = w;
	winHeight = h;
	glViewport(0,0,w,h);
	glScissor(0,0,w,h);
	winAspectRatio = (double) winHeight / (double) winWidth;
	perspectiveDeg(eyeProjection,60.0,1.0/winAspectRatio,nearDist,70.0);
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
	const int eyeButton = GLUT_LEFT_BUTTON;
	const int lightButton = GLUT_RIGHT_BUTTON;

	if(button == eyeButton && state == GLUT_DOWN) {
		movingEye = 1;
		xEyeBegin = x;
		yEyeBegin = y;
	}
	if(button == eyeButton && state == GLUT_UP) {
		movingEye = 0;
	}
	if(button == lightButton && state == GLUT_DOWN) {
		movingLight = 1;
		xLightBegin = x;
		yLightBegin = y;
	}
	if(button == lightButton && state == GLUT_UP) {
		movingLight = 0;
		glutPostRedisplay();
	}
}

void motion(int x, int y) {
	if(movingEye) {
		static double eyeXAngle = 0.0;
		static double eyeYAngle = 0.0;
		eyeXAngle -= 0.015*(x - xEyeBegin);
		eyeYAngle -= 0.015*(y - yEyeBegin);
		clamp(&eyeYAngle,-PI_2+0.1,PI_2-0.1);
		xEyeBegin = x;
		yEyeBegin = y;
		{
			const double cosM = cos(eyeYAngle);
			const double sinM = sin(eyeYAngle);
			const double cosN = cos(eyeXAngle);
			const double sinN = sin(eyeXAngle);
			viewDir[0] = -cosM*sinN;
			viewDir[1] = sinM;
			viewDir[2] = -cosM*cosN;
		}
		glutPostRedisplay();
	}
	if(movingLight) {
		static double lightXAngle = 0.0;
		static double lightYAngle = -0.99;
		lightXAngle -= 0.005*(x - xLightBegin);
		lightYAngle += 0.015*(y - yLightBegin);
		clamp(&lightYAngle,-PI_2+0.01,0.0);
		xLightBegin = x;
		yLightBegin = y;
		{
			const double cosM = cos(lightYAngle);
			const double sinM = sin(lightYAngle);
			const double cosN = cos(lightXAngle);
			const double sinN = sin(lightXAngle);
			lightDir[0] = -cosM*sinN;
			lightDir[1] = sinM;
			lightDir[2] = -cosM*cosN;
		}
		glutPostRedisplay();
	}
}

void initExtensions(void) {
	GLenum err = glewInit();
	if(GLEW_OK != err) {
		/* problem: glewInit failed, something is seriously wrong */
		fprintf(stderr,"Error: %s\n",glewGetErrorString(err));
		exit(1);
	}

	if(!GLEW_ARB_multitexture) {
		printf("I require the GL_ARB_multitexture OpenGL extension to work.\n");
		exit(1);
	}
}
void initGLstate(void) {
	const GLfloat lightColor[] = {1.0, 1.0, 1.0, 1.0};
	const GLfloat fogColor[] = {0.6, 0.6, 0.8, 1.0};
	glClearColor(0.6,0.6,0.8,1.0);
	
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(GL_PACK_ALIGNMENT,1);

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbient);

	glAlphaFunc(GL_GREATER,0.9);

	glClearDepth(1.0);
	glDepthRange(0.0,1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, black);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);

	glShadeModel(GL_SMOOTH);
	glMaterialf(GL_FRONT, GL_SHININESS, 30.0);

	glFogi(GL_FOG_MODE,GL_LINEAR);
	glFogf(GL_FOG_START,20.0);
	glFogf(GL_FOG_END,70.0);
	glFogfv(GL_FOG_COLOR,fogColor);
	glEnable(GL_FOG);

	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);

	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);
	glEnable(GL_SCISSOR_TEST);
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,1);
	glEnable(GL_NORMALIZE);

	//for each scene other values are suitable
	glPolygonOffset(2.0,4.0);

	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);

	//generate depth texture and set its properties
	glGenTextures(1,&depthTexture);
	glBindTexture(GL_TEXTURE_2D,depthTexture);
	glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT,depthMapSize,depthMapSize,
		0,GL_DEPTH_COMPONENT,GL_UNSIGNED_BYTE,0);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FUNC_ARB,GL_LEQUAL);	
	glTexParameteri(GL_TEXTURE_2D,GL_DEPTH_TEXTURE_MODE_ARB,GL_LUMINANCE);
	//if available use a shadow ambient value
	glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_COMPARE_FAIL_VALUE_ARB,globalAmbient);
}

int main(int argc, char **argv) {
	glutInitWindowSize(800,600);
	glutInit(&argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow("LispSM");

	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	initExtensions();
	initGLstate();

	view = EYE_SHADOWED;
	movingLight = 1;
	movingEye = 1;
	motion(0,0);
	movingEye = 0;
	movingLight = 0;

	glutMainLoop();
	return 0;
}
