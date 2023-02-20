#include "HapticsEventManager.h"


using namespace midl;

float prev_stylus[3], curr_stylus[3], stylus[3], displacement[3], pivot[3], left_stylus[3], right_stylus[3];
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
	//m_RhHD(HD_INVALID_HANDLE),
	m_LhUpdateCallback(HD_INVALID_HANDLE),
	//m_RhUpdateCallback(HD_INVALID_HANDLE),
	m_pHapticDeviceLHT(0),
	m_pHapticDeviceLGT(0),
	/*m_pHapticDeviceRHT(0),
	m_pHapticDeviceRGT(0),*/
	abcSketchManager(0),
	m_nCursorDisplayList(0)
{}

/*******************************************************************************
HapticsEventManager Destructor.
*******************************************************************************/
HapticsEventManager::~HapticsEventManager(){}

/*******************************************************************************
This is the main initialization needed for the haptic glue code.
*******************************************************************************/
void HapticsEventManager::Setup(ABCSketchManager *skManager)
{	
	HDErrorInfo error;

	/* Intialize a device configuration. */

	//m_RhHD = hdInitDevice("Right Device");
	//m_LhHD = hdInitDevice("Left Device");
	m_LhHD = hdInitDevice(HD_DEFAULT_DEVICE);

	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		std::cerr << error << std::endl;
		std::cerr << "Failed to initialize haptic device" << std::endl;
		std::cerr << "Press any key to quit." << std::endl;
		getchar();
		exit(-1);
	}

	/* Create the IHapticDevice instances for the haptic and graphic threads
	These interfaces are useful for handling the synchronization of
	state between the two main threads. */
	
	m_pHapticDeviceLHT = IHapticDevice::create(
		IHapticDevice::HAPTIC_THREAD_INTERFACE, m_LhHD);

	/*m_pHapticDeviceRHT = IHapticDevice::create(
		IHapticDevice::HAPTIC_THREAD_INTERFACE, m_RhHD);*/

	m_pHapticDeviceLGT = IHapticDevice::create(
		IHapticDevice::GRAPHIC_THREAD_INTERFACE, m_LhHD);

	/*m_pHapticDeviceRGT = IHapticDevice::create(
		IHapticDevice::GRAPHIC_THREAD_INTERFACE, m_RhHD);*/

	/* Setup callbacks so we can be notified about events in the graphics
	thread. */
	
	
	
	m_pHapticDeviceLGT->setCallback(
		IHapticDevice::MADE_CONTACT, madeContactCallbackLGT, this);
	
	m_pHapticDeviceLGT->setCallback(
		IHapticDevice::LOST_CONTACT, lostContactCallbackLGT, this);
	

	m_pHapticDeviceLGT->setCallback(
		IHapticDevice::BUTTON_1_UP, button1UpClickCallbackLGT, this);
	

	m_pHapticDeviceLGT->setCallback(
		IHapticDevice::BUTTON_1_DOWN, button1DownClickCallbackLGT, this);
	

	m_pHapticDeviceLGT->setCallback(
		IHapticDevice::BUTTON_2_UP, button2UpClickCallbackLGT, this);
	

	m_pHapticDeviceLGT->setCallback(
		IHapticDevice::BUTTON_2_DOWN, button2DownClickCallbackLGT, this);
	

	m_pHapticDeviceLGT->setCallback(
		IHapticDevice::DEVICE_ERROR, errorCallbackLGT, this);
	
	//hdMakeCurrentDevice(m_LhHD);
	hdEnable(HD_FORCE_OUTPUT);
	

	
	//hdMakeCurrentDevice(m_RhHD);
	
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::MADE_CONTACT, madeContactCallbackRGT, this);

	//
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::LOST_CONTACT, lostContactCallbackRGT, this);

	//
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_1_UP, button1UpClickCallbackRGT, this);

	//
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_1_DOWN, button1DownClickCallbackRGT, this);

	//
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_2_UP, button2UpClickCallbackRGT, this);

	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::BUTTON_2_DOWN, button2DownClickCallbackRGT, this);

	//
	//m_pHapticDeviceRGT->setCallback(
	//	IHapticDevice::DEVICE_ERROR, errorCallbackRGT, this);

	//hdEnable(HD_FORCE_OUTPUT);	

	/* Schedule a haptic thread callback for updating the device every
	tick of the servo loop. */

	/*m_LhUpdateCallback = hdScheduleAsynchronous(LdeviceUpdateCallback, this, HD_MAX_SCHEDULER_PRIORITY);
	m_RhUpdateCallback = hdScheduleAsynchronous(RdeviceUpdateCallback, this, HD_MAX_SCHEDULER_PRIORITY);*/
	m_LhUpdateCallback = hdScheduleAsynchronous(LdeviceUpdateCallback, this, HD_MAX_SCHEDULER_PRIORITY);
	//m_RhUpdateCallback = hdScheduleAsynchronous(RdeviceUpdateCallback, this, HD_DEFAULT_SCHEDULER_PRIORITY);

	/* Start the scheduler to get the haptic loop going. */
	hdStartScheduler();
	if (HD_DEVICE_ERROR(error = hdGetError()))
	{
		std::cerr << error << std::endl;
		std::cerr << "Failed to start scheduler" << std::endl;
		std::cerr << "Press any key to quit." << std::endl;
		getchar();
		exit(-1);
	}

	// GRAPHICS SETUP
	abcSketchManager =  skManager;
	//diffuseShader.Initialize(".//Shaders//diffuseShader.vert", ".//Shaders//diffuseShader.frag");
	isForceOn = false;
}

