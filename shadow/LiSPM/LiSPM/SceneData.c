//Copyright and Disclaimer:
//This code is copyright Vienna University of Technology, 2004.

#include <limits.h>
#include <stdlib.h>
#include "glInterface.h"
#include "SceneData.h"

const Vector4 black = {0.0, 0.0, 0.0, 1.0};
const Vector4 white = {1.0, 1.0, 1.0, 1.0};

#define TREE_SIZE 7.0
#define HILL_HEIGHT 4.0
#define HILL_SIZE 200.0
#define HILL_SIZE_2 HILL_SIZE/2

const struct AABox sceneAABox = { 
	{-HILL_SIZE_2,          -HILL_HEIGHT, -HILL_SIZE_2}, 
	{ HILL_SIZE_2, HILL_HEIGHT+TREE_SIZE,  HILL_SIZE_2}
};

GLenum texO = 0;
GLenum texHeight = 0;

//height map
#define MAP_SIZE 20
GLubyte heightMap[MAP_SIZE*MAP_SIZE];	// Holds The Height Map Data

float rndNormFloat() {
	float r = rand();
	r /= RAND_MAX;
	return r;
}

void setVector4(Vector4 v, const float x, const float y, const float z, const float a) {
	v[0] = x;
	v[1] = y;
	v[2] = z;
	v[3] = a;
}

void makeOTexture(void) {
	const char *circles[] = {
	"...xxx.xx.......",
	"..xx...xx..x....",
	".xx...xxx.x.x...",
	".x.xx...xxx.....",
	"xxx......x.xxx..",
	"xxx......x..xx..",
	"......x....x....",
	"xxx......xxx....",
	".xxx....xxx.....",
	".xx.xx...xx...x.",
	"..xxx....xx.....",
	"....xxxx......x.",
	"...........x....",
	"....xx.....xx...",
	"....x...x.......",
	"...x......xx....",
	};
	GLubyte oTexture[16][16][1];
	GLubyte *loc;
	int s, t;

	/* Setup RGB image for the texture. */
	loc = (GLubyte*) oTexture;
	for (t = 0; t < 16; t++) {
		for (s = 0; s < 16; s++, loc+= 1) {
			if (circles[t][s] == 'x') {
				/* green. */
				loc[0] = 0x2f;
			} else {
				/* near white gray. */
				loc[0] = 0x00;
			}
		}
	}
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	gluBuild2DMipmaps(GL_TEXTURE_2D,GL_ALPHA,16,16, GL_ALPHA,GL_UNSIGNED_BYTE,oTexture);
}

void makeHeightTexture(void) {
	#define SIZE 4
	GLubyte heightTexture[SIZE*3];

	/* Setup RGB image for the texture. */
	GLubyte *loc = (GLubyte*) heightTexture;
	/* cyan. */
	loc[0] = 0x00;
	loc[1] = 0x8f;
	loc[2] = 0x8f;
	loc += 3;

	/* green. */
	loc[0] = 0x00;
	loc[1] = 0x8f;
	loc[2] = 0x00;
	loc += 3;

	/* gray. */
	loc[0] = 0x4f;
	loc[1] = 0x4f;
	loc[2] = 0x4f;
	loc += 3;

	/* light gray. */
	loc[0] = 0xaf;
	loc[1] = 0xaf;
	loc[2] = 0xaf;

	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D,GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexImage1D(GL_TEXTURE_1D,0,3,SIZE,0,GL_RGB,GL_UNSIGNED_BYTE,heightTexture);
}

void setAmbientAndDiffuseMaterial(const GLfloat *material) {
	glMaterialfv(GL_FRONT,GL_AMBIENT_AND_DIFFUSE,material);
}

void drawThing(const float x, const float y, const float z) {
	Vector4 material;
	setVector4(material,0.1,rndNormFloat(),0.3,1.0);
	
	setAmbientAndDiffuseMaterial(material);

	glPushMatrix();
		glTranslatef(x,y,z);
		glRotatef(-90.0,1.0,0.0,0.0);
		glTranslatef(0.0,0.0,-HILL_HEIGHT/20);
		glutSolidCone(0.5,TREE_SIZE,5,16);
		glTranslatef(0.0,0.0,5.0);
		glutSolidTorus(0.2,1.0,8,16);	
	glPopMatrix();
}


void initHeightMap(BYTE pHeightMap[]) {
	int x, y;
	if(!pHeightMap) return;
	for( x = 0; x < MAP_SIZE; x++) {
		for( y = 0; y < MAP_SIZE; y++) { 
			pHeightMap[x+(y*MAP_SIZE)] = 256*rndNormFloat();
		}
	}
	//smooth step

}
int height(const BYTE *pHeightMap, const int X, const int Y) {
	if(!pHeightMap) return 0;		// Make Sure Our Data Is Valid
	{
		int x = X % MAP_SIZE;			// Error Check Our x Value
		int y = Y % MAP_SIZE;			// Error Check Our y Value
		return pHeightMap[x+(y*MAP_SIZE)];
	}
}

void sendVertex(const BYTE pHeightMap[], const int X, const int Y) {
	GLfloat x, y, z;
	x = (float)X/(MAP_SIZE-1);
	y = (float)height(pHeightMap,X,Y)/256;
	z = (float)Y/(MAP_SIZE-1);

	glTexCoord1f(y);
	//vertices in range [-0.5;0.5] for x,y,z axes
	glVertex3f(x-0.5,y-0.5,z-0.5);
}

