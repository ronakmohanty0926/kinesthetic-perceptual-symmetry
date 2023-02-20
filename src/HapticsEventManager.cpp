#include "HapticsEventManager.h"

using namespace midl;

float Rprev_stylus[3], Lprev_stylus[3], curr_stylus[3], left_stylus[3], right_stylus[3];
float leftdisplacement[3], rightdisplacement[3], leftpivot[3], rightpivot[3];
float hapStiffness;
bool forceOn;

/******************************************************************************
Factory method for creating an instance of the HapticsEventManager.
******************************************************************************/
HapticsEventManagerGeneric *HapticsEventManagerGeneric::Initialize()
{
	return new HapticsEventManager;
}

/******************************************************************************
Factory method for destroying an instance of the HapticsEventManager.
******************************************************************************/
void HapticsEventManagerGeneric::Delete(HapticsEventManagerGeneric *&pInterface)
{
	if (pInterface)
	{
		HapticsEventManager *pImp = static_cast<HapticsEventManager *>(pInterface);
		delete pImp;
		pInterface = 0;
	}
}

/******************************************************************************
HapticsEventManager Constructor.
******************************************************************************/
HapticsEventManager::HapticsEventManager() :
	m_LhHD(HD_INVALID_HANDLE),
	m_RhHD(HD_INVALID_HANDLE),
	m_hUpdateCallback(HD_INVALID_HANDLE),
	m_pHapticDeviceHT(0),
	m_pHapticDeviceRHT(0),
	m_pHapticDeviceGT(0),
	m_pHapticDeviceRGT(0),
	abcSketchManager(0),
	m_nCursorDisplayList(0)
{}

/*******************************************************************************
HapticsEventManager Destructor.
*******************************************************************************/
HapticsEventManager::~HapticsEventManager() {}

void HapticsEventManager::Setup(ABCSketchManager *skManager)
{
	HDErrorInfo error;

	/* Intialize a device configuration. */
	m_LhHD = hdInitDevice("Left Device");
	hdEnable(HD_FORCE_OUTPUT);	

	m_RhHD = hdInitDevice("Right Device");
	hdEnable(HD_FORCE_OUTPUT);
	

	/* Create the IHapticDevice instances for the haptic and graphic threads
	These interfaces are useful for handling the synchronization of
	state between the two main threads. */
	m_pHapticDeviceHT = IHapticDevice::create(
		IHapticDevice::HAPTIC_THREAD_INTERFACE, m_LhHD);

	m_pHapticDeviceRHT = IHapticDevice::create(
		IHapticDevice::HAPTIC_THREAD_INTERFACE, m_RhHD);

	m_pHapticDeviceGT = IHapticDevice::create(
		IHapticDevice::GRAPHIC_THREAD_INTERFACE, m_LhHD);

	m_pHapticDeviceRGT = IHapticDevice::create(
		IHapticDevice::GRAPHIC_THREAD_INTERFACE, m_RhHD);

	/* Setup callbacks so we can be notified about events in the graphics
	thread. */
	m_pHapticDeviceGT->setCallback(
		IHapticDevice::MADE_CONTACT, madeContactCallbackGT, &m_LhHD);
	m_pHapticDeviceGT->setCallback(
		IHapticDevice::LOST_CONTACT, lostContactCallbackGT, &m_LhHD);
	m_pHapticDeviceGT->setCallback(
		IHapticDevice::BUTTON_1_UP, button1UpClickCallbackGT, &m_LhHD);
	m_pHapticDeviceGT->setCallback(
		IHapticDevice::BUTTON_1_DOWN, button1DownClickCallbackGT, &m_LhHD);
	m_pHapticDeviceGT->setCallback(
		IHapticDevice::BUTTON_2_UP, button2UpClickCallbackGT, &m_LhHD);
	m_pHapticDeviceGT->setCallback(
		IHapticDevice::BUTTON_2_DOWN, button2DownClickCallbackGT, &m_LhHD);
	m_pHapticDeviceGT->setCallback(
		IHapticDevice::DEVICE_ERROR, errorCallbackGT, &m_LhHD);

	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::MADE_CONTACT, madeContactCallbackRGT, &m_RhHD);
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::LOST_CONTACT, lostContactCallbackRGT, &m_RhHD);
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_1_UP, button1UpClickCallbackRGT, &m_RhHD);
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_1_DOWN, button1DownClickCallbackRGT, &m_RhHD);
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_2_UP, button2UpClickCallbackRGT, &m_RhHD);
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_2_DOWN, button2DownClickCallbackRGT, &m_RhHD);
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::DEVICE_ERROR, errorCallbackRGT, &m_RhHD);	

	hdStartScheduler();	

	m_RhUpdateCallback = m_hUpdateCallback;

	m_hUpdateCallback = hdScheduleAsynchronous(deviceUpdateCallback, this, HD_MAX_SCHEDULER_PRIORITY);
	
	//m_RhUpdateCallback = hdScheduleAsynchronous(deviceUpdateCallback, this, HD_MAX_SCHEDULER_PRIORITY);
	
	
	//m_hUpdateCallback = hdScheduleAsynchronous(LdeviceUpdateCallback, 0, HD_DEFAULT_SCHEDULER_PRIORITY);	

	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		std::cerr << error << std::endl;
		std::cerr << "Failed to initialize haptic device" << std::endl;
		std::cerr << "Press any key to quit." << std::endl;
		getchar();
		exit(-1);
	}

	forceOn = false;
	
	// GRAPHICS SETUP
	abcSketchManager = skManager;
	diffuseShader.Initialize(".//Shaders//diffuseShader.vert", ".//Shaders//diffuseShader.frag");
	
}