/*******************************************************************************
Reverse the setup process by shutting down and destructing the services
used by the HapticsEventManager.
*******************************************************************************/
void HapticsEventManager::Cleanup()
{
	hdStopScheduler();

	if (m_LhUpdateCallback != HD_INVALID_HANDLE)
	{
		hdUnschedule(m_LhUpdateCallback);
		m_LhUpdateCallback = HD_INVALID_HANDLE;
	}

	/*if (m_RhUpdateCallback != HD_INVALID_HANDLE)
	{
		hdUnschedule(m_RhUpdateCallback);
		m_RhUpdateCallback = HD_INVALID_HANDLE;
	}
*/
	if (m_LhHD != HD_INVALID_HANDLE)
	{
		hdDisableDevice(m_LhHD);
		m_LhHD = HD_INVALID_HANDLE;
	}

	/*if (m_RhHD != HD_INVALID_HANDLE)
	{
		hdDisableDevice(m_RhHD);
		m_RhHD = HD_INVALID_HANDLE;
	}*/

	IHapticDevice::destroy(m_pHapticDeviceLGT);
	IHapticDevice::destroy(m_pHapticDeviceLHT);

	/*IHapticDevice::destroy(m_pHapticDeviceRGT);
	IHapticDevice::destroy(m_pHapticDeviceRHT);*/

	glDeleteLists(m_nCursorDisplayList, 1);
}

/*******************************************************************************
This method will get called every tick of the graphics loop. It is primarily
responsible for synchronizing state with the haptics thread as well as
updating snap state.
*******************************************************************************/
void HapticsEventManager::UpdateState()
{
	//cerr << "here I am" << endl;
	/* Capture the latest state from the servoloop. */
	/*hlBeginFrame();
	hdMakeCurrentDevice(m_LhHD);
	m_pHapticDeviceLGT->beginUpdate(m_pHapticDeviceLHT);
	m_pHapticDeviceLGT->endUpdate(m_pHapticDeviceLHT);
	hlEndFrame();

	hlBeginFrame();
	hdMakeCurrentDevice(m_RhHD);
	m_pHapticDeviceLGT->beginUpdate(m_pHapticDeviceRHT);
	m_pHapticDeviceLGT->endUpdate(m_pHapticDeviceRHT);
	hlEndFrame();*/

	m_pHapticDeviceRGT->beginUpdate(m_pHapticDeviceLHT);
	m_pHapticDeviceRGT->endUpdate(m_pHapticDeviceLHT);
}

/*******************************************************************************
Uses the current OpenGL viewing transforms to determine a mapping from device
coordinates to world coordinates.
*******************************************************************************/
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

	hdScheduleSynchronous(LsetDeviceTransformCallback, this, SYNCHRONIZE_STATE_PRIORITY);
	//hdScheduleSynchronous(RsetDeviceTransformCallback, this, SYNCHRONIZE_STATE_PRIORITY);
}

void HapticsEventManager::RenderStylus()
{
	prev_stylus[0] = left_stylus[0];
	prev_stylus[1] = left_stylus[1];
	prev_stylus[2] = left_stylus[2];
}

/*******************************************************************************
Draws a 3D cursor using the current device transform and the workspace
to world transform.
*******************************************************************************/

/******************************************************************************
Scheduler Callbacks
These callback routines get performed in the haptics thread.
******************************************************************************/