void renderHeightMap(const BYTE pHeightMap[], 
		const float gridScaleX, const float gridScaleY, const float gridScaleZ) {
	int X = 0, Y = 0;
	if(!pHeightMap) return;

	glPushMatrix();
	glScalef(gridScaleX*2.0,gridScaleY*2.0,gridScaleZ*2.0);

	setAmbientAndDiffuseMaterial(white);
	glBindTexture(GL_TEXTURE_1D,texHeight);
	glEnable(GL_TEXTURE_1D);
	glBegin(GL_QUADS);				
		glNormal3f(0.0,1.0,0.0);

		for(X = 0; X < MAP_SIZE-1; X++) {
			for(Y = 0; Y < MAP_SIZE-1; Y++) {
				sendVertex(pHeightMap,X,Y);
				sendVertex(pHeightMap,X,Y+1);
				sendVertex(pHeightMap,X+1,Y+1);
				sendVertex(pHeightMap,X+1,Y);
			}
		}
	glEnd();
	glDisable(GL_TEXTURE_1D);

	glPopMatrix();
}

void drawGridConfiguration(const BYTE pHeightMap[], 
		const float gridScaleX, const float gridScaleY, const float gridScaleZ) {
	int X, Y;
	if(!pHeightMap) return;

	glMaterialfv(GL_FRONT,GL_SPECULAR,white);

	for(X = 1; X < MAP_SIZE-1; X++) {
		for(Y = 1; Y < MAP_SIZE-1; Y++) {
			GLfloat x, y, z;
			x = (float)X/(MAP_SIZE-1);
			y = (float)height(pHeightMap,X,Y)/256;
			z = (float)Y/(MAP_SIZE-1);
			drawThing((x-0.5)*gridScaleX*2.0,(y-0.5)*gridScaleY*2.0,(z-0.5)*gridScaleZ*2.0);
		}
	}

	glMaterialfv(GL_FRONT,GL_SPECULAR,black);
}

void drawSky(const float sizeX, const float sizeZ) {
	const float xmax = +sizeX/2;
	const float xmin = -xmax;
	const float zmax = +sizeZ/2;
	const float zmin = -zmax;
	const float count = 5.0;
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glBindTexture(GL_TEXTURE_2D,texO);
	glEnable(GL_TEXTURE_2D);
	glColor3f(1.0,1.0,1.0);
	glBegin(GL_QUADS);
		glTexCoord2f(0.0,0.0);
		glVertex3f(xmin,0.0,zmin);
		glTexCoord2f(count,0.0);
		glVertex3f(xmax,0.0,zmin);
		glTexCoord2f(count,count);
		glVertex3f(xmax,0.0,zmax);
		glTexCoord2f(0.0,count);
		glVertex3f(xmin,0.0,zmax);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_FOG);
	glEnable(GL_LIGHTING);

	glDisable(GL_BLEND);

}

void drawBoxVolume(const Vector3x8 v) {
	glBegin(GL_TRIANGLE_STRIP);
		glVertex3dv(v[0]);
		glVertex3dv(v[1]);
		glVertex3dv(v[3]);
		glVertex3dv(v[2]);
		glVertex3dv(v[7]);
		glVertex3dv(v[6]);
		glVertex3dv(v[4]);
		glVertex3dv(v[5]);
	glEnd();

	glBegin(GL_TRIANGLE_STRIP);
		glVertex3dv(v[6]);
		glVertex3dv(v[2]);
		glVertex3dv(v[5]);
		glVertex3dv(v[1]);
		glVertex3dv(v[4]);
		glVertex3dv(v[0]);
		glVertex3dv(v[7]);
		glVertex3dv(v[3]);
	glEnd();
}

void drawLineBoxVolume(const Vector3x8 v) {
	glBegin(GL_LINE_LOOP);
		glVertex3dv(v[0]);
		glVertex3dv(v[1]);
		glVertex3dv(v[5]);
		glVertex3dv(v[6]);
		glVertex3dv(v[2]);
		glVertex3dv(v[3]);
		glVertex3dv(v[7]);
		glVertex3dv(v[4]);
	glEnd();
	glBegin(GL_LINES);
		glVertex3dv(v[0]);
		glVertex3dv(v[3]);
		glVertex3dv(v[1]);
		glVertex3dv(v[2]);
		glVertex3dv(v[4]);
		glVertex3dv(v[5]);
		glVertex3dv(v[6]);
		glVertex3dv(v[7]);
	glEnd();
}


void drawScene() {
	static GLuint DL_SCENE = 0;		/* scene display list */
	if(DL_SCENE != 0) {
		glCallList(DL_SCENE);
	}
	else {
		glGenTextures(1,&texO);
		glBindTexture(GL_TEXTURE_2D,texO);
		makeOTexture();

		glGenTextures(1,&texHeight);
		glBindTexture(GL_TEXTURE_1D,texHeight);
		makeHeightTexture();

		srand(10);
		initHeightMap(heightMap);
		DL_SCENE = glGenLists(1);
		glNewList(DL_SCENE,GL_COMPILE);
			renderHeightMap(heightMap,HILL_SIZE_2,HILL_HEIGHT,HILL_SIZE_2);
			drawGridConfiguration(heightMap,HILL_SIZE_2,HILL_HEIGHT,HILL_SIZE_2);

			glPushMatrix();
				glTranslatef(0.0,HILL_HEIGHT+TREE_SIZE,0.0);
				drawSky(HILL_SIZE_2*2,HILL_SIZE_2*2);
			glPopMatrix();

		glEndList();
	}
}

