#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <numeric>
#include <algorithm>
#include <functional>

#include "GLUTApp.h"
#include "TriMesh.h"
#include "MeshIO.h"
#include "DrawUtil.h"
#include "PickPixel.h"
#include "XGLM.h"


using namespace xglm;

//void *gTextFont = GLUT_BITMAP_9_BY_15;
//void *gTextFont = GLUT_BITMAP_HELVETICA_18;
void *gTextFont = GLUT_BITMAP_TIMES_ROMAN_24;
const int TEXT_WIDTH = 13;
const int TEXT_HEIGHT = 24;
class MyGLView : public GLView
{
public:
	TriMesh			mMesh;
public:
	PickBuffer      mPickBuffer;
	PixelInfo       mObjUnderMouse;
	vector<int>		mPickedPoints;
	vector<int>		mPickedFaces;
	bool            mInPicking;
	
public:
	MyGLView()
	{
		mInPicking = true;
	}
	virtual void cbReshape(int width, int height)
	{
		GLView::cbReshape(width,height);
		mPickBuffer.markDirty(true);
	}
	virtual void cbKeyboard(unsigned char key, int x, int y)
	{
		switch (key)
		{
			case 'v':
				mInPicking = !mInPicking; break;
			case 's':
			case 'S':
				GLView::cbKeyboard(key,x,y);
				mPickBuffer.markDirty(true); break;
			default:
				GLView::cbKeyboard(key,x,y);
		}
		
	}
	virtual void cbMouse(int button, int state, int x, int y)
	{
		GLView::cbMouse(button,state,x,y);
		if( state==GLUT_DOWN )
		{
			if( mInPicking && ! _trackball.isActive() && ! mPickBuffer.isDirty() )
			{
			}
		}
	}
	virtual void cbMotion(int x, int y)
	{
		GLView::cbMotion(x,y);
		if( _trackball.isActive() )
			mPickBuffer.markDirty(true);
	}
	virtual void cbPassiveMotion(int x, int y)
	{
		mObjUnderMouse = getObjectAt(x,_viewport[3]-y);
	}
	// callbacks 
	virtual void cbDisplay ()
	{
		if( mInPicking && ! _trackball.isActive() && mPickBuffer.isDirty() )
		{
			updatePickBuffer();
		}
	    //glClear(GL_COLOR_BUFFER_BIT);
		//glDrawPixels(_viewport[2],_viewport[3],GL_RGBA, GL_UNSIGNED_BYTE, mPickBuffer.getBuf());
		//updatePickBuffer();
		//glFlush();
		//glutSwapBuffers();
		//glutPostRedisplay();
		GLView::cbDisplay();
	}
	virtual int loadScene(string scene)
	{
		XGLM_LOG("Reading mesh from %s\n", scene.c_str());
		if( ! MeshIO<TriMesh>::readMesh(scene, mMesh) )
		{
			XGLM_LOG("Failed reading mesh %s\n", scene.c_str());
			return 0;
		}
		return 1;
	}
	virtual void getSceneBBox(Vec3f & bbmin, Vec3f& bbmax)
	{
		if( ! mMesh._bbox.IsValid() )
			mMesh.needBBox();
		bbmin = mMesh._bbox._min;
		bbmax = mMesh._bbox._max;
	}
	virtual void drawScene(void)
	{
		glColor4f(1.f, 1.f, 1.f, 1.f);
		DrawFace<TriMesh>::draw(mMesh);
		if( (unsigned int)mObjUnderMouse )
			drawObjUnderMouse();
	}
	virtual void drawText()
	{
		const int *vport = _viewport;
		const int screenWidth = vport[2];
		const int screenHeight = vport[3];
		float color[] = {1.0f,.0f,1.0f,1.0f};
		stringstream ss;
		// frame per second
        //ss << std::fixed << std::setprecision(1);
        ss << getFPS() << " FPS" << ends;
        //ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
		string fps = ss.str();
		drawString(fps.c_str(), screenWidth-(int)fps.size() * TEXT_WIDTH, screenHeight-TEXT_HEIGHT,color,gTextFont);
		ss.str(""); // clear buffer
		ss << "Press v to toggle visual model." << ends;
		drawString(ss.str().c_str(), 1, 10, color, gTextFont);
	}
	void drawObjUnderMouse()
	{
		unsigned int grp = mObjUnderMouse.getGroup();
		unsigned int idx = mObjUnderMouse.getIndex();
		glPushAttrib(GL_LIGHTING_BIT);
		glDisable(GL_LIGHTING);
		if( grp==1 && idx>=0 && idx<mMesh.mPosition.size() ) { // vertex
			glBegin(GL_POINTS);
			glColor4ub(255,0,0,0);
			glVertex3fv(mMesh.mPosition[idx].get_value());
			glEnd();
			//printf("cursor points\n");
		}
		else if( grp==2 && idx>=0 && idx<mMesh.mTriangles.size() ) {  // face
			const int * t = mMesh.mTriangles[idx].get_value();
			glPointSize(15);
			glBegin(GL_TRIANGLES);
			glColor4ub(255,0,0,0);
			for( int k = 0; k<3; k++ )
				glVertex3fv(mMesh.mPosition[t[k]].get_value());
			glEnd();
			//printf("cursor triangle\n");
		}
		glPopAttrib();
	}
protected:
	PixelInfo getObjectAt(int x, int y)
	{
		if( mPickBuffer.isDirty() || ! mPickBuffer.contain(x,y) )
			return PixelInfo(0);
		return mPickBuffer.getAt(x,y);
	}
	