/******************************************************************************
This is the main haptic thread scheduler callback. It handles updating the
currently applied constraint.
******************************************************************************/

HDCallbackCode HDCALLBACK HapticsEventManager::LdeviceUpdateCallback(
	void *pUserData)
{		
	
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
	//hdMakeCurrentDevice(pThis->m_LhHD);

	/* Force the haptic device to update its state. */
	pThis->m_pHapticDeviceLHT->beginUpdate(0);
	
	IHapticDevice::IHapticDeviceState *pLCurrentState =
		pThis->m_pHapticDeviceLHT->getCurrentState();
	
	IHapticDevice::IHapticDeviceState *pLLastState =
		pThis->m_pHapticDeviceLHT->getLastState();

	/* Get the position of the device. */
	hduVector3Dd devicePositionLLC = pLCurrentState->getPosition();
		
	left_stylus[0] = (float)devicePositionLLC[0]; left_stylus[1] = (float)devicePositionLLC[1]; left_stylus[2] = (float)devicePositionLLC[2];
	
	float forceVector[] = {0.0,0.0,0.0};

	/*float xRng[] = { -172.0, 172.0 }; float yRng[] = { -108.0, 50.0 }; float zRng[] = { -56.0, -32.0 };
	float xLen = xRng[1] - xRng[0]; float yLen = yRng[1] - yRng[0]; float zLen = zRng[1] - zRng[0];

	float newRngX[] = { -1, 1 };
	float newRngY[] = { -0.75, 0.75 };
	float newRngZ[] = { -0.088, 0.088 };
	float RngX = newRngX[1] - newRngX[0];
	float RngY = newRngY[1] - newRngY[0];
	float RngZ = newRngZ[1] - newRngZ[0];

	stylus[0] = RngX * (((float)devicePositionLC[0] - xRng[0]) / xLen) + newRngX[0];
	stylus[1] = RngY * (((float)devicePositionLC[1] - yRng[0]) / yLen) + newRngY[0];
	stylus[2] = RngZ * (((float)devicePositionLC[2] - zRng[0]) / zLen) + newRngZ[0];

	curr_stylus[0] = stylus[0];
	curr_stylus[1] = stylus[1];
	curr_stylus[2] = stylus[2];*/

	pThis->RenderStylus();	

	ABCSketchManager *skAPI = pThis->abcSketchManager;
	
	skAPI->HapListen(left_stylus);
	//skAPI->Listen(stylus);
	//bool isHapticsActive = skAPI->CheckHapticsForce();
	//skAPI->SetPivot(prev_stylus);
	//float pivot[3], 

	/*displacement[0] = prev_stylus[0] - curr_stylus[0];
	displacement[1] = prev_stylus[1] - curr_stylus[1];
	displacement[2] = prev_stylus[2] - curr_stylus[2];
	float disp[3];
	Normalize3(displacement);*/

	//cout << "planeDistance->" << planeDist << endl;
	//string option = skAPI->option;
	
	//bool isSketchOn = skAPI->isSketchOn;
	//bool isHapticsOn = skAPI->CheckHapticsForce();
	//cerr << "isHapticsOn" << isHapticsOn << endl;

	SubVectors3(left_stylus, pivot, displacement);
	float disp = Norm3(displacement);	

	hduVector3Dd force;
	//cout << "The Stiffness is->" << skAPI->stiffness << endl;
	if (pThis->isForceOn == true)
	{
		forceVector[0] = 0.0; forceVector[1] = 0.0; forceVector[2] = 2.0 * displacement[2];
		force.set((HDdouble)forceVector[0], (HDdouble)forceVector[1], (HDdouble)forceVector[2]);
	}

	hdSetDoublev(HD_CURRENT_FORCE, force);
	/*if (startSketch)
	force.set((HDdouble)forceVector[0], (HDdouble)forceVector[1], (HDdouble)forceVector[2]);
	else force.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);*/

	/*hduVector3Dd force;
	if (startSketch)
		force.set((HDdouble)forceVector[0], (HDdouble)forceVector[1], (HDdouble)forceVector[2]);
	else force.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);*/
	

	//if (pCurrentState->isInContact())
	//{
	//	/* If currently in contact, use the freshest contact data. */
	//	pCurrentState->setContactData(pSnapAPI->getConstraint()->getUserData());
	//}
	//else if (pLastState->isInContact())
	//{
	//	/* If was in contact the last frame, use that contact data, since it
	//	will get reported to the event callbacks. */
	//	pCurrentState->setContactData(pLastState->getContactData());
	//}
	//else
	//{
	//	pCurrentState->setContactData((void *)-1);
	//}

	///* Transform result from world coordinates back to device coordinates. */
	//hduVector3Dd proxyPositionLC = pSnapAPI->getConstrainedProxy();
	//pCurrentState->setProxyPosition(proxyPositionLC);

	//hdGetDoublev(HD_CURRENT_TRANSFORM, pCurrentState->getProxyTransform());
	//pCurrentState->getProxyTransform()[12] = proxyPositionLC[0];
	//pCurrentState->getProxyTransform()[13] = proxyPositionLC[1];
	//pCurrentState->getProxyTransform()[14] = proxyPositionLC[2];

	//double kStiffness;
	//hdGetDoublev(HD_NOMINAL_MAX_STIFFNESS, &kStiffness);
	//kStiffness = hduMin(0.4, kStiffness);

	///* Compute spring force to attract device to constrained proxy. */
	//hduVector3Dd force = kStiffness * (proxyPositionLC - devicePositionLC);
	//hdSetDoublev(HD_CURRENT_FORCE, force);

	pThis->m_pHapticDeviceLHT->endUpdate(0);
	return HD_CALLBACK_CONTINUE;
}

