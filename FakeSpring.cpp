
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <fstream>
#include <crtdbg.h>


#include "EventManager.h"
#include "HapticsEventManager.h"
#include "opencv2\opencv.hpp"

#define GL_WIN_SIZE_X 128
#define GL_WIN_SIZE_Y 128

using namespace std;
using namespace midl;

// Event Manager
static HapticsEventManagerGeneric *hapticsEvent = 0;
static ABCSketchManager *skEvent = 0;

float randNum, prevrandNum;
int pcount, prevpcount;
vector<float>_rand;
string a, b, c;

float eye[] = { 0.0,0.0,5.0 };
float center[] = { 0.0,0.0,0.0 };
float head[] = { 0.0, 1.0, 0.0 };

//---- Open GL View
PerspectiveView view;

//---- Open GL Lighting
Light light1, light2;
GLfloat lightPos[] = { 0.0, 0.0, 10.0, 1.0 };
GLfloat diffuse1[] = { 0.54, 0.0, 0.0, 1.0 };
GLfloat diffuse2[] = { 0.0, 0.8, 0.0, 0.5 };
GLfloat ambient[] = { 0.5, 0.5, 0.5, 1.0 };
GLfloat specular[] = { 1.0, 1.0, 1.0, 1.0 };

float scale[] = { 1.0 ,1.0, 1.0 };


void CleanupAndExit()
{
	if (hapticsEvent)
	{
		hapticsEvent->Cleanup();
		HapticsEventManagerGeneric::Delete(hapticsEvent);
	}
	//if (skEvent)delete skEvent;
	exit(0);
}

void glutDisplay(void)
{
	//cerr << "here baby" << endl;
	hapticsEvent->UpdateState();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_NORMALIZE);

	view.Bind();
	

	glPolygonMode(GL_FRONT, GL_FILL);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(2.0f, 2.0f);

	skEvent->Render();

	view.Unbind();

	glutSwapBuffers();
}

void glutReshape(int w, int h)
{
	view.Reshape(w, h);
}

void glutIdle(void)
{
	// We just set the backgournd to White!
	// and do nothing.
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glutPostRedisplay();
}

void mouseClickCallback(int button, int state, int x, int y)
{
	// Example usage:
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			// When the left button is pressed
			// down, do something
		}
		else if (state == GLUT_UP)
		{
			// When the left button is lifted
			// up, do something else
		}
	}
}

void mouseActiveMotionCallback(int x, int y)
{
	// You can define anythging here
	// Right now, we do not want to 
	// do anything when the mouse moves!
	// So, let us leave this function empty!
}

void mousePassiveMotionCallback(int x, int y)
{
}