void HapticsEventManager::Cleanup()
{
	hdStopScheduler();

	if (m_hUpdateCallback != HD_INVALID_HANDLE)
	{
		hdUnschedule(m_hUpdateCallback);
		m_hUpdateCallback = HD_INVALID_HANDLE;
	}

	if (m_LhHD != HD_INVALID_HANDLE)
	{
		hdDisableDevice(m_LhHD);
		m_LhHD = HD_INVALID_HANDLE;
	}

	//if (m_RhUpdateCallback != HD_INVALID_HANDLE)
	//{
	//	hdUnschedule(m_RhUpdateCallback);
	//	m_RhUpdateCallback = HD_INVALID_HANDLE;
	//}

	//if (m_RhHD != HD_INVALID_HANDLE)
	//{
	//	hdDisableDevice(m_RhHD);
	//	m_RhHD = HD_INVALID_HANDLE;
	//}

	IHapticDevice::destroy(m_pHapticDeviceGT);
	IHapticDevice::destroy(m_pHapticDeviceHT);

	/*IHapticDevice::destroy(m_pHapticDeviceRGT);
	IHapticDevice::destroy(m_pHapticDeviceRHT);
*/
	glDeleteLists(m_nCursorDisplayList, 1);
}

void HapticsEventManager::UpdateState()
{
	//cerr << "yay" << endl;
	/* Capture the latest state from the servoloop. */

	m_pHapticDeviceGT->beginUpdate(m_pHapticDeviceHT);
	m_pHapticDeviceGT->beginUpdate(m_pHapticDeviceRHT);
	m_pHapticDeviceGT->endUpdate(m_pHapticDeviceHT);
	m_pHapticDeviceGT->endUpdate(m_pHapticDeviceRHT);

	/*m_pHapticDeviceGT->beginUpdate(m_pHapticDeviceRHT);
	m_pHapticDeviceGT->endUpdate(m_pHapticDeviceRHT);*/
}