//HDCallbackCode HDCALLBACK HapticsEventManager::RdeviceUpdateCallback(
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);	
//
//	/* Force the haptic device to update its state. */
//	pThis->m_pHapticDeviceRHT->beginUpdate(0);
//
//	IHapticDevice::IHapticDeviceState *pLCurrentState =
//		pThis->m_pHapticDeviceRHT->getCurrentState();
//	
//	IHapticDevice::IHapticDeviceState *pLLastState =
//		pThis->m_pHapticDeviceRHT->getLastState();
//	
//
//	/* Get the position of the device. */
//	hduVector3Dd devicePositionRLC = pLCurrentState->getPosition();
//	
//	right_stylus[0] = (float)devicePositionRLC[0]; right_stylus[1] = (float)devicePositionRLC[1]; right_stylus[2] = (float)devicePositionRLC[2];
//
//	/*cout << "X->" << right_stylus[0] << endl;
//	cout << "Y->" << right_stylus[1] << endl;
//	cout << "Z->" << right_stylus[2] << endl;*/
//
//	float forceVector[] = { 0.0,0.0,0.0 };
//
//	/*float xRng[] = { -172.0, 172.0 }; float yRng[] = { -108.0, 50.0 }; float zRng[] = { -56.0, -32.0 };
//	float xLen = xRng[1] - xRng[0]; float yLen = yRng[1] - yRng[0]; float zLen = zRng[1] - zRng[0];
//
//	float newRngX[] = { -1, 1 };
//	float newRngY[] = { -0.75, 0.75 };
//	float newRngZ[] = { -0.088, 0.088 };
//	float RngX = newRngX[1] - newRngX[0];
//	float RngY = newRngY[1] - newRngY[0];
//	float RngZ = newRngZ[1] - newRngZ[0];
//
//	stylus[0] = RngX * (((float)devicePositionLC[0] - xRng[0]) / xLen) + newRngX[0];
//	stylus[1] = RngY * (((float)devicePositionLC[1] - yRng[0]) / yLen) + newRngY[0];
//	stylus[2] = RngZ * (((float)devicePositionLC[2] - zRng[0]) / zLen) + newRngZ[0];
//
//	curr_stylus[0] = stylus[0];
//	curr_stylus[1] = stylus[1];
//	curr_stylus[2] = stylus[2];*/
//
//	//pThis->RenderStylus();	
//
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//	//skAPI->HapListen(left_stylus);
//
//	//skAPI->Listen(stylus);
//	//bool isHapticsActive = skAPI->CheckHapticsForce();
//	//skAPI->SetPivot(prev_stylus);
//	//float pivot[3], 
//
//	//float radius;
//
//	/*displacement[0] = prev_stylus[0] - curr_stylus[0];
//	displacement[1] = prev_stylus[1] - curr_stylus[1];
//	displacement[2] = prev_stylus[2] - curr_stylus[2];
//	float disp[3];
//	Normalize3(displacement);*/
//
//	//float rotaxis[3];
//	//rotaxis[0] = displacement[1];
//	//rotaxis[1] = -displacement[0];
//	//rotaxis[2] = 0.0;
//
//	//Normalize3(displacement);
//	//float rotangle = -0.008*Norm3(displacement);
//	//float vertex[3];	
//
//	//cout << "planeDistance->" << planeDist << endl;
//	//string option = skAPI->option;
//
//	//bool isSketchOn = skAPI->isSketchOn;
//	//bool isHapticsOn = skAPI->CheckHapticsForce();
//	//cerr << "isHapticsOn" << isHapticsOn << endl;
//
//
//	/*SubVectors3(left_stylus, pivot, displacement);
//	float disp = Norm3(displacement);*/
//
//	SubVectors3(left_stylus, pivot, displacement);
//	float disp = Norm3(displacement);
//
//	hduVector3Dd force;
//	//cout << "The Stiffness is->" << skAPI->stiffness << endl;
//	if (pThis->isForceOn)
//	{
//		forceVector[0] = 0.0; forceVector[1] = 0.0; forceVector[2] = 2.0 * displacement[2];
//		force.set((HDdouble)forceVector[0], (HDdouble)forceVector[1], (HDdouble)forceVector[2]);
//	}
//	hdSetDoublev(HD_CURRENT_FORCE, force);
//
//	/*if (startSketch)
//	force.set((HDdouble)forceVector[0], (HDdouble)forceVector[1], (HDdouble)forceVector[2]);
//	else force.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);*/
//
//	/*hduVector3Dd force;
//	if (startSketch)
//		force.set((HDdouble)forceVector[0], (HDdouble)forceVector[1], (HDdouble)forceVector[2]);
//	else force.set((HDdouble)0.0, (HDdouble)0.0, (HDdouble)0.0);*/
//	
//
//	//if (pCurrentState->isInContact())
//	//{
//	//	/* If currently in contact, use the freshest contact data. */
//	//	pCurrentState->setContactData(pSnapAPI->getConstraint()->getUserData());
//	//}
//	//else if (pLastState->isInContact())
//	//{
//	//	/* If was in contact the last frame, use that contact data, since it
//	//	will get reported to the event callbacks. */
//	//	pCurrentState->setContactData(pLastState->getContactData());
//	//}
//	//else
//	//{
//	//	pCurrentState->setContactData((void *)-1);
//	//}
//
//	///* Transform result from world coordinates back to device coordinates. */
//	//hduVector3Dd proxyPositionLC = pSnapAPI->getConstrainedProxy();
//	//pCurrentState->setProxyPosition(proxyPositionLC);
//
//	//hdGetDoublev(HD_CURRENT_TRANSFORM, pCurrentState->getProxyTransform());
//	//pCurrentState->getProxyTransform()[12] = proxyPositionLC[0];
//	//pCurrentState->getProxyTransform()[13] = proxyPositionLC[1];
//	//pCurrentState->getProxyTransform()[14] = proxyPositionLC[2];
//
//	//double kStiffness;
//	//hdGetDoublev(HD_NOMINAL_MAX_STIFFNESS, &kStiffness);
//	//kStiffness = hduMin(0.4, kStiffness);
//
//	///* Compute spring force to attract device to constrained proxy. */
//	//hduVector3Dd force = kStiffness * (proxyPositionLC - devicePositionLC);
//	//hdSetDoublev(HD_CURRENT_FORCE, force);
//
//	pThis->m_pHapticDeviceRHT->endUpdate(0);
//
//	return HD_CALLBACK_CONTINUE;
//}

