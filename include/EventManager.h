#pragma once

#include <queue>
#include <deque>
#include "Core.h"
#include "Renderer.h"
#include "Curve.h"
#include "Mesh.h"

#include "MeshRenderer.h"
#include "HapticsEventManager.h"
#include <time.h>

#define MODE_IDLE 0
//#define MODE_SKETCH 0
//#define MODE_TRANSLATION 1
#define MODE_ROTATION 1
//#define UNDO 3
//#define REDO 4

namespace midl {
	class Mesh;

	class ABCSketchManager
	{
	private:
		Shader diffuseShader, textureShader;//Shaders
		/*int mode;*/
		float curr_stylus[3], prev_stylus[3], displacement[3], cursor[3], hapcurr_stylusleft[3], hapcurr_stylusright[3], forceLeft, forceRight;//stylus properties
		/*float angle , axis[3], matrix[9];*///rotation 

		clock_t start, end;
		double time_elapsed;

		vector<Curve3> Traj;
		deque<Curve3>Undo;
		vector<int> currentMode;
		vector<int> leftDevState;
		vector<int> rightDevState;
		/*vector<Tuple3f>stylusleft;
		vector<Tuple3f>stylusright;*/

		void UpdateStylusLeft(float *stylus);
		void UpdateStylusRight(float *stylus);
		void UpdateForceLeft(float &force);
		void UpdateForceRight(float &force);
	
	public:
		//char *userID;

		ABCSketchManager();
		~ABCSketchManager();

		vector<clock_t>stylusmov;
		vector<clock_t>hapticstylusmov;

		Curve3 c;
		int undoPress, redoPress, usermode, trialCount;
		float stiffnessVal;
		string option, testmode;		

		void InitShaders();
		void Render();
		int Update();
		int HapListenRight(float *stylus);
		int HapListenLeft(float *stylus);
		int ForceListenLeft(float &force);
		int ForceListenRight(float &force);
		string userID;		
		bool SaveLog();
		bool StartTimer();
		bool StopTimer();
		//bool UpdateDeviceState();
		bool recordData;
		bool isLeftOn;
		bool isRightOn;

		//bool needForce = false;		
		/*bool CheckHapticsForce();
		bool SetHaptics();*/
		/*void ComputeError();*/			
	};
}