	void updatePickBuffer()
	{
		//printf("update pick buffer\n");
		int viewport[4];
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);	
		glGetIntegerv(GL_VIEWPORT,viewport);
		mPickBuffer.resize(viewport[2],viewport[3]);
		// prepare to draw IDs
		glPushAttrib(GL_COLOR_BUFFER_BIT|GL_LIGHTING_BIT|GL_POLYGON_BIT|GL_POINT_BIT);
		glClearColor(0.f, 0.f, 0.f, 0.f);
		glDisable(GL_LIGHTING);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		GLView::applyProjectionAndModelview();
		// drawing....
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		DrawFaceID<TriMesh>::draw(mMesh);
		glPointSize(10.0f);
		DrawPointID<TriMesh>::draw(mMesh);
		glFlush();
		// read out the color buffer 
		glReadBuffer(GL_BACK);
		glReadPixels(0,0,viewport[2],viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, mPickBuffer.getBuf());
		glPopAttrib();
		mPickBuffer.markDirty(false);
	}
	
};

GLUTApp  theApp("MeshProcessingKit", 1600, 1200, 800, 200);
MyGLView theView;
int main (int argc, char *argv[])
{
	setbuf(stdout, NULL); // to seed ouput immediately for debugging

	if( argc<2 ) {
		printf("Requiring a path to the input model\n");
		return 1;
	}
	if( ! theApp.createWindow(argc, argv) ) {
		printf("Failed creating the window\n");
		return 1;		
	}
	if( ! theView.loadScene(string(argv[1])) ) {
		printf("Failed loading the input model:\n\t%s\n", argv[1]);
		return 1;
	}
	theView.initialize ();
	// glut callback functions // using c++ 11's lambda
	glutReshapeFunc(	[](int w, int h)						->void { theView.cbReshape(w,h); });
	glutDisplayFunc(	[]()									->void { theView.cbDisplay(); });
	glutKeyboardFunc(	[](unsigned char key, int x, int y)		->void { theView.cbKeyboard(key,x,y); });
	glutMouseFunc(		[](int button, int state, int x, int y)	->void { theView.cbMouse(button,state,x,y); });
	glutMotionFunc(		[](int x, int y)						->void { theView.cbMotion(x,y); });
	glutPassiveMotionFunc([](int x, int y)						->void { theView.cbPassiveMotion(x,y); });
	// running the application
	glutMainLoop();
    return 0;
}