/******************************************************************************
Scheduler callback to set the workspace transform both for use in the graphics
thread and haptics thread.
******************************************************************************/
HDCallbackCode HDCALLBACK HapticsEventManager::LsetDeviceTransformCallback(
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	IHapticDevice::IHapticDeviceState *pStateLGT =
		pThis->m_pHapticDeviceLGT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pStateLHT =
		pThis->m_pHapticDeviceLHT->getCurrentState();

	/*IHapticDevice::IHapticDeviceState *pStateRGT =
		pThis->m_pHapticDeviceLGT->getCurrentState();
	IHapticDevice::IHapticDeviceState *pStateRHT =
		pThis->m_pHapticDeviceLHT->getCurrentState();
*/
	pStateLGT->setParentCumulativeTransform(pThis->m_workspaceXform);
	pStateLHT->setParentCumulativeTransform(pThis->m_workspaceXform);

	return HD_CALLBACK_DONE;
}

//HDCallbackCode HDCALLBACK HapticsEventManager::RsetDeviceTransformCallback(
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	IHapticDevice::IHapticDeviceState *pStateRGT =
//		pThis->m_pHapticDeviceRGT->getCurrentState();
//	IHapticDevice::IHapticDeviceState *pStateRHT =
//		pThis->m_pHapticDeviceRHT->getCurrentState();
//
//	pStateRGT->setParentCumulativeTransform(pThis->m_workspaceXform);
//	pStateRHT->setParentCumulativeTransform(pThis->m_workspaceXform);
//
//	/*pStateRGT->setParentCumulativeTransform(pThis->m_workspaceXform);
//	pStateRHT->setParentCumulativeTransform(pThis->m_workspaceXform);*/
//
//	return HD_CALLBACK_DONE;
//}