void glutKeyboard(unsigned char key, int x, int y)
{
	char filename1[1000], filename2[1000];
	string s1, s2;

	switch (key)
	{
	case 27: // 27 is ASCII code for the "Esc" key
		CleanupAndExit();
		break;
	case 'q':
		skEvent->isLeftOn = false;
		cerr << "Left Device Off" << endl;
		break;
	case 'w':
		skEvent->isRightOn = false;
		cerr << "Right Device Off" << endl;
		break;
	case 'z':
		skEvent->isLeftOn = true;
		cerr << "Left Device On" << endl;
		break;
	case 'x':
		skEvent->isRightOn = true;
		cerr << "Right Device On" << endl;
		break;
	case 'a':

		if (!_rand.empty())
		{
			//skEvent->SaveLog();
			skEvent->StartTimer();
			skEvent->recordData = true;

			prevrandNum = _rand[_rand.size() - 1];
			//skEvent->hapticstylusmov.clear();
			//skEvent->stylusmov.clear();

			srand(time(0));
			/*if (randNum == prevrandNum) { randNum = (rand() % 4) + 1; }
			else if (randNum != prevrandNum) { randNum = (rand() % 4) + 1; }*/
			//setprecision(2);
			randNum = randNum = (rand() % 5) + 0;
			while (randNum == prevrandNum) { randNum = (rand() % 5) + 0; }
			_rand.push_back(randNum);
			skEvent->stiffnessVal = randNum;
			
			cerr << "Stiffness->" << skEvent->stiffnessVal << endl;
		}

		else
		{
			skEvent->recordData = true;
			srand(time(0));
			randNum = (rand() % 5) + 0;
			_rand.push_back(randNum);
			skEvent->stiffnessVal = randNum;
			cerr << "Stiffness->" << skEvent->stiffnessVal << endl;
		}

		break;
	case '4':
		skEvent->stiffnessVal = 4;
		cerr << "Minimum Stiffness->"<<skEvent->stiffnessVal << endl;
		break;
	case 'r': //Let us Reset the scene
		//pcount = prevpcount = 0;
		skEvent->~ABCSketchManager();
		break;
	case 'R':
		//pcount = prevpcount = 0;
		skEvent->~ABCSketchManager();
		break;
	case 's':
		skEvent->SaveLog();
		break;
	case '5':
		skEvent->recordData = true;
		skEvent->stiffnessVal = 5;
		cerr << "Stiffness->" << skEvent->stiffnessVal << endl;
		break;
	case '6':
		skEvent->recordData = true;
		skEvent->stiffnessVal = 6;
		cerr << "Stiffness->" << skEvent->stiffnessVal << endl;
		break;
	case '7':
		skEvent->recordData = true;
		skEvent->stiffnessVal = 7;
		cerr << "Stiffness->" << skEvent->stiffnessVal << endl;
		break;
	case '8':
		skEvent->recordData = true;
		skEvent->stiffnessVal = 8;
		cerr << "Stiffness->" << skEvent->stiffnessVal << endl;
		break;
	case '9':
		skEvent->recordData = true;
		skEvent->stiffnessVal = 9;
		cerr << "Maximum Stiffness" << skEvent->stiffnessVal << endl;
		break;
	}
}

void specialKeyCallback(unsigned char key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_LEFT:
		break;
	case GLUT_KEY_RIGHT:
		break;
	}
}

void glInit(int * pargc, char ** argv)
{
	glutInit(pargc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);

	//glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
	// Create a window of size GL_WIN_SIZE_X, GL_WIN_SIZE_Y
	// See where GL_WIN_SIZE_X and GL_WIN_SIZE_Y are defined!!
	glutInitWindowSize(GL_WIN_SIZE_X, GL_WIN_SIZE_Y);

	// Create a window with some name
	glutCreateWindow("IDETC");
	//glutHideWindow();

	// GLEW is another library that
	// will be used as a standard for
	// initializing Shaders. 
	// Do not worry about this now!
	GLenum err = glewInit();
	if (GLEW_OK != err) // If GLEW was not properly initialized, we create a pop-up window and tell the user!
	{
		printf("Error: %s\n", glewGetErrorString(err));
		MessageBox(NULL, L"GLEW unable to initialize.", L"ERROR", MB_OK);
		return;
	}

	//---- Initialize Haptics device
	hapticsEvent = HapticsEventManager::Initialize();
	hapticsEvent->Setup(skEvent);
	//hapticsEvent->Setup2(skEvent);

	//---- Initialize the openGL camera 
	//---- and projection parameters
	view.SetParameters(35, 0.1, 10000.0);
	view.SetCameraCenter(0.0, 0.0, 0.0);
	view.SetCameraEye(0.0, 0.2, 2.0);
	view.SetCameraHead(0.0, 1.0, 0.0);

	// Shader Initialization:
	skEvent->InitShaders();

	light1.SetPosition(lightPos);
	light1.SetDiffuseColor(diffuse1);
	light1.SetAmbientColor(ambient);
	light1.SetSpecularColor(specular);

	light2.SetPosition(lightPos);
	light2.SetDiffuseColor(diffuse2);
	light2.SetAmbientColor(ambient);
	light2.SetSpecularColor(specular);

	glutReshapeFunc(glutReshape);
	glutDisplayFunc(glutDisplay);
	glutKeyboardFunc(glutKeyboard);
	//glutPassiveMotionFunc(mousePassiveMotionCallback);
	glutIdleFunc(glutIdle);
}

int main(int argc, char** argv)
{
	/*pcount = 0;
	prevpcount = 0;*/

	skEvent = new ABCSketchManager();
	//char filename[1000];

	cerr << "UserID: ";
	cin >> skEvent->userID;

	atexit(CleanupAndExit);

	glInit(&argc, argv);

	glutMainLoop();

	return 0;
}