void HapticsEventManager::UpdateWorkspace()
{
	GLdouble modelview[16];
	GLdouble projection[16];
	GLint viewport[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);
	glGetIntegerv(GL_VIEWPORT, viewport);

	/* Compute the transform for going from device coordinates to world
	coordinates based on the current viewing transforms. */
	hduMapWorkspaceModel(modelview, projection, m_workspaceXform);

	/* Compute the scale factor that can be applied to a unit sized object
	in world coordinates that will make it a particular size in pixels. */
	HDdouble screenTworkspace = hduScreenToWorkspaceScale(
		modelview, projection, viewport, m_workspaceXform);

	m_cursorScale = CURSOR_SIZE_PIXELS * screenTworkspace;

	hlMatrixMode(HL_TOUCHWORKSPACE);
	hluFitWorkspace(projection);

	/* Compute the updated camera position in world coordinates. */
	hduMatrix worldTeye(modelview);
	hduMatrix eyeTworld = worldTeye.getInverse();
	eyeTworld.multVecMatrix(hduVector3Dd(0, 0, 0), m_cameraPosWC);

	hdScheduleSynchronous(setDeviceTransformCallback, &m_LhHD, SYNCHRONIZE_STATE_PRIORITY);
	hdScheduleSynchronous(setDeviceTransformCallback, &m_RhHD, SYNCHRONIZE_STATE_PRIORITY);
}