/******************************************************************************
Event Callbacks

These are event callbacks that are registered with the IHapticDevice
******************************************************************************/

/******************************************************************************
This handler gets called in the graphics thread whenever the device makes
contact with a constraint. Provide a visual cue (i.e. highlighting) to
accompany the haptic cue of being snapped to the point.
******************************************************************************/
void HapticsEventManager::madeContactCallbackLGT(
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
//
//}

/******************************************************************************
This handler gets called in the graphics thread whenever the device loses
contact with a constraint. Provide a visual cue (i.e. highlighting) to
accompany the haptic cue of losing contact with the point.
******************************************************************************/
void HapticsEventManager::lostContactCallbackLGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
}

//void HapticsEventManager::lostContactCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//}

/******************************************************************************
This handler gets called in the graphics thread whenever a button press is
detected. Interpret the click as a drilling on/off.
******************************************************************************/

void HapticsEventManager::button1UpClickCallbackLGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
	ABCSketchManager *skAPI = pThis->abcSketchManager;
	
	//hdMakeCurrentDevice(pThis->m_LhHD);
	cerr << "Button Pressed lEFT" << endl;
}

//void HapticsEventManager::button1UpClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//	//cout << "Here" << endl;
//	cerr << "Button Pressed Right" << endl;
//}

void HapticsEventManager::button1DownClickCallbackLGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	ABCSketchManager *skAPI = pThis->abcSketchManager;
	pThis->isForceOn = true;
	
	//hdMakeCurrentDevice(pThis->m_LhHD);
	cerr << "Button Pressed lEFT" << endl;
	
	/*pThis->RenderStylus();
	pivot[0] = prev_stylus[0];
	pivot[1] = prev_stylus[1];
	pivot[2] = prev_stylus[2];*/

	//cout << pivot[0] << endl;
	//cout << pivot[1] << endl;
	//cout << pivot[2] << endl;	
}

//void HapticsEventManager::button1DownClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//	pThis->isForceOn = true;
//	cerr << "Button Pressed Right" << endl;
//	//hdEnable(HD_CURRENT_BUTTONS);
//
//	pThis->RenderStylus();
//	pivot[0] = prev_stylus[0];
//	pivot[1] = prev_stylus[1];
//	pivot[2] = prev_stylus[2];
//
//	/*pThis->RenderStylus();
//	pivot[0] = prev_stylus[0];
//	pivot[1] = prev_stylus[1];
//	pivot[2] = prev_stylus[2];*/
//
//	//cout << pivot[0] << endl;
//	//cout << pivot[1] << endl;
//	//cout << pivot[2] << endl;	
//}

void HapticsEventManager::button2UpClickCallbackLGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	ABCSketchManager *skAPI = pThis->abcSketchManager;
	//skAPI->ManipulationOff();
	
}

//void HapticsEventManager::button2UpClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//	//skAPI->ManipulationOff();
//
//}

void HapticsEventManager::button2DownClickCallbackLGT(
	IHapticDevice::EventType event,
	const IHapticDevice::IHapticDeviceState * const pState,
	void *pUserData)
{
	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);

	ABCSketchManager *skAPI = pThis->abcSketchManager;
	//skAPI->TranslationOn();
}

//void HapticsEventManager::button2DownClickCallbackRGT(
//	IHapticDevice::EventType event,
//	const IHapticDevice::IHapticDeviceState * const pState,
//	void *pUserData)
//{
//	HapticsEventManager *pThis = static_cast<HapticsEventManager *>(pUserData);
//
//	ABCSketchManager *skAPI = pThis->abcSketchManager;
//	//skAPI->TranslationOn();
//}


/******************************************************************************
This handler gets called to handle errors
******************************************************************************/
void HapticsEventManager::errorCallbackLGT(
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

/******************************************************************************/

