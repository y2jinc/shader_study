//Copyright and Disclaimer:
//This code is copyright Vienna University of Technology, 2004.

#include "Main.h"

//CHANGED
void calcNewDir(Vector3 dir, const struct VecPoint* B) {
	int i;
	copyVector3(dir,ZERO);
	for(i = 0; i < B->size; i++) {
		Vector3 p;
		diffVector3(p,B->points[i],eyePos);
		addVector3(dir,dir,p);
	}
	normalize(dir);
}

void calcUniformShadowMtx(struct VecPoint* B) {
	Vector3 min, max;

	//look from position(eyePos)
	//into direction(lightDir) 
	//with up vector(up)
	//CHANGED
	if(useBodyVec) {
		Vector3 newDir;
		calcNewDir(newDir,B);
		look(lightView,eyePos,lightDir,newDir);
	}
	else {
		look(lightView,eyePos,lightDir,viewDir);
	}

	//transform the light volume points from world into light space
	transformVecPoint(B,lightView);

	//calculate the cubic hull (an AABB) 
	//of the light space extents of the intersection body B
	//and save the two extreme points min and max
	calcCubicHull(min,max,B->points,B->size);

	//refit to unit cube
	//this operation calculates a scale translate matrix that
	//maps the two extreme points min and max into (-1,-1,-1) and (1,1,1)
	scaleTranslateToFit(lightProjection,min,max);
}

//calculates the up vector for the light coordinate frame
void calcUpVec(Vector3 up, const Vector3 viewDir, const Vector3 lightDir) {
	//we do what gluLookAt does...
	Vector3 left;
	//left is the normalized vector perpendicular to lightDir and viewDir
	//this means left is the normalvector of the yz-plane from the paper
	cross(left,lightDir,viewDir);
	//we now can calculate the rotated(in the yz-plane) viewDir vector
	//and use it as up vector in further transformations
	cross(up,left,lightDir);
	normalize(up);
}

//this is the algorithm discussed in the paper
void calcLispSMMtx(struct VecPoint* B) {
	Vector3 min, max;
	Vector3 up;
	Matrix4x4 lispMtx;
	struct VecPoint Bcopy = VECPOINT_NULL;
	double dotProd = dot(viewDir,lightDir);
	double sinGamma;

	sinGamma = sqrt(1.0-dotProd*dotProd);

	copyMatrix(lispMtx,IDENTITY);

	copyVecPoint(&Bcopy,*B);

	//CHANGED
	if(useBodyVec) {
		Vector3 newDir;
		calcNewDir(newDir,B);
		calcUpVec(up,newDir,lightDir);
	}
	else {
		calcUpVec(up,viewDir,lightDir);
	}

	//temporal light View
	//look from position(eyePos)
	//into direction(lightDir) 
	//with up vector(up)
	look(lightView,eyePos,lightDir,up);

	//transform the light volume points from world into light space
	transformVecPoint(B,lightView);

	//calculate the cubic hull (an AABB) 
	//of the light space extents of the intersection body B
	//and save the two extreme points min and max
	calcCubicHull(min,max,B->points,B->size);

	{
		//use the formulas of the paper to get n (and f)
		const double factor = 1.0/sinGamma;
		const double z_n = factor*nearDist; //often 1 
		const double d = absDouble(max[1]-min[1]); //perspective transform depth //light space y extents
		const double z_f = z_n + d*sinGamma;
		const double n = (z_n+sqrt(z_f*z_n))/sinGamma;
		const double f = n+d;
		Vector3 pos;

		//new observer point n-1 behind eye position
		//pos = eyePos-up*(n-nearDist)
		linCombVector3(pos,eyePos,up,-(n-nearDist));

		look(lightView,pos,lightDir,up);

		//one possibility for a simple perspective transformation matrix
		//with the two parameters n(near) and f(far) in y direction
		copyMatrix(lispMtx,IDENTITY);	// a = (f+n)/(f-n); b = -2*f*n/(f-n);
		lispMtx[ 5] = (f+n)/(f-n);		// [ 1 0 0 0] 
		lispMtx[13] = -2*f*n/(f-n);		// [ 0 a 0 b]
		lispMtx[ 7] = 1;				// [ 0 0 1 0]
		lispMtx[15] = 0;				// [ 0 1 0 0]

		//temporal arrangement for the transformation of the points to post-perspective space
		mult(lightProjection,lispMtx,lightView); // ligthProjection = lispMtx*lightView
		
		//transform the light volume points from world into the distorted light space
		transformVecPoint(&Bcopy,lightProjection);

		//calculate the cubic hull (an AABB) 
		//of the light space extents of the intersection body B
		//and save the two extreme points min and max
		calcCubicHull(min,max,Bcopy.points,Bcopy.size);
	}

	//refit to unit cube
	//this operation calculates a scale translate matrix that
	//maps the two extreme points min and max into (-1,-1,-1) and (1,1,1)
	scaleTranslateToFit(lightProjection,min,max);

	//together
	mult(lightProjection,lightProjection,lispMtx); // ligthProjection = scaleTranslate*lispMtx
}

void updateLightMtx(void) {
	//the intersection Body B
	struct VecPoint B = VECPOINT_NULL;

	//calculates the ViewFrustum Object; clippes this Object By the sceneAABox and
	//extrudes the object into -lightDir and clippes by the sceneAABox
	//the defining points are returned
	calcFocusedLightVolumePoints(&B,invEyeProjView,lightDir,sceneAABox);

	if(!useLispSM) {
		//do uniform shadow mapping
		calcUniformShadowMtx(&B);
	}
	else {
		//do Light Space Perspective shadow mapping
		calcLispSMMtx(&B);
	}

	emptyVecPoint(&B);

	//transform from right handed into left handed coordinate system
	{
		Matrix4x4 rh2lf;
		makeScaleMtx(rh2lf,1.0,1.0,-1.0);
		mult(lightProjection,rh2lf,lightProjection);
	}
}