HDCallbackCode HDCALLBACK HapticsEventManager::deviceUpdateCallback(
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	hdBeginFrame(pThis->m_LhHD);
	hdMakeCurrentDevice(pThis->m_LhHD);
	/* Force the haptic device to update its state. */
	pThis->m_pHapticDeviceHT->beginUpdate(0);

	IHapticDevice::IHapticDeviceState *pLeftCurrentState =
		pThis->m_pHapticDeviceHT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pLeftLastState =
		pThis->m_pHapticDeviceHT->getLastState();

	/*IHapticDevice::IHapticDeviceState *pCurrentState =
		pThis->m_pHapticDeviceHT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pLastState =
		pThis->m_pHapticDeviceHT->getLastState();*/

		/* Get the position of the device. */
	hduVector3Dd devicePositionLeft = pLeftCurrentState->getPosition();

	float left_stylus[] = { (float)devicePositionLeft[0] , (float)devicePositionLeft[1] , (float)devicePositionLeft[2] };

	//pThis->RenderStylus();
	Lprev_stylus[0] = left_stylus[0];
	Lprev_stylus[1] = left_stylus[1];
	Lprev_stylus[2] = left_stylus[2];

	//cerr << "X->" << prev_stylus[0] << endl;
	//cerr << "Y->" << prev_stylus[1] << endl;
	//cerr << "Z->" << prev_stylus[2] << endl;

	float LeftforceVector[] = { 0.0,0.0,0.0 };

	/*cerr << "RX->" << devicePosition[0] << endl;
	cerr << "RY->" << devicePosition[1] << endl;
	cerr << "RZ->" << devicePosition[2] << endl;*/

	//float xRng[] = { -172.0, 172.0 }; float yRng[] = { -108.0, 50.0 }; float zRng[] = { -56.0, -32.0 };
	//float xLen = xRng[1] - xRng[0]; float yLen = yRng[1] - yRng[0]; float zLen = zRng[1] - zRng[0];

	//float newRngX[] = { -1, 1 };
	//float newRngY[] = { -0.75, 0.75 };
	//float newRngZ[] = { -0.088, 0.088 };
	//float RngX = newRngX[1] - newRngX[0];
	//float RngY = newRngY[1] - newRngY[0];
	//float RngZ = newRngZ[1] - newRngZ[0];

	//stylus[0] = RngX * (((float)devicePositionLC[0] - xRng[0]) / xLen) + newRngX[0];
	//stylus[1] = RngY * (((float)devicePositionLC[1] - yRng[0]) / yLen) + newRngY[0];
	//stylus[2] = RngZ * (((float)devicePositionLC[2] - zRng[0]) / zLen) + newRngZ[0];

	
	SubVectors3(left_stylus, leftpivot, leftdisplacement);
	if (leftdisplacement[0] < 0)leftdisplacement[0] = -1 * leftdisplacement[0];
	//float leftdisp = Norm3(leftdisplacement);

	/* Update the voxel model based on the new position data. */
	ABCSketchManager *leftStyAPI = pThis->abcSketchManager;

	leftStyAPI->HapListenLeft(left_stylus);
	//skAPI->Listen(stylus);

	hduVector3Dd Leftforce;
	float stiffness = 1.5;
	//cerr << "Stiffness->" << leftStyAPI->stiffnessVal << endl;
	//cerr << "Stiffness->" << leftStyAPI->stiffness << endl;
	//cerr << "Status->" <<forceOn << endl;
	if (forceOn)
	{
		if (leftStyAPI->isLeftOn)
		{
			LeftforceVector[0] = 2*leftStyAPI->stiffnessVal * (leftdisplacement[0]*0.001); LeftforceVector[1] = 0.0; LeftforceVector[2] = 0.0;
			//LeftforceVector[0] = 2.0; LeftforceVector[1] = 0.0; LeftforceVector[2] = 0.0;			
			Leftforce.set((HDdouble)LeftforceVector[0], (HDdouble)LeftforceVector[1], (HDdouble)LeftforceVector[2]);
			leftStyAPI->ForceListenLeft(LeftforceVector[0]);
			//cerr << "LForce->" << LeftforceVector[0] << endl;
		}
		else if (!leftStyAPI->isLeftOn)
		{
			Leftforce.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);
			LeftforceVector[0] = 0.0;
			leftStyAPI->ForceListenLeft(LeftforceVector[0]);			
		}		
	}
	else
	{
		Leftforce.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);
		LeftforceVector[0] = 0.0;
		leftStyAPI->ForceListenLeft(LeftforceVector[0]);		
	}
	
	hdSetDoublev(HD_CURRENT_FORCE, Leftforce);

	pThis->m_pHapticDeviceHT->endUpdate(0);

	hdEndFrame(pThis->m_LhHD);


	hdBeginFrame(pThis->m_RhHD);
	hdMakeCurrentDevice(pThis->m_RhHD);
	/* Force the haptic device to update its state. */
	pThis->m_pHapticDeviceRHT->beginUpdate(0);

	IHapticDevice::IHapticDeviceState *pRightCurrentState =
		pThis->m_pHapticDeviceRHT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pRightLastState =
		pThis->m_pHapticDeviceRHT->getLastState();

	/*IHapticDevice::IHapticDeviceState *pCurrentState =
		pThis->m_pHapticDeviceRHT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pLastState =
		pThis->m_pHapticDeviceRHT->getLastState();*/

	/* Get the position of the device. */
	hduVector3Dd RightdevicePosition = pRightCurrentState->getPosition();

	float right_stylus[] = { (float)RightdevicePosition[0] , (float)RightdevicePosition[1] , (float)RightdevicePosition[2] };	

	Rprev_stylus[0] = right_stylus[0];
	Rprev_stylus[1] = right_stylus[1];
	Rprev_stylus[2] = right_stylus[2];

	float RightforceVector[] = { 0.0,0.0,0.0 };

	//float xRng[] = { -172.0, 172.0 }; float yRng[] = { -108.0, 50.0 }; float zRng[] = { -56.0, -32.0 };
	//float xLen = xRng[1] - xRng[0]; float yLen = yRng[1] - yRng[0]; float zLen = zRng[1] - zRng[0];

	//float newRngX[] = { -1, 1 };
	//float newRngY[] = { -0.75, 0.75 };
	//float newRngZ[] = { -0.088, 0.088 };
	//float RngX = newRngX[1] - newRngX[0];
	//float RngY = newRngY[1] - newRngY[0];
	//float RngZ = newRngZ[1] - newRngZ[0];

	//stylus[0] = RngX * (((float)devicePositionLC[0] - xRng[0]) / xLen) + newRngX[0];
	//stylus[1] = RngY * (((float)devicePositionLC[1] - yRng[0]) / yLen) + newRngY[0];
	//stylus[2] = RngZ * (((float)devicePositionLC[2] - zRng[0]) / zLen) + newRngZ[0];

	//curr_stylus[0] = stylus[0];
	//curr_stylus[1] = stylus[1];
	//curr_stylus[2] = stylus[2];

	//pThis->RenderStylus();
	SubVectors3(right_stylus, rightpivot, rightdisplacement);
	if (rightdisplacement[0] < 0)rightdisplacement[0] = -1 * rightdisplacement[0];

	//float rightdisp = Norm3(rightdisplacement);

	/* Update the voxel model based on the new position data. */
	ABCSketchManager *rightStyAPI = pThis->abcSketchManager;

	rightStyAPI->HapListenRight(right_stylus);
	//skAPI->Listen(stylus);

	hduVector3Dd Rightforce;
	//cerr << "Stiffness->" << rightStyAPI->stiffness << endl;
	if (forceOn)
	{
		if (rightStyAPI->isRightOn)
		{
			
			RightforceVector[0] = -2*rightStyAPI->stiffnessVal * (rightdisplacement[0]*0.001); RightforceVector[1] = 0.0; RightforceVector[2] = 0.0;
			//RightforceVector[0] = -2.0; RightforceVector[1] = 0.0; RightforceVector[2] = 0.0;
			Rightforce.set((HDdouble)RightforceVector[0], (HDdouble)RightforceVector[1], (HDdouble)RightforceVector[2]);
			rightStyAPI->ForceListenRight(RightforceVector[0]);			
			//cerr << "RForce->"<<RightforceVector[0] << endl;
		}
		else if (!rightStyAPI->isRightOn)
		{
			Rightforce.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);
			RightforceVector[0] = 0.0;
			rightStyAPI->ForceListenRight(RightforceVector[0]);			
		}
	}
	else
	{
		Rightforce.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);
		RightforceVector[0] = 0.0;		
		rightStyAPI->ForceListenRight(RightforceVector[0]);		
	}
	
	/*}
	else Leftforce.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);*/
	
	hdSetDoublev(HD_CURRENT_FORCE, Rightforce);
	
	pThis->m_pHapticDeviceRHT->endUpdate(0);

	hdEndFrame(pThis->m_RhHD);

	return HD_CALLBACK_CONTINUE;
}

