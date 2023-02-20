

#include <chrono>
#include <ctime>
#include "EventManager.h"
#include "Renderer.h"
chrono::time_point<chrono::system_clock> timerON, timerOFF, startDataRecording, onceTimerStarts;

using namespace midl;

float _hapcurr_stylusleft[3], _hapcurr_stylusright[3], _leftForce, _rightForce;
vector<Tuple3f>stylusleft;
vector<Tuple3f>stylusright;
vector<float> LForce;
vector<float> RForce;



vector<float>_timer;
int _timerValue;
bool _recordData;

void _UpdateHapStylusLeft(float *stylus)
{
	if (_recordData)
	{
		Tuple3f _stylusleft;

		_hapcurr_stylusleft[0] = stylus[0];
		_hapcurr_stylusleft[1] = stylus[1];
		_hapcurr_stylusleft[2] = stylus[2];

		_stylusleft.data[0] = _hapcurr_stylusleft[0];
		_stylusleft.data[1] = _hapcurr_stylusleft[2];
		_stylusleft.data[2] = _hapcurr_stylusleft[1];

		stylusleft.push_back(_stylusleft);
	}
	else {}
}

void _UpdateHapStylusRight(float *stylus)
{
	if (_recordData)
	{
		Tuple3f _stylusright;

		_hapcurr_stylusright[0] = stylus[0];
		_hapcurr_stylusright[1] = stylus[1];
		_hapcurr_stylusright[2] = stylus[2];


		/*cerr << "X->" << stylus[0] << endl;
		cerr << "Y->" << stylus[1] << endl;
		cerr << "Z->" << stylus[2] << endl;*/

		_stylusright.data[0] = _hapcurr_stylusright[0];
		_stylusright.data[1] = _hapcurr_stylusright[2];
		_stylusright.data[2] = _hapcurr_stylusright[1];

		stylusright.push_back(_stylusright);		
	}
	else {}
}

void _UpdateForces(float &leftForce, float &rightForce)
{
	if (_recordData)
	{
		_leftForce = leftForce;
		LForce.push_back(_leftForce);

		_rightForce = rightForce;
		RForce.push_back(_rightForce);
	}
	else {}	
}

void ABCSketchManager::UpdateStylusLeft(float *stylus)
{
	hapcurr_stylusleft[0] = stylus[0];
	hapcurr_stylusleft[1] = stylus[1];
	hapcurr_stylusleft[2] = stylus[2];
}

void ABCSketchManager::UpdateStylusRight(float *stylus)
{
	hapcurr_stylusright[0] = stylus[0];
	hapcurr_stylusright[1] = stylus[1];
	hapcurr_stylusright[2] = stylus[2];
}

void ABCSketchManager::UpdateForceLeft(float &force)
{
	forceLeft = force;	
}

void ABCSketchManager::UpdateForceRight(float &force)
{
	forceRight = force;	
}


ABCSketchManager::ABCSketchManager()
{
	undoPress = 0;
	redoPress = 0;	
	stiffnessVal = 0;
	trialCount = 0;
	isLeftOn = true;
	isRightOn = true;
	recordData = false;
}

ABCSketchManager::~ABCSketchManager()
{	
	trialCount = 0;
	isLeftOn = true;
	isRightOn = true;
	recordData = false;
	stylusleft.clear();
	stylusright.clear();
	LForce.clear();
	RForce.clear();
}

void ABCSketchManager::InitShaders()
{
	diffuseShader.Initialize(".//Shaders//diffuseShader.vert", ".//Shaders//diffuseShader.frag");
	//textureShader.Initialize(".//Shaders//colorShader.vert", ".//Shaders//colorShader.frag");
}

//void ABCSketchManager::ManipulationOff()
//{
//	mode = MODE_IDLE;
//}


//bool ABCSketchManager::SetHaptics()
//{
//	needForce = true;
//	return true;
//}

//bool ABCSketchManager::CheckHapticsForce()
//{
//	return true;
//}

void ABCSketchManager::Render()
{	
	_UpdateHapStylusLeft(hapcurr_stylusleft);
	_UpdateHapStylusRight(hapcurr_stylusright);
	_UpdateForces(forceLeft, forceRight);
	_recordData = recordData;
	stiffnessVal;
	//UpdateDeviceState();
}

int ABCSketchManager::Update()
{

	//UpdatePointToPlaneDist();
	//return mode;
	return 0;
}

int ABCSketchManager::HapListenRight(float *stylus)
{
	UpdateStylusRight(stylus);
	return Update();
}

int ABCSketchManager::HapListenLeft(float *stylus)
{
	UpdateStylusLeft(stylus);
	return 0;
}

int ABCSketchManager::ForceListenLeft(float &force)
{
	UpdateForceLeft(force);
	return 0;
}

int ABCSketchManager::ForceListenRight(float &force)
{
	UpdateForceRight(force);
	return 0;
}

bool ABCSketchManager::StartTimer()
{
	timerON = chrono::system_clock::now();
	return true;
}

bool ABCSketchManager::StopTimer()
{
	timerOFF = chrono::system_clock::now();	
	return true;
}

//bool ABCSketchManager::UpdateDeviceState()
//{
//	isLeftOn;
//	isRightOn;
//	if (recordData)
//	{
//		leftDevState.push_back(isLeftOn);
//		//cerr << "Dev Status->" << endl;
//		rightDevState.push_back(isRightOn);
//	}
//
//	else {}
//	return true;
//}

bool ABCSketchManager::SaveLog()
{
	StopTimer();
	recordData = false;
	chrono::duration<float> timeTAKEN = timerOFF - timerON;

	trialCount++;
	string _trialCount = to_string(trialCount);
	
	ofstream myfile;
	myfile.open("User_" + userID + "_" + _trialCount + ".txt");
	//myfile.open(userID + "_" + shapeIteration + ".txt");
	//myfile << timeTAKEN.count() - timerValue << endl;
	//myfile << "mode->" << currentMode.size() << endl;	
	myfile << timeTAKEN.count() << endl;
	myfile << stiffnessVal << endl;
	for (int i = 0; i < stylusleft.size(); i++)
	{
		//myfile << LForce[i] <<" "<< stylusleft[i].data[0] << " " << stylusleft[i].data[1] << " " << stylusleft[i].data[2] << " " << RForce[i] << " " << stylusright[i].data[0] << " " << stylusright[i].data[1] << " " << stylusright[i].data[2] << " " << leftDevState[i] << " "<< rightDevState[i] << endl;
		myfile << LForce[i] <<" "<< stylusleft[i].data[0] << " " << stylusleft[i].data[1] << " " << stylusleft[i].data[2] << " " << RForce[i] << " " << stylusright[i].data[0] << " " << stylusright[i].data[1] << " " << stylusright[i].data[2] <<  endl;
	}
	/*{

		myfile << currentMode[i] << " " << styl[i].data[0] << " " << styl[i].data[1] << " " << styl[i].data[2] << " " << hapstyl[i].data[0] << " " << hapstyl[i].data[1] << " " << hapstyl[i].data[2] << " "<<_stylusmov[i] << endl;
	}*/
	/*for (int i = 0; i < currentMode.size(); i++)
	{*/
		
	stylusright.clear();
	stylusleft.clear();
	LForce.clear();
	RForce.clear();

	cerr << "Data Logged Successfully for Trial->" << trialCount << ", at stiffness value-> " << stiffnessVal << endl;
	return true;
}