/******************************************************************************
Scheduler callback to set the workspace transform both for use in the graphics
thread and haptics thread.
******************************************************************************/
HDCallbackCode HDCALLBACK HapticsEventManager::setDeviceTransformCallback(
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	hdBeginFrame(pThis->m_LhHD);
	IHapticDevice::IHapticDeviceState *pStateGT =
		pThis->m_pHapticDeviceGT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pStateHT =
		pThis->m_pHapticDeviceHT->getCurrentState();

	pStateGT->setParentCumulativeTransform(pThis->m_workspaceXform);
	pStateHT->setParentCumulativeTransform(pThis->m_workspaceXform);
	hdEndFrame(pThis->m_LhHD);

	hdBeginFrame(pThis->m_RhHD);
	IHapticDevice::IHapticDeviceState *pStateRGT =
		pThis->m_pHapticDeviceRGT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pStateRHT =
		pThis->m_pHapticDeviceRHT->getCurrentState();

	pStateRGT->setParentCumulativeTransform(pThis->m_workspaceXform);
	pStateRHT->setParentCumulativeTransform(pThis->m_workspaceXform);
	hdEndFrame(pThis->m_RhHD);
	return HD_CALLBACK_DONE;
}

//HDCallbackCode HDCALLBACK HapticsEventManager::RsetDeviceTransformCallback(
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	IHapticDevice::IHapticDeviceState *pStateGT =
//		pThis->m_pHapticDeviceRGT->getCurrentState();
//	IHapticDevice::IHapticDeviceState *pStateHT =
//		pThis->m_pHapticDeviceRHT->getCurrentState();
//
//	pStateGT->setParentCumulativeTransform(pThis->m_workspaceXform);
//	pStateHT->setParentCumulativeTransform(pThis->m_workspaceXform);
//
//	return HD_CALLBACK_DONE;
//}

/******************************************************************************
This handler gets called in the graphics thread whenever the device makes
contact with a constraint. Provide a visual cue (i.e. highlighting) to
accompany the haptic cue of being snapped to the point.
******************************************************************************/
void HapticsEventManager::madeContactCallbackGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
}

//void HapticsEventManager::madeContactCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//}

/******************************************************************************
This handler gets called in the graphics thread whenever the device loses
contact with a constraint. Provide a visual cue (i.e. highlighting) to
accompany the haptic cue of losing contact with the point.
******************************************************************************/
void HapticsEventManager::lostContactCallbackGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
}
//
//void HapticsEventManager::lostContactCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//}


/******************************************************************************
This handler gets called in the graphics thread whenever a button press is
detected. Interpret the click as a drilling on/off.
******************************************************************************/
void HapticsEventManager::button1UpClickCallbackGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
	ABCSketchManager *skAPI = pThis->abcSketchManager;

	
	hdMakeCurrentDevice(pThis->m_LhHD);
	//cerr << "Yay - it worked" << endl;		
	//skAPI->recordData = false;
	//skAPI->DataRecording(false);
	//skAPI->StartTimer();
	forceOn = false;
	
	//skAPI->stiffness = 0.0;
}

void HapticsEventManager::button1DownClickCallbackGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
	ABCSketchManager *skAPI = pThis->abcSketchManager;	
	//cerr << "Yay - it worked again" << endl;	
	
	hdMakeCurrentDevice(pThis->m_LhHD);
	//cerr << "ForceVal->" << forceOn << endl;
	//skAPI->recordData = true;
	skAPI->StartTimer();
	forceOn = true;
	leftpivot[0] = Lprev_stylus[0];
	leftpivot[1] = Lprev_stylus[1];
	leftpivot[2] = Lprev_stylus[2];

	rightpivot[0] = Rprev_stylus[0];
	rightpivot[1] = Rprev_stylus[1];
	rightpivot[2] = Rprev_stylus[2];	
	
}

//void HapticsEventManager::button1UpClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//	//cerr << "Yay - it worked too on Right" << endl;
//
//}
//
//void HapticsEventManager::button1DownClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//	//hdMakeCurrentDevice(pThis->m_RhHD);
//	//cerr << "Yay - it worked again on Right" << endl;
//	
//}

void HapticsEventManager::button2UpClickCallbackGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	ABCSketchManager *skAPI = pThis->abcSketchManager;
}

void HapticsEventManager::button2DownClickCallbackGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
	ABCSketchManager *skAPI = pThis->abcSketchManager;
}

//void HapticsEventManager::button2UpClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//}
//
//void HapticsEventManager::button2DownClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//}


/******************************************************************************
This handler gets called to handle errors
******************************************************************************/
void HapticsEventManager::errorCallbackGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	if (hduIsForceError(&pState->getLastError()))
	{
	}
	else
	{
		/* This is likely a more serious error, so just bail. */
		std::cerr << pState->getLastError() << std::endl;
		std::cerr << "Error during haptic rendering" << std::endl;
		std::cerr << "Press any key to quit." << std::endl;
		getchar();
		exit(-1);
	}
}

//void HapticsEventManager::errorCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	if (hduIsForceError(&pState->getLastError()))
//	{
//	}
//	else
//	{
//		/* This is likely a more serious error, so just bail. */
//		std::cerr << pState->getLastError() << std::endl;
//		std::cerr << "Error during haptic rendering" << std::endl;
//		std::cerr << "Press any key to quit." << std::endl;
//		getchar();
//		exit(-1);
//	}
//}
