#include "common.h"
#include "patcher.h"
#include "main.h"
#include "Draw.h"
#include "World.h"
#include "Vehicle.h"
#include "Automobile.h"
#include "Ped.h"
#include "PlayerPed.h"
#include "CopPed.h"
#include "RpAnimBlend.h"
#include "ControllerConfig.h"
#include "Pad.h"
#include "Frontend.h"
#include "General.h"
#include "Renderer.h"
#include "Shadows.h"
#include "Hud.h"
#include "ZoneCull.h"
#include "SurfaceTable.h"
#include "WaterLevel.h"
#include "MBlur.h"
#include "SceneEdit.h"
#include "Debug.h"
#include "Camera.h"

const float DefaultFOV = 70.0f;	// beta: 80.0f

bool PrintDebugCode = false;
int16 &DebugCamMode = *(int16*)0x95CCF2;

void
CCam::Init(void)
{
	Mode = MODE_FOLLOWPED;
	Front = CVector(0.0f, 0.0f, -1.0f);
	Up = CVector(0.0f, 0.0f, 1.0f);
	Rotating = 0;
	m_iDoCollisionChecksOnFrameNum = 1;
	m_iDoCollisionCheckEveryNumOfFrames = 9;
	m_iFrameNumWereAt = 0;
	m_bCollisionChecksOn = 1;
	m_fRealGroundDist = 0.0f;
	BetaSpeed = 0.0f;
	AlphaSpeed = 0.0f;
	DistanceSpeed = 0.0f;
	f_max_role_angle = DEGTORAD(5.0f);
	Distance = 30.0f;
	DistanceSpeed = 0.0f;
	m_pLastCarEntered = 0;
	m_pLastPedLookedAt = 0;
	ResetStatics = 1;
	Beta = 0.0f;
	m_bFixingBeta = 0;
	CA_MIN_DISTANCE = 0.0f;
	CA_MAX_DISTANCE = 0.0f;
	LookingBehind = 0;
	LookingLeft = 0;
	LookingRight = 0;
	m_fPlayerInFrontSyphonAngleOffSet = DEGTORAD(20.0f);
	m_fSyphonModeTargetZOffSet = 0.5f;
	m_fRadiusForDead = 1.5f;
	DirectionWasLooking = LOOKING_FORWARD;
	LookBehindCamWasInFront = 0;
	f_Roll = 0.0f;
	f_rollSpeed = 0.0f;
	m_fCloseInPedHeightOffset = 0.0f;
	m_fCloseInPedHeightOffsetSpeed = 0.0f;
	m_fCloseInCarHeightOffset = 0.0f;
	m_fCloseInCarHeightOffsetSpeed = 0.0f;
	m_fPedBetweenCameraHeightOffset = 0.0f;
	m_fTargetBeta = 0.0f;
	m_fBufferedTargetBeta = 0.0f;
	m_fBufferedTargetOrientation = 0.0f;
	m_fBufferedTargetOrientationSpeed = 0.0f;
	m_fDimensionOfHighestNearCar = 0.0f;
	m_fRoadOffSet = 0.0f;
}

void
CCam::Process(void)
{
	CVector CameraTarget;
	float TargetSpeedVar = 0.0f;
	float TargetOrientation = 0.0f;

	if(CamTargetEntity == nil)
		CamTargetEntity = TheCamera.pTargetEntity;

	m_iFrameNumWereAt++;
	if(m_iFrameNumWereAt > m_iDoCollisionCheckEveryNumOfFrames)
		m_iFrameNumWereAt = 1;
	m_bCollisionChecksOn = m_iFrameNumWereAt == m_iDoCollisionChecksOnFrameNum;

	if(m_bCamLookingAtVector){
		CameraTarget = m_cvecCamFixedModeVector;
	}else if(CamTargetEntity->IsVehicle()){
		CameraTarget = CamTargetEntity->GetPosition();

		if(CamTargetEntity->GetForward().x == 0.0f && CamTargetEntity->GetForward().y == 0.0f)
			TargetOrientation = 0.0f;
		else
			TargetOrientation = CGeneral::GetATanOfXY(CamTargetEntity->GetForward().x, CamTargetEntity->GetForward().y);

		CVector Fwd(0.0f, 0.0f, 0.0f);
		Fwd.x = CamTargetEntity->GetForward().x;
		Fwd.y = CamTargetEntity->GetForward().y;
		Fwd.Normalise();
		// Game normalizes again here manually. useless, so skipped

		float FwdSpeedX = ((CVehicle*)CamTargetEntity)->GetMoveSpeed().x * Fwd.x;
		float FwdSpeedY = ((CVehicle*)CamTargetEntity)->GetMoveSpeed().y * Fwd.y;
		if(FwdSpeedX + FwdSpeedY > 0.0f)
			TargetSpeedVar = min(Sqrt(SQR(FwdSpeedX) + SQR(FwdSpeedY))/0.9f, 1.0f);
		else
			TargetSpeedVar = -min(Sqrt(SQR(FwdSpeedX) + SQR(FwdSpeedY))/1.8f, 0.5f);
		SpeedVar = 0.895f*SpeedVar + 0.105*TargetSpeedVar;
	}else{
		CameraTarget = CamTargetEntity->GetPosition();

		if(CamTargetEntity->GetForward().x == 0.0f && CamTargetEntity->GetForward().y == 0.0f)
			TargetOrientation = 0.0f;
		else
			TargetOrientation = CGeneral::GetATanOfXY(CamTargetEntity->GetForward().x, CamTargetEntity->GetForward().y);
		TargetSpeedVar = 0.0f;
		SpeedVar = 0.0f;
	}

	switch(Mode){
	case MODE_TOPDOWN:
	case MODE_GTACLASSIC:
		Process_TopDown(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_BEHINDCAR:
		Process_BehindCar(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FOLLOWPED:
		if(CCamera::m_bUseMouse3rdPerson)
			Process_FollowPedWithMouse(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		else
			Process_FollowPed(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_AIMING:
	case MODE_DEBUG:
		Process_Debug(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_SNIPER:
		Process_Sniper(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_ROCKETLAUNCHER:
		Process_Rocket(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_MODELVIEW:
		Process_ModelView(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_BILL:
	case MODE_SYPHON:
		Process_Syphon(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_CIRCLE:
		Process_Circle(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_CHEESYZOOM:
	case MODE_WHEELCAM:
		Process_WheelCam(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FIXED:
		Process_Fixed(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_1STPERSON:
		Process_1stPerson(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FLYBY:
		Process_FlyBy(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_CAM_ON_A_STRING:
		Process_Cam_On_A_String(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_REACTION:
//	case MODE_FOLLOW_PED_WITH_BIND:
//	case MODE_CHRIS:
	case MODE_BEHINDBOAT:
		Process_BehindBoat(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_PLAYER_FALLEN_WATER:
		Process_Player_Fallen_Water(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
//	case MODE_CAM_ON_TRAIN_ROOF:
//	case MODE_CAM_RUNNING_SIDE_TRAIN:
//	case MODE_BLOOD_ON_THE_TRACKS:
//	case MODE_IM_THE_PASSENGER_WOOWOO:
	case MODE_SYPHON_CRIM_IN_FRONT:
		Process_Syphon_Crim_In_Front(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_PED_DEAD_BABY:
		ProcessPedsDeadBaby();
		break;
//	case MODE_PILLOWS_PAPS:
//	case MODE_LOOK_AT_CARS:
	case MODE_ARRESTCAM_ONE:
		ProcessArrestCamOne();
		break;
	case MODE_ARRESTCAM_TWO:
		ProcessArrestCamTwo();
		break;
	case MODE_M16_1STPERSON:
	case MODE_HELICANNON_1STPERSON:	// miami
		Process_M16_1stPerson(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_SPECIAL_FIXED_FOR_SYPHON:
		Process_SpecialFixedForSyphon(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_FIGHT_CAM:
		Process_Fight_Cam(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_TOP_DOWN_PED:
		Process_TopDownPed(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_SNIPER_RUNABOUT:
	case MODE_ROCKETLAUNCHER_RUNABOUT:
	case MODE_1STPERSON_RUNABOUT:
	case MODE_M16_1STPERSON_RUNABOUT:
	case MODE_FIGHT_CAM_RUNABOUT:
		Process_1rstPersonPedOnPC(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	case MODE_EDITOR:
		Process_Editor(CameraTarget, TargetOrientation, SpeedVar, TargetSpeedVar);
		break;
	default:
		Source = CVector(0.0f, 0.0f, 0.0f);
		Front = CVector(0.0f, 1.0f, 0.0f);
		Up = CVector(0.0f, 0.0f, 1.0f);
	}

	CVector TargetToCam = Source - m_cvecTargetCoorsForFudgeInter;
	float DistOnGround = TargetToCam.Magnitude2D();
	m_fTrueBeta = CGeneral::GetATanOfXY(TargetToCam.x, TargetToCam.y);
	m_fTrueAlpha = CGeneral::GetATanOfXY(TargetToCam.z, DistOnGround);
	if(TheCamera.m_uiTransitionState == 0)	// TODO? what values are possible? enum?
		KeepTrackOfTheSpeed(Source, m_cvecTargetCoorsForFudgeInter, Up, m_fTrueAlpha, m_fTrueBeta, FOV);

	// Look Behind, Left, Right
	LookingBehind = false;
	LookingLeft = false;
	LookingRight = false;
	SourceBeforeLookBehind = Source;
	if(&TheCamera.Cams[TheCamera.ActiveCam] == this){
		if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_1STPERSON || Mode == MODE_BEHINDBOAT) &&
		   CamTargetEntity->IsVehicle()){
			if(CPad::GetPad(0)->GetLookBehindForCar()){
				LookBehind();
				if(DirectionWasLooking != LOOKING_BEHIND)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_BEHIND;
			}else if(CPad::GetPad(0)->GetLookLeft()){
				LookLeft();
				if(DirectionWasLooking != LOOKING_LEFT)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_LEFT;
			}else if(CPad::GetPad(0)->GetLookRight()){
				LookRight();
				if(DirectionWasLooking != LOOKING_RIGHT)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_RIGHT;
			}else{
				if(DirectionWasLooking != LOOKING_FORWARD)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_FORWARD;
			}
		}
		if(Mode == MODE_FOLLOWPED && CamTargetEntity->IsPed()){
			if(CPad::GetPad(0)->GetLookBehindForPed()){
				LookBehind();
				if(DirectionWasLooking != LOOKING_BEHIND)
					TheCamera.m_bJust_Switched = true;
				DirectionWasLooking = LOOKING_BEHIND;
			}else
				DirectionWasLooking = LOOKING_FORWARD;
		}
	}

	if(Mode == MODE_SNIPER || Mode == MODE_ROCKETLAUNCHER || Mode == MODE_M16_1STPERSON ||
	   Mode == MODE_1STPERSON || Mode == MODE_HELICANNON_1STPERSON || GetWeaponFirstPersonOn())
		ClipIfPedInFrontOfPlayer();
}

// MaxSpeed is a limit of how fast the value is allowed to change. 1.0 = to Target in up to 1ms
// Acceleration is how fast the speed will change to MaxSpeed. 1.0 = to MaxSpeed in 1ms
void
WellBufferMe(float Target, float *CurrentValue, float *CurrentSpeed, float MaxSpeed, float Acceleration, bool IsAngle)
{
	float Delta = Target - *CurrentValue;

	if(IsAngle){
		while(Delta >= PI) Delta -= 2*PI;
		while(Delta < -PI) Delta += 2*PI;
	}

	float TargetSpeed = Delta * MaxSpeed;
	// Add or subtract absolute depending on sign, genius!
//	if(TargetSpeed - *CurrentSpeed > 0.0f)
//		*CurrentSpeed += Acceleration * Abs(TargetSpeed - *CurrentSpeed) * CTimer::GetTimeStep();
//	else
//		*CurrentSpeed -= Acceleration * Abs(TargetSpeed - *CurrentSpeed) * CTimer::GetTimeStep();
	// this is simpler:
	*CurrentSpeed += Acceleration * (TargetSpeed - *CurrentSpeed) * CTimer::GetTimeStep();

	// Clamp speed if we overshot
	if(TargetSpeed < 0.0f && *CurrentSpeed < TargetSpeed)
		*CurrentSpeed = TargetSpeed;
	else if(TargetSpeed > 0.0f && *CurrentSpeed > TargetSpeed)
		*CurrentSpeed = TargetSpeed;

	*CurrentValue += *CurrentSpeed * min(10.0f, CTimer::GetTimeStep());
}

void
MakeAngleLessThan180(float &Angle)
{
	while(Angle >= PI) Angle -= 2*PI;
	while(Angle < -PI) Angle += 2*PI;
}

void
CCam::ProcessSpecialHeightRoutines(void)
{
	int i = 0;
	bool StandingOnBoat = false;
	static bool PreviouslyFailedRoadHeightCheck = false;
	CVector CamToTarget, CamToPed;
	float DistOnGround, BetaAngle;
	CPed *Player;
	int ClosestPed = 0;
	bool FoundPed = false;
	float ClosestPedDist, PedZDist;
	CColPoint colPoint;

	CamToTarget = TheCamera.pTargetEntity->GetPosition() - TheCamera.GetGameCamPosition();
	DistOnGround = CamToTarget.Magnitude2D();
	BetaAngle = CGeneral::GetATanOfXY(CamToTarget.x, CamToTarget.y);
	m_bTheHeightFixerVehicleIsATrain = false;
	ClosestPedDist = 0.0f;
	// CGeneral::GetATanOfXY(TheCamera.GetForward().x, TheCamera.GetForward().y);
	Player = CWorld::Players[CWorld::PlayerInFocus].m_pPed;

	if(DistOnGround > 10.0f)
		DistOnGround = 10.0f;

	if(CamTargetEntity && CamTargetEntity->IsPed()){
		if(FindPlayerPed()->m_pCurSurface && FindPlayerPed()->m_pCurSurface->IsVehicle() &&
		   ((CVehicle*)FindPlayerPed()->m_pCurSurface)->IsBoat())
			StandingOnBoat = true;

		// Move up the camera if there is a ped close to it
		if(Mode == MODE_FOLLOWPED || Mode == MODE_FIGHT_CAM){
			// Find ped closest to camera
			while(i < Player->m_numNearPeds){
				if(Player->m_nearPeds[i] && Player->m_nearPeds[i]->GetPedState() != PED_DEAD){
					CamToPed = Player->m_nearPeds[i]->GetPosition() - TheCamera.GetGameCamPosition();
					if(FoundPed){
						if(CamToPed.Magnitude2D() < ClosestPedDist){
							ClosestPed = i;
							ClosestPedDist = CamToPed.Magnitude2D();
						}
					}else{
						FoundPed = true;
						ClosestPed = i;
						ClosestPedDist = CamToPed.Magnitude2D();
					}
				}
				i++;
			}

			if(FoundPed){
				float Offset = 0.0f;
				CPed *Ped = Player->m_nearPeds[ClosestPed];
				CamToPed = Ped->GetPosition() - TheCamera.GetGameCamPosition();
				PedZDist = 0.0f;
				float dist = CamToPed.Magnitude2D();	// should be same as ClosestPedDist
				if(dist < 2.1f){
					// Ped is close to camera, move up

					// Z Distance between player and close ped
					PedZDist = 0.0f;
					if(Ped->bIsStanding)
						PedZDist = Ped->GetPosition().z - Player->GetPosition().z;
					// Ignore if too distant
					if(PedZDist > 1.2f || PedZDist < -1.2f)
						PedZDist = 0.0f;

					float DistScale = (2.1f - dist)/2.1f;
					if(Mode == MODE_FOLLOWPED){
						if(TheCamera.PedZoomIndicator == 1.0f)
							Offset = 0.45*DistScale + PedZDist;
						if(TheCamera.PedZoomIndicator == 2.0f)
							Offset = 0.35*DistScale + PedZDist;
						if(TheCamera.PedZoomIndicator == 3.0f)
							Offset = 0.25*DistScale + PedZDist;
						if(Abs(CGeneral::GetRadianAngleBetweenPoints(CamToPed.x, CamToPed.y, CamToTarget.x, CamToTarget.y)) > HALFPI)
							Offset += 0.3f;
						m_fPedBetweenCameraHeightOffset = Offset + 1.3f;
						PedZDist = 0.0f;
					}else if(Mode == MODE_FIGHT_CAM)
						m_fPedBetweenCameraHeightOffset = PedZDist + 1.3f + 0.5f;
				}else
					m_fPedBetweenCameraHeightOffset = 0.0f;
			}else{
				PedZDist = 0.0f;
				m_fPedBetweenCameraHeightOffset = 0.0f;
			}
		}else
			PedZDist = 0.0f;


		// Move camera up for vehicles in the way
		if(m_bCollisionChecksOn && (Mode == MODE_FOLLOWPED || Mode == MODE_FIGHT_CAM)){
			bool FoundCar = false;
			CEntity *vehicle = nil;
			float TestDist = DistOnGround + 1.25f;
			float HighestCar = 0.0f;
			CVector TestBase = CamTargetEntity->GetPosition();
			CVector TestPoint;
			TestBase.z -= 0.15f;

			TestPoint = TestBase - TestDist * CVector(Cos(BetaAngle), Sin(BetaAngle), 0.0f);
			if(CWorld::ProcessLineOfSight(CamTargetEntity->GetPosition(), TestPoint, colPoint, vehicle, false, true, false, false, false, false) &&
			   vehicle->IsVehicle()){
				float height = vehicle->GetColModel()->boundingBox.GetSize().z;
				FoundCar = true;
				HighestCar = height;
				if(((CVehicle*)vehicle)->IsTrain())
					m_bTheHeightFixerVehicleIsATrain = true;
			}

			TestPoint = TestBase - TestDist * CVector(Cos(BetaAngle+DEGTORAD(28.0f)), Sin(BetaAngle+DEGTORAD(28.0f)), 0.0f);
			if(CWorld::ProcessLineOfSight(CamTargetEntity->GetPosition(), TestPoint, colPoint, vehicle, false, true, false, false, false, false) &&
			   vehicle->IsVehicle()){
				float height = vehicle->GetColModel()->boundingBox.GetSize().z;
				if(FoundCar){
					HighestCar = max(HighestCar, height);
				}else{
					FoundCar = true;
					HighestCar = height;
				}
				if(((CVehicle*)vehicle)->IsTrain())
					m_bTheHeightFixerVehicleIsATrain = true;
			}

			TestPoint = TestBase - TestDist * CVector(Cos(BetaAngle-DEGTORAD(28.0f)), Sin(BetaAngle-DEGTORAD(28.0f)), 0.0f);
			if(CWorld::ProcessLineOfSight(CamTargetEntity->GetPosition(), TestPoint, colPoint, vehicle, false, true, false, false, false, false) &&
			   vehicle->IsVehicle()){
				float height = vehicle->GetColModel()->boundingBox.GetSize().z;
				if(FoundCar){
					HighestCar = max(HighestCar, height);
				}else{
					FoundCar = true;
					HighestCar = height;
				}
				if(((CVehicle*)vehicle)->IsTrain())
					m_bTheHeightFixerVehicleIsATrain = true;
			}

			if(FoundCar){
				m_fDimensionOfHighestNearCar = HighestCar + 0.1f;
				if(Mode == MODE_FIGHT_CAM)
					m_fDimensionOfHighestNearCar += 0.75f;
			}else
				m_fDimensionOfHighestNearCar = 0.0f;
		}

		// Move up for road
		if(Mode == MODE_FOLLOWPED || Mode == MODE_FIGHT_CAM ||
		   Mode == MODE_SYPHON || Mode == MODE_SYPHON_CRIM_IN_FRONT || Mode == MODE_SPECIAL_FIXED_FOR_SYPHON){
			bool Inside = false;
			bool OnRoad = false;

			switch(((CPhysical*)CamTargetEntity)->m_nSurfaceTouched)
			case SURFACE_GRASS:
			case SURFACE_DIRT:
			case SURFACE_DIRTTRACK:
			case SURFACE_STEEL:
			case SURFACE_TIRE:
			case SURFACE_STONE:
				OnRoad = true;

			if(CCullZones::PlayerNoRain())
				Inside = true;

			if((m_bCollisionChecksOn || PreviouslyFailedRoadHeightCheck || OnRoad) &&
			   m_fCloseInPedHeightOffset < 0.0001f && !Inside){
				CVector TestPoint;
				CEntity *road;
				float GroundZ = 0.0f;
				bool FoundGround = false;
				float RoofZ = 0.0f;
				bool FoundRoof = false;
				static float MinHeightAboveRoad = 0.9f;

				TestPoint = CamTargetEntity->GetPosition() - DistOnGround * CVector(Cos(BetaAngle), Sin(BetaAngle), 0.0f);
				m_fRoadOffSet = 0.0f;

				if(CWorld::ProcessVerticalLine(TestPoint, -1000.0f, colPoint, road, true, false, false, false, false, false, nil)){
					FoundGround = true;
					GroundZ = colPoint.point.z;
				}
				// Move up if too close to ground
				if(FoundGround){
					if(TestPoint.z - GroundZ < MinHeightAboveRoad){
						m_fRoadOffSet = GroundZ + MinHeightAboveRoad - TestPoint.z;
						PreviouslyFailedRoadHeightCheck = true;
					}else{
						if(m_bCollisionChecksOn)
							PreviouslyFailedRoadHeightCheck = false;
						else
							m_fRoadOffSet = 0.0f;
					}
				}else{
					if(CWorld::ProcessVerticalLine(TestPoint, 1000.0f, colPoint, road, true, false, false, false, false, false, nil)){
						FoundRoof = true;
						RoofZ = colPoint.point.z;
					}
					if(FoundRoof){
						if(TestPoint.z - RoofZ < MinHeightAboveRoad){
							m_fRoadOffSet = RoofZ + MinHeightAboveRoad - TestPoint.z;
							PreviouslyFailedRoadHeightCheck = true;
						}else{
							if(m_bCollisionChecksOn)
								PreviouslyFailedRoadHeightCheck = false;
							else
								m_fRoadOffSet = 0.0f;
						}
					}
				}
			}
		}

		if(PreviouslyFailedRoadHeightCheck && m_fCloseInPedHeightOffset < 0.0001f){
			if(colPoint.surfaceB != SURFACE_TARMAC &&
			   colPoint.surfaceB != SURFACE_GRASS &&
			   colPoint.surfaceB != SURFACE_DIRT &&
			   colPoint.surfaceB != SURFACE_DIRTTRACK &&
			   colPoint.surfaceB != SURFACE_STONE){
				if(m_fRoadOffSet > 1.4f)
					m_fRoadOffSet = 1.4f;
			}else{
				if(Mode == MODE_FOLLOWPED){
					if(TheCamera.PedZoomIndicator == 1.0f)
						m_fRoadOffSet += 0.2f;
					if(TheCamera.PedZoomIndicator == 2.0f)
						m_fRoadOffSet += 0.5f;
					if(TheCamera.PedZoomIndicator == 3.0f)
						m_fRoadOffSet += 0.95f;
				}
			}
		}
	}

	if(StandingOnBoat){
		m_fRoadOffSet = 0.0f;
		m_fDimensionOfHighestNearCar = 1.0f;
		m_fPedBetweenCameraHeightOffset = 0.0f;
	}
}

void
CCam::GetVectorsReadyForRW(void)
{
	CVector right;
	Up = CVector(0.0f, 0.0f, 1.0f);
	Front.Normalise();
	if(Front.x == 0.0f && Front.y == 0.0f){
		Front.x = 0.0001f;
		Front.y = 0.0001f;
	}
	right = CrossProduct(Front, Up);
	right.Normalise();
	Up = CrossProduct(right, Front);
}

void
CCam::LookBehind(void)
{
	float Dist, DeltaBeta, TargetOrientation, Angle;
	CVector TargetCoors, TargetFwd, TestCoors;
	CColPoint colPoint;
	CEntity *entity;

	TargetCoors = CamTargetEntity->GetPosition();
	Front = CamTargetEntity->GetPosition() - Source;

	if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_BEHINDBOAT) && CamTargetEntity->IsVehicle()){
		LookingBehind = true;
		Dist = Mode == MODE_CAM_ON_A_STRING ? CA_MAX_DISTANCE : 15.5f;
		TargetFwd = CamTargetEntity->GetForward();
		TargetFwd.Normalise();
		TargetOrientation = CGeneral::GetATanOfXY(TargetFwd.x, TargetFwd.y);
		DeltaBeta = TargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;
		if(DirectionWasLooking == LOOKING_BEHIND)
			LookBehindCamWasInFront = DeltaBeta <= -HALFPI || DeltaBeta >= HALFPI;
		if(LookBehindCamWasInFront)
			TargetOrientation += PI;
		Source.x = Dist*Cos(TargetOrientation) + TargetCoors.x;
		Source.y = Dist*Sin(TargetOrientation) + TargetCoors.y;
		Source.z -= 1.0f;
		if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, false, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.9f);
			Source = colPoint.point;
		}
		Source.z += 1.0f;
		Front = CamTargetEntity->GetPosition() - Source;
		GetVectorsReadyForRW();
	}
	if(Mode == MODE_1STPERSON && CamTargetEntity->IsVehicle()){
		LookingBehind = true;
		RwCameraSetNearClipPlane(Scene.camera, 0.25f);
		Front = CamTargetEntity->GetForward();
		Front.Normalise();
		if(((CVehicle*)CamTargetEntity)->IsBoat())
			Source.z -= 0.5f;
		Source += 0.25f*Front;
		Front = -Front;
#ifdef FIX_BUGS
		// not sure if this is a bug...
		GetVectorsReadyForRW();
#endif
	}
	if(CamTargetEntity->IsPed()){
		Angle = CGeneral::GetATanOfXY(Source.x - TargetCoors.x, Source.y - TargetCoors.y) + PI;
		Source.x = 4.5f*Cos(Angle) + TargetCoors.x;
		Source.y = 4.5f*Sin(Angle) + TargetCoors.y;
		Source.z = 1.15f + TargetCoors.z;
		TestCoors = TargetCoors;
		TestCoors.z = Source.z;
		if(CWorld::ProcessLineOfSight(TestCoors, Source, colPoint, entity, true, true, false, true, false, true, true)){
			Source.x = colPoint.point.x;
			Source.y = colPoint.point.y;
			if((TargetCoors - Source).Magnitude2D() < 1.15f)
				RwCameraSetNearClipPlane(Scene.camera, 0.05f);
		}
		Front = TargetCoors - Source;
		GetVectorsReadyForRW();
	}
}

void
CCam::LookLeft(void)
{
	float Dist, TargetOrientation;
	CVector TargetCoors, TargetFwd;
	CColPoint colPoint;
	CEntity *entity;

	if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_BEHINDBOAT) && CamTargetEntity->IsVehicle()){
		LookingLeft = true;
		TargetCoors = CamTargetEntity->GetPosition();
		Front = CamTargetEntity->GetPosition() - Source;
		Dist = Mode == MODE_CAM_ON_A_STRING ? CA_MAX_DISTANCE : 9.0f;
		TargetFwd = CamTargetEntity->GetForward();
		TargetFwd.Normalise();
		TargetOrientation = CGeneral::GetATanOfXY(TargetFwd.x, TargetFwd.y);
		Source.x = Dist*Cos(TargetOrientation - HALFPI) + TargetCoors.x;
		Source.y = Dist*Sin(TargetOrientation - HALFPI) + TargetCoors.y;
		Source.z -= 1.0f;
		if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, false, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
			Source = colPoint.point;
		}
		Source.z += 1.0f;
		Front = CamTargetEntity->GetPosition() - Source;
		Front.z += 1.1f;
		if(Mode == MODE_BEHINDBOAT)
			Front.z += 1.2f;
		GetVectorsReadyForRW();
	}
	if(Mode == MODE_1STPERSON && CamTargetEntity->IsVehicle()){
		LookingLeft = true;
		RwCameraSetNearClipPlane(Scene.camera, 0.25f);
		if(((CVehicle*)CamTargetEntity)->IsBoat())
			Source.z -= 0.5f;

		Up = CamTargetEntity->GetUp();
		Up.Normalise();
		Front = CamTargetEntity->GetForward();
		Front.Normalise();
		Front = -CrossProduct(Front, Up);
		Front.Normalise();
#ifdef FIX_BUGS
		// not sure if this is a bug...
		GetVectorsReadyForRW();
#endif
	}
}

void
CCam::LookRight(void)
{
	float Dist, TargetOrientation;
	CVector TargetCoors, TargetFwd;
	CColPoint colPoint;
	CEntity *entity;

	if((Mode == MODE_CAM_ON_A_STRING || Mode == MODE_BEHINDBOAT) && CamTargetEntity->IsVehicle()){
		LookingRight = true;
		TargetCoors = CamTargetEntity->GetPosition();
		Front = CamTargetEntity->GetPosition() - Source;
		Dist = Mode == MODE_CAM_ON_A_STRING ? CA_MAX_DISTANCE : 9.0f;
		TargetFwd = CamTargetEntity->GetForward();
		TargetFwd.Normalise();
		TargetOrientation = CGeneral::GetATanOfXY(TargetFwd.x, TargetFwd.y);
		Source.x = Dist*Cos(TargetOrientation + HALFPI) + TargetCoors.x;
		Source.y = Dist*Sin(TargetOrientation + HALFPI) + TargetCoors.y;
		Source.z -= 1.0f;
		if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, false, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
			Source = colPoint.point;
		}
		Source.z += 1.0f;
		Front = CamTargetEntity->GetPosition() - Source;
		Front.z += 1.1f;
		if(Mode == MODE_BEHINDBOAT)
			Front.z += 1.2f;
		GetVectorsReadyForRW();
	}
	if(Mode == MODE_1STPERSON && CamTargetEntity->IsVehicle()){
		LookingRight = true;
		RwCameraSetNearClipPlane(Scene.camera, 0.25f);
		if(((CVehicle*)CamTargetEntity)->IsBoat())
			Source.z -= 0.5f;

		Up = CamTargetEntity->GetUp();
		Up.Normalise();
		Front = CamTargetEntity->GetForward();
		Front.Normalise();
		Front = CrossProduct(Front, Up);
		Front.Normalise();
#ifdef FIX_BUGS
		// not sure if this is a bug...
		GetVectorsReadyForRW();
#endif
	}
}

void
CCam::ClipIfPedInFrontOfPlayer(void)
{
	float FwdAngle, PedAngle, DeltaAngle, fDist, Near;
	CVector vDist;
	CPed *Player;
	bool found = false;
	int ped = 0;

	// unused: TheCamera.pTargetEntity->GetPosition() - TheCamera.GetGameCamPosition();

	FwdAngle = CGeneral::GetATanOfXY(TheCamera.GetForward().x, TheCamera.GetForward().y);
	Player = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
	while(ped < Player->m_numNearPeds && !found)
		if(Player->m_nearPeds[ped] && Player->m_nearPeds[ped]->GetPedState() != PED_DEAD)
			found = true;
		else
			ped++;
	if(found){
		vDist = Player->m_nearPeds[ped]->GetPosition() - TheCamera.GetGameCamPosition();
		PedAngle = CGeneral::GetATanOfXY(vDist.x, vDist.y);
		DeltaAngle = FwdAngle - PedAngle;
		while(DeltaAngle >= PI) DeltaAngle -= 2*PI;
		while(DeltaAngle < -PI) DeltaAngle += 2*PI;
		if(Abs(DeltaAngle) < HALFPI){
			fDist = Sqrt(SQR(vDist.x) + SQR(vDist.y));
			if(fDist < 1.25f){
				Near = 0.9f - (1.25f - fDist);
				if(Near < 0.05f)
					Near = 0.05f;
				RwCameraSetNearClipPlane(Scene.camera, Near);
			}
		}
	}
}

void
CCam::KeepTrackOfTheSpeed(const CVector &source, const CVector &target, const CVector &up, const float &alpha, const float &beta, const float &fov)
{
	static CVector PreviousSource = source;
	static CVector PreviousTarget = target;
	static CVector PreviousUp = up;
	static float PreviousBeta = beta;
	static float PreviousAlpha = alpha;
	static float PreviousFov = fov;

	if(TheCamera.m_bJust_Switched){
		PreviousSource = source;
		PreviousTarget = target;
		PreviousUp = up;
	}

	m_cvecSourceSpeedOverOneFrame = PreviousSource - source;
	m_cvecTargetSpeedOverOneFrame = PreviousTarget - target;
	m_cvecUpOverOneFrame = PreviousUp - up;
	m_fFovSpeedOverOneFrame = fov - PreviousFov;
	m_fBetaSpeedOverOneFrame = beta - PreviousBeta;
	MakeAngleLessThan180(m_fBetaSpeedOverOneFrame);
	m_fAlphaSpeedOverOneFrame = alpha - PreviousAlpha;
	MakeAngleLessThan180(m_fAlphaSpeedOverOneFrame);

	PreviousSource = source;
	PreviousTarget = target;
	PreviousUp = up;
	PreviousBeta = beta;
	PreviousAlpha = alpha;
	PreviousFov = fov;
}

bool
CCam::Using3rdPersonMouseCam(void) 
{
	return CCamera::m_bUseMouse3rdPerson &&
		(Mode == MODE_FOLLOWPED ||
			TheCamera.m_bPlayerIsInGarage &&
			FindPlayerPed() && FindPlayerPed()->m_nPedState != PED_DRIVING &&
			Mode != MODE_TOPDOWN && this->CamTargetEntity == FindPlayerPed());
}

bool
CCam::GetWeaponFirstPersonOn(void)
{
	CEntity *target = this->CamTargetEntity;
	if (target && target->IsPed())
		return ((CPed*)target)->GetWeapon()->m_bAddRotOffset;
	
	return false;
}

bool
CCam::IsTargetInWater(const CVector &CamCoors)
{
	if(CamTargetEntity == nil)
		return false;
	if(CamTargetEntity->IsPed()){
		if(!((CPed*)CamTargetEntity)->bIsInWater)
			return false;
		if(!((CPed*)CamTargetEntity)->bIsStanding)
			return true;
		return false;
	}
	return ((CPhysical*)CamTargetEntity)->bIsInWater;
}

void
CCam::PrintMode(void)
{
	// Doesn't do anything
	char buf[256];

	if(PrintDebugCode){
		sprintf(buf, "                                                   ");
		sprintf(buf, "                                                   ");
		sprintf(buf, "                                                   ");

		static char *modes[] = { "None",
			"Top Down", "GTA Classic", "Behind Car", "Follow Ped",
			"Aiming", "Debug", "Sniper", "Rocket", "Model Viewer", "Bill",
			"Syphon", "Circle", "Cheesy Zoom", "Wheel", "Fixed",
			"1st Person", "Fly by", "on a String", "Reaction",
			"Follow Ped with Bind", "Chris", "Behind Boat",
			"Player fallen in Water", "Train Roof", "Train Side",
			"Blood on the tracks", "Passenger", "Syphon Crim in Front",
			"Dead Baby", "Pillow Paps", "Look at Cars", "Arrest One",
			"Arrest Two", "M16", "Special fixed for Syphon", "Fight",
			"Top Down Ped",
			"Sniper run about", "Rocket run about",
			"1st Person run about", "M16 run about", "Fight run about",
			"Editor"
		};
		sprintf(buf, "Cam: %s", modes[TheCamera.Cams[TheCamera.ActiveCam].Mode]);
		CDebug::PrintAt(buf, 2, 5);
	}

	if(DebugCamMode != MODE_NONE){
		switch(Mode){
		case MODE_FOLLOWPED:
			sprintf(buf, "Debug:- Cam Choice1. No Locking, used as game default");
			break;
		case MODE_REACTION:
			sprintf(buf, "Debug:- Cam Choice2. Reaction Cam On A String ");
			sprintf(buf, "        Uses Locking Button LeftShoulder 1. ");
			break;
		case MODE_FOLLOW_PED_WITH_BIND:
			sprintf(buf, "Debug:- Cam Choice3. Game ReactionCam with Locking ");
			sprintf(buf, "        Uses Locking Button LeftShoulder 1. ");
			break;
		case MODE_CHRIS:
			sprintf(buf, "Debug:- Cam Choice4. Chris's idea.  ");
			sprintf(buf, "        Uses Locking Button LeftShoulder 1. ");
			sprintf(buf, "        Also control the camera using the right analogue stick.");
			break;
		}
	}
}

// This code is really bad. wtf R*?
CVector
CCam::DoAverageOnVector(const CVector &vec)
{
	int i;
	CVector Average = { 0.0f, 0.0f, 0.0f };

	if(ResetStatics){
		m_iRunningVectorArrayPos = 0;
		m_iRunningVectorCounter = 1;
	}

	// TODO: make this work with NUMBER_OF_VECTORS_FOR_AVERAGE != 2
	if(m_iRunningVectorCounter == 3){
		m_arrPreviousVectors[0] = m_arrPreviousVectors[1];
		m_arrPreviousVectors[1] = vec;
	}else
		m_arrPreviousVectors[m_iRunningVectorArrayPos] = vec;

	for(i = 0; i <= m_iRunningVectorArrayPos; i++)
		Average += m_arrPreviousVectors[i];
	Average /= i;

	m_iRunningVectorArrayPos++;
	m_iRunningVectorCounter++;
	if(m_iRunningVectorArrayPos >= NUMBER_OF_VECTORS_FOR_AVERAGE)
		m_iRunningVectorArrayPos = NUMBER_OF_VECTORS_FOR_AVERAGE-1;
	if(m_iRunningVectorCounter > NUMBER_OF_VECTORS_FOR_AVERAGE+1)
		m_iRunningVectorCounter = NUMBER_OF_VECTORS_FOR_AVERAGE+1;

	return Average;
}

// Rotate Beta in direction opposite of BetaOffset in 5 deg. steps.
// Return the first angle for which Beta + BetaOffset + Angle has a clear view.
// i.e. BetaOffset is a safe zone so that Beta + Angle is really clear.
// If BetaOffset == 0, try both directions.
float
CCam::GetPedBetaAngleForClearView(const CVector &Target, float Dist, float BetaOffset, bool checkBuildings, bool checkVehicles, bool checkPeds, bool checkObjects, bool checkDummies)
{
	CColPoint point;
	CEntity *ent = nil;
	CVector ToSource;
	float a;

	// This would be so much nicer if we just got the step variable before the loop...R*

	for(a = 0.0f; a <= PI; a += DEGTORAD(5.0f)){
		if(BetaOffset <= 0.0f){
			ToSource = CVector(Cos(Beta + BetaOffset + a), Sin(Beta + BetaOffset + a), 0.0f)*Dist;
			if(!CWorld::ProcessLineOfSight(Target, Target + ToSource,
						point, ent, checkBuildings, checkVehicles, checkPeds,
						checkObjects, checkDummies, true, true))
				return a;
		}
		if(BetaOffset >= 0.0f){
			ToSource = CVector(Cos(Beta + BetaOffset - a), Sin(Beta + BetaOffset - a), 0.0f)*Dist;
			if(!CWorld::ProcessLineOfSight(Target, Target + ToSource,
						point, ent, checkBuildings, checkVehicles, checkPeds,
						checkObjects, checkDummies, true, true))
				return -a;
		}
	}
	return 0.0f;
}

static float DefaultAcceleration = 0.045f;
static float DefaultMaxStep = 0.15f;

void
CCam::Process_FollowPed(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	const float GroundDist = 1.85f;

	CVector TargetCoors, Dist, IdealSource;
	float Length = 0.0f;
	float LateralLeft = 0.0f;
	float LateralRight = 0.0f;
	float Center = 0.0f;
	static bool PreviouslyObscured;
	static bool PickedASide;
	static float FixedTargetOrientation = 0.0f;
	float AngleToGoTo = 0.0f;
	float BetaOffsetAvoidBuildings = 0.45f;		// ~25 deg
	float BetaOffsetGoingBehind = 0.45f;
	bool GoingBehind = false;
	bool Obscured = false;
	bool BuildingCheckObscured = false;
	bool HackPlayerOnStoppingTrain = false;
	static int TimeIndicatedWantedToGoDown = 0;
	static bool StartedCountingForGoDown = false;
	float DeltaBeta;

	m_bFixingBeta = false;
	bBelowMinDist = false;
	bBehindPlayerDesired = false;

#ifdef FIX_BUGS
	if(!CamTargetEntity->IsPed())
		return;
#endif
	assert(CamTargetEntity->IsPed());

	// CenterDist should be > LateralDist because we don't have an angle for safety in this case
	float CenterDist, LateralDist;
	float AngleToGoToSpeed;
	if(m_fCloseInPedHeightOffset > 0.00001f){
		LateralDist = 0.55f;
		CenterDist = 1.25f;
		BetaOffsetAvoidBuildings = 0.9f;	// ~50 deg
		BetaOffsetGoingBehind = 0.9f;
		AngleToGoToSpeed = 0.88254666f;
	}else{
		LateralDist = 0.8f;
		CenterDist = 1.35f;
		if(TheCamera.PedZoomIndicator == 1.0f || TheCamera.PedZoomIndicator == 4.0f){
			LateralDist = 1.25f;
			CenterDist = 1.6f;
		}
		AngleToGoToSpeed = 0.43254671f;
	}

	FOV = DefaultFOV;

	if(ResetStatics){
		Rotating = false;
		m_bCollisionChecksOn = true;
		FixedTargetOrientation = 0.0f;
		PreviouslyObscured = false;
		PickedASide = false;
		StartedCountingForGoDown = false;
		AngleToGoTo = 0.0f;
		// unused LastAngleWithNoPickedASide
	}


	TargetCoors = CameraTarget;
	IdealSource = Source;
	TargetCoors.z += m_fSyphonModeTargetZOffSet;

	TargetCoors = DoAverageOnVector(TargetCoors);
	TargetCoors.z += m_fRoadOffSet;

	Dist.x = IdealSource.x - TargetCoors.x;
	Dist.y = IdealSource.y - TargetCoors.y;
	Length = Dist.Magnitude2D();

	// Cam on a string. With a fixed distance. Zoom in/out is done later.
	if(Length != 0.0f)
		IdealSource = TargetCoors + CVector(Dist.x, Dist.y, 0.0f)/Length * GroundDist;
	else
		IdealSource = TargetCoors + CVector(1.0f, 1.0f, 0.0f);

	// TODO: what's transition beta?
	if(TheCamera.m_bUseTransitionBeta && ResetStatics){
		CVector VecDistance;
		IdealSource.x = TargetCoors.x + GroundDist*Cos(m_fTransitionBeta);
		IdealSource.y = TargetCoors.y + GroundDist*Sin(m_fTransitionBeta);
		Beta = CGeneral::GetATanOfXY(IdealSource.x - TargetCoors.x, IdealSource.y - TargetCoors.y);
	}else
		Beta = CGeneral::GetATanOfXY(Source.x - TargetCoors.x, Source.y - TargetCoors.y);

	if(TheCamera.m_bCamDirectlyBehind){
		 m_bCollisionChecksOn = true;
		 Beta = TargetOrientation + PI;
	}

	if(FindPlayerVehicle())
		if(FindPlayerVehicle()->m_vehType == VEHICLE_TYPE_TRAIN)
			HackPlayerOnStoppingTrain = true;

	if(TheCamera.m_bCamDirectlyInFront){
		 m_bCollisionChecksOn = true;
		 Beta = TargetOrientation;
	}

	while(Beta >= PI) Beta -= 2.0f * PI;
	while(Beta < -PI) Beta += 2.0f * PI;

	// BUG? is this ever used?
	// The values seem to be roughly m_fPedZoomValueSmooth + 1.85
	if(ResetStatics){
		if(TheCamera.PedZoomIndicator == 1.0) m_fRealGroundDist = 2.090556f;
		if(TheCamera.PedZoomIndicator == 2.0) m_fRealGroundDist = 3.34973f;
		if(TheCamera.PedZoomIndicator == 3.0) m_fRealGroundDist = 4.704914f;
		if(TheCamera.PedZoomIndicator == 4.0) m_fRealGroundDist = 2.090556f;
	}
	// And what is this? It's only used for collision and rotation it seems
	float RealGroundDist;
	if(TheCamera.PedZoomIndicator == 1.0) RealGroundDist = 2.090556f;
	if(TheCamera.PedZoomIndicator == 2.0) RealGroundDist = 3.34973f;
	if(TheCamera.PedZoomIndicator == 3.0) RealGroundDist = 4.704914f;
	if(TheCamera.PedZoomIndicator == 4.0) RealGroundDist = 2.090556f;
	if(m_fCloseInPedHeightOffset >  0.00001f)
		RealGroundDist = 1.7016f;


	bool Shooting = false;
	CPed *ped = (CPed*)CamTargetEntity;
	if(ped->GetWeapon()->m_eWeaponType != WEAPONTYPE_UNARMED)
		if(CPad::GetPad(0)->GetWeapon())
			Shooting = true;
	if(ped->GetWeapon()->m_eWeaponType == WEAPONTYPE_DETONATOR ||
	   ped->GetWeapon()->m_eWeaponType == WEAPONTYPE_BASEBALLBAT)
		Shooting = false;


	if(m_fCloseInPedHeightOffset > 0.00001f)
		TargetCoors.z -= m_fRoadOffSet;

	// Figure out if and where we want to rotate

	if(CPad::GetPad(0)->ForceCameraBehindPlayer() || Shooting){

		// Center cam behind player

		GoingBehind = true;
		m_bCollisionChecksOn = true;
		float OriginalBeta = Beta;
		// Set Beta behind player
		Beta = TargetOrientation + PI;
		TargetCoors.z -= 0.1f;

		AngleToGoTo = GetPedBetaAngleForClearView(TargetCoors, CenterDist * RealGroundDist, 0.0f, true, false, false, true, false);
		if(AngleToGoTo != 0.0f){
			if(AngleToGoTo < 0.0f)
				AngleToGoTo -= AngleToGoToSpeed;
			else
				AngleToGoTo += AngleToGoToSpeed;
		}else{
			float LateralLeft = GetPedBetaAngleForClearView(TargetCoors, LateralDist * RealGroundDist, BetaOffsetGoingBehind, true, false, false, true, false);
			float LateralRight = GetPedBetaAngleForClearView(TargetCoors, LateralDist * RealGroundDist, -BetaOffsetGoingBehind, true, false, false, true, false);
			if(LateralLeft == 0.0f && LateralRight != 0.0f)
				AngleToGoTo += LateralRight;
			else if(LateralLeft != 0.0f && LateralRight == 0.0f)
				AngleToGoTo += LateralLeft;
		}

		TargetCoors.z += 0.1f;
		Beta = OriginalBeta;

		if(PickedASide){
			if(AngleToGoTo == 0.0f)
				FixedTargetOrientation = TargetOrientation + PI;
			Rotating = true;
		}else{
			FixedTargetOrientation = TargetOrientation + PI + AngleToGoTo;
			Rotating = true;
			PickedASide = true;
		}
	}else{

		// Rotate cam to avoid clipping into buildings

		TargetCoors.z -= 0.1f;

		Center = GetPedBetaAngleForClearView(TargetCoors, CenterDist * RealGroundDist, 0.0f, true, false, false, true, false);
		if(m_bCollisionChecksOn || PreviouslyObscured || Center != 0.0f || m_fCloseInPedHeightOffset > 0.00001f){
			if(Center != 0.0f){
				AngleToGoTo = Center;
			}else{
				LateralLeft = GetPedBetaAngleForClearView(TargetCoors, LateralDist * RealGroundDist, BetaOffsetAvoidBuildings, true, false, false, true, false);
				LateralRight = GetPedBetaAngleForClearView(TargetCoors, LateralDist * RealGroundDist, -BetaOffsetAvoidBuildings, true, false, false, true, false);
				if(LateralLeft == 0.0f && LateralRight != 0.0f){
					AngleToGoTo += LateralRight;
					if(m_fCloseInPedHeightOffset > 0.0f)
						RwCameraSetNearClipPlane(Scene.camera, 0.7f);
				}else if(LateralLeft != 0.0f && LateralRight == 0.0f){
					AngleToGoTo += LateralLeft;
					if(m_fCloseInPedHeightOffset > 0.0f)
						RwCameraSetNearClipPlane(Scene.camera, 0.7f);
				}
			}
			if(LateralLeft != 0.0f || LateralRight != 0.0f || Center != 0.0f)
				BuildingCheckObscured = true;
		}

		TargetCoors.z += 0.1f;
	}

	if(m_fCloseInPedHeightOffset > 0.00001f)
		TargetCoors.z += m_fRoadOffSet;


	// Have to fix to avoid collision

	if(AngleToGoTo != 0.0f){
		Obscured = true;
		Rotating = true;
		if(CPad::GetPad(0)->ForceCameraBehindPlayer() || Shooting){
			if(!PickedASide)
				FixedTargetOrientation = Beta + AngleToGoTo;	// can this even happen?
		}else
			FixedTargetOrientation = Beta + AngleToGoTo;

		// This calculation is only really used to figure out how fast to rotate out of collision

		m_fAmountFractionObscured = 1.0f;
		CVector PlayerPos = FindPlayerPed()->GetPosition();
		float RotationDist = (AngleToGoTo == Center ? CenterDist : LateralDist) * RealGroundDist;
		// What's going on here? - AngleToGoTo?
		CVector RotatedSource = PlayerPos + CVector(Cos(Beta - AngleToGoTo), Sin(Beta - AngleToGoTo), 0.0f) * RotationDist;

		CColPoint colpoint;
		CEntity *entity;
		if(CWorld::ProcessLineOfSight(PlayerPos, RotatedSource, colpoint, entity, true, false, false, true, false, false, false)){
			if((PlayerPos - RotatedSource).Magnitude() != 0.0f)
				m_fAmountFractionObscured = (PlayerPos - colpoint.point).Magnitude() / (PlayerPos - RotatedSource).Magnitude();
			else
				m_fAmountFractionObscured = 1.0f;
		}
	}
	if(m_fAmountFractionObscured < 0.0f) m_fAmountFractionObscured = 0.0f;
	if(m_fAmountFractionObscured > 1.0f) m_fAmountFractionObscured = 1.0f;



	// Figure out speed values for Beta rotation

	float Acceleration, MaxSpeed;
	static float AccelerationMult = 0.35f;
	static float MaxSpeedMult = 0.85f;
	static float AccelerationMultClose = 0.7f;
	static float MaxSpeedMultClose = 1.6f;
	float BaseAcceleration = 0.025f;
	float BaseMaxSpeed = 0.09f;
	if(m_fCloseInPedHeightOffset > 0.00001f){
		if(AngleToGoTo == 0.0f){
			BaseAcceleration = 0.022f;
			BaseMaxSpeed = 0.04f;
		}else{
			BaseAcceleration = DefaultAcceleration;
			BaseMaxSpeed = DefaultMaxStep;
		}
	}
	if(AngleToGoTo == 0.0f){
		Acceleration = BaseAcceleration;
		MaxSpeed = BaseMaxSpeed;
	}else if(CPad::GetPad(0)->ForceCameraBehindPlayer() && !Shooting){
		Acceleration = 0.051f;
		MaxSpeed = 0.18f;
	}else if(m_fCloseInPedHeightOffset > 0.00001f){
		Acceleration = BaseAcceleration + AccelerationMultClose*sq(m_fAmountFractionObscured - 1.05f);
		MaxSpeed = BaseMaxSpeed + MaxSpeedMultClose*sq(m_fAmountFractionObscured - 1.05f);
	}else{
		Acceleration = DefaultAcceleration + AccelerationMult*sq(m_fAmountFractionObscured - 1.05f);
		MaxSpeed = DefaultMaxStep + MaxSpeedMult*sq(m_fAmountFractionObscured - 1.05f);
	}
	static float AccelerationLimit = 0.3f;
	static float MaxSpeedLimit = 0.65f;
	if(Acceleration > AccelerationLimit) Acceleration = AccelerationLimit;
	if(MaxSpeed > MaxSpeedLimit) MaxSpeed = MaxSpeedLimit;


	int MoveState = ((CPed*)CamTargetEntity)->m_nMoveState;
	if(MoveState != PEDMOVE_NONE && MoveState != PEDMOVE_STILL &&
	   !CPad::GetPad(0)->ForceCameraBehindPlayer() && !Obscured && !Shooting){
		Rotating = false;
		BetaSpeed = 0.0f;
	}

	// Now do the Beta rotation

	float Distance = (IdealSource - TargetCoors).Magnitude2D();
	m_fDistanceBeforeChanges = Distance;

	if(Rotating){
		m_bFixingBeta = true;

		while(FixedTargetOrientation >= PI) FixedTargetOrientation -= 2*PI;
		while(FixedTargetOrientation < -PI) FixedTargetOrientation += 2*PI;

		while(Beta >= PI) Beta -= 2*PI;
		while(Beta < -PI) Beta += 2*PI;


/*
		// This is inlined WellBufferMe
		DeltaBeta = FixedTargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;

		float ReqSpeed = DeltaBeta * MaxSpeed;
		// Add or subtract absolute depending on sign, genius!
		if(ReqSpeed - BetaSpeed > 0.0f)
			BetaSpeed += SpeedStep * Abs(ReqSpeed - BetaSpeed) * CTimer::GetTimeStep();
		else
			BetaSpeed -= SpeedStep * Abs(ReqSpeed - BetaSpeed) * CTimer::GetTimeStep();
		// this would be simpler:
		// BetaSpeed += SpeedStep * (ReqSpeed - BetaSpeed) * CTimer::ms_fTimeStep;

		if(ReqSpeed < 0.0f && BetaSpeed < ReqSpeed)
			BetaSpeed = ReqSpeed;
		else if(ReqSpeed > 0.0f && BetaSpeed > ReqSpeed)
			BetaSpeed = ReqSpeed;

		Beta += BetaSpeed * min(10.0f, CTimer::GetTimeStep());
*/
		WellBufferMe(FixedTargetOrientation, &Beta, &BetaSpeed, MaxSpeed, Acceleration, true);

		if(ResetStatics){
			Beta = FixedTargetOrientation;
			BetaSpeed = 0.0f;
		}

		Source.x = TargetCoors.x + Distance * Cos(Beta);
		Source.y = TargetCoors.y + Distance * Sin(Beta);

		// Check if we can stop rotating
		DeltaBeta = FixedTargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;
		if(Abs(DeltaBeta) < DEGTORAD(1.0f) && !bBehindPlayerDesired){
			// Stop rotation
			PickedASide = false;
			Rotating = false;
			BetaSpeed = 0.0f;
		}
	}


	if(TheCamera.m_bCamDirectlyBehind || TheCamera.m_bCamDirectlyInFront ||
	   HackPlayerOnStoppingTrain || Rotating){
		if(TheCamera.m_bCamDirectlyBehind){
			Beta = TargetOrientation + PI;
			Source.x = TargetCoors.x + Distance * Cos(Beta);
			Source.y = TargetCoors.y + Distance * Sin(Beta);
		}
		if(TheCamera.m_bCamDirectlyInFront){
			Beta = TargetOrientation;
			Source.x = TargetCoors.x + Distance * Cos(Beta);
			Source.y = TargetCoors.y + Distance * Sin(Beta);
		}
		if(HackPlayerOnStoppingTrain){
			Beta = TargetOrientation + PI;
			Source.x = TargetCoors.x + Distance * Cos(Beta);
			Source.y = TargetCoors.y + Distance * Sin(Beta);
			m_fDimensionOfHighestNearCar = 0.0f;
			m_fCamBufferedHeight = 0.0f;
			m_fCamBufferedHeightSpeed = 0.0f;
		}
		// Beta and Source already set in the rotation code
	}else{
		Source = IdealSource;
		BetaSpeed = 0.0f;
	}

	// Subtract m_fRoadOffSet from both?
	TargetCoors.z -= m_fRoadOffSet;
	Source.z = IdealSource.z - m_fRoadOffSet;

	// Apply zoom now
	// m_fPedZoomValueSmooth makes the cam go down the further out it is
	//  0.25 ->  0.20 for nearest dist
	//  1.50 -> -0.05 for mid dist
	//  2.90 -> -0.33 for far dist
	Source.z += (2.5f - TheCamera.m_fPedZoomValueSmooth)*0.2f - 0.25f;
	// Zoom out camera
	Front = TargetCoors - Source;
	Front.Normalise();
	Source -= Front * TheCamera.m_fPedZoomValueSmooth;
	// and then we move up again
	//  -0.375
	//   0.25
	//   0.95
	Source.z += (TheCamera.m_fPedZoomValueSmooth - 1.0f)*0.5f + m_fCloseInPedHeightOffset;


	// Process height offset to avoid peds and cars

	float TargetZOffSet = m_fRoadOffSet + m_fDimensionOfHighestNearCar;
	TargetZOffSet = max(TargetZOffSet, m_fPedBetweenCameraHeightOffset);
	float TargetHeight = CameraTarget.z + TargetZOffSet - Source.z;

	if(TargetHeight > m_fCamBufferedHeight){
		// Have to go up
		if(TargetZOffSet == m_fPedBetweenCameraHeightOffset && TargetZOffSet > m_fCamBufferedHeight)
			WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.04f, false);
		else if(TargetZOffSet == m_fRoadOffSet && TargetZOffSet > m_fCamBufferedHeight){
			// TODO: figure this out
			bool foo = false;
			switch(((CPhysical*)CamTargetEntity)->m_nSurfaceTouched)
			case SURFACE_GRASS:
			case SURFACE_DIRT:
			case SURFACE_PAVEMENT:
			case SURFACE_STEEL:
			case SURFACE_TIRE:
			case SURFACE_STONE:
				foo = true;
			if(foo)
				WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.4f, 0.05f, false);
			else
				WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.025f, false);
		}else
			WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.025f, false);
		StartedCountingForGoDown = false;
	}else{
		// Have to go down
		if(StartedCountingForGoDown){
			if(CTimer::GetTimeInMilliseconds() != TimeIndicatedWantedToGoDown){
				if(TargetHeight > 0.0f)
					WellBufferMe(TargetHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.01f, false);
				else
					WellBufferMe(0.0f, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.2f, 0.01f, false);
			}
		}else{
			StartedCountingForGoDown = true;
			TimeIndicatedWantedToGoDown = CTimer::GetTimeInMilliseconds();
		}
	}

	Source.z += m_fCamBufferedHeight;


	// Clip Source if necessary

	bool ClipSource = m_fCloseInPedHeightOffset > 0.00001f && m_fCamBufferedHeight > 0.001f;
	if(GoingBehind || ResetStatics || ClipSource){
		CColPoint colpoint;
		CEntity *entity;
		if(CWorld::ProcessLineOfSight(TargetCoors, Source, colpoint, entity, true, false, false, true, false, true, true)){
			Source = colpoint.point;
			if((TargetCoors - Source).Magnitude2D() < 1.0f)
				RwCameraSetNearClipPlane(Scene.camera, 0.05f);
		}
	}

	TargetCoors.z += min(1.0f, m_fCamBufferedHeight/2.0f);
	m_cvecTargetCoorsForFudgeInter = TargetCoors;

	Front = TargetCoors - Source;
	m_fRealGroundDist = Front.Magnitude2D();
	m_fMinDistAwayFromCamWhenInterPolating = m_fRealGroundDist;	
	Front.Normalise();
	GetVectorsReadyForRW();
	TheCamera.m_bCamDirectlyBehind = false;
	TheCamera.m_bCamDirectlyInFront = false;
	PreviouslyObscured = BuildingCheckObscured;

	ResetStatics = false;
}

static float fBaseDist = 1.7f;
static float fAngleDist = 2.0f;
static float fFalloff = 3.0f;
static float fStickSens = 0.01f;
static float fTweakFOV = 1.05f;
static float fTranslateCamUp = 0.8f;
static int16 nFadeControlThreshhold = 45;
static float fDefaultAlphaOrient = -0.22f;

void
CCam::Process_FollowPedWithMouse(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsPed())
		return;

	CVector TargetCoors;
	float CamDist;
	CColPoint colPoint;
	CEntity *entity;

	if(ResetStatics){
		Rotating = false;
		m_bCollisionChecksOn = true;
		CPad::GetPad(0)->ClearMouseHistory();
		ResetStatics = false;
	}

	bool OnTrain = FindPlayerVehicle() && FindPlayerVehicle()->IsTrain();

	// Look around
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
	if((MouseX != 0.0f || MouseY != 0.0f) && !CPad::GetPad(0)->ArePlayerControlsDisabled()){
		UseMouse = true;
		LookLeftRight = -2.5f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight();
		LookUpDown = CPad::GetPad(0)->LookAroundUpDown();
	}
	float AlphaOffset, BetaOffset;
	if(UseMouse){
		BetaOffset = LookLeftRight * TheCamera.m_fMouseAccelHorzntl * FOV/80.0f;
		AlphaOffset = LookUpDown * TheCamera.m_fMouseAccelVertical * FOV/80.0f;
	}else{
		BetaOffset = LookLeftRight * fStickSens * (0.5f/7.0f) * FOV/80.0f * CTimer::GetTimeStep();
		AlphaOffset = LookUpDown * fStickSens * (0.3f/7.0f) * FOV/80.0f * CTimer::GetTimeStep();
	}

	if(TheCamera.GetFading() && TheCamera.GetFadingDirection() == FADE_IN && nFadeControlThreshhold < CDraw::FadeValue ||
	   CDraw::FadeValue > 200){
		if(Alpha < fDefaultAlphaOrient-0.05f)
			AlphaOffset = 0.05f;
		else if(Alpha < fDefaultAlphaOrient)
			AlphaOffset = fDefaultAlphaOrient - Alpha;
		else if(Alpha > fDefaultAlphaOrient+0.05f)
			AlphaOffset = -0.05f;
		else if(Alpha > fDefaultAlphaOrient)
			AlphaOffset = fDefaultAlphaOrient - Alpha;
		else
			AlphaOffset = 0.0f;
	}

	Alpha += AlphaOffset;
	Beta += BetaOffset;
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;
	if(Alpha > DEGTORAD(45.0f)) Alpha = DEGTORAD(45.0f);
	if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

	TargetCoors = CameraTarget;
	TargetCoors.z += fTranslateCamUp;
	TargetCoors = DoAverageOnVector(TargetCoors);

	if(Alpha > fBaseDist)	// comparing an angle against a distance?
		CamDist = fBaseDist + Cos(min(Alpha*fFalloff, HALFPI))*fAngleDist;
	else
		CamDist = fBaseDist + Cos(Alpha)*fAngleDist;

	if(TheCamera.m_bUseTransitionBeta)
		Beta = -CGeneral::GetATanOfXY(-Cos(m_fTransitionBeta), -Sin(m_fTransitionBeta));

	if(TheCamera.m_bCamDirectlyBehind)
		Beta = TheCamera.m_PedOrientForBehindOrInFront;
	if(TheCamera.m_bCamDirectlyInFront)
		Beta = TheCamera.m_PedOrientForBehindOrInFront + PI;
	if(OnTrain)
		Beta = TargetOrientation;

	Front.x = Cos(Alpha) * Cos(Beta);
	Front.y = Cos(Alpha) * Sin(Beta);
	Front.z = Sin(Alpha);
	Source = TargetCoors - Front*CamDist;
	m_cvecTargetCoorsForFudgeInter = TargetCoors;

	// Clip Source and fix near clip
	CWorld::pIgnoreEntity = CamTargetEntity;
	entity = nil;
	if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, true, true, true, false, false, true)){
		float PedColDist = (TargetCoors - colPoint.point).Magnitude();
		float ColCamDist = CamDist - PedColDist;
		if(entity->IsPed() && ColCamDist > 1.0f){
			// Ped in the way but not clipping through
			if(CWorld::ProcessLineOfSight(colPoint.point, Source, colPoint, entity, true, true, true, true, false, false, true)){
				PedColDist = (TargetCoors - colPoint.point).Magnitude();
				Source = colPoint.point;
				if(PedColDist < 0.9f + 0.3f)
					RwCameraSetNearClipPlane(Scene.camera, max(PedColDist-0.3f, 0.05f));
			}else{
				RwCameraSetNearClipPlane(Scene.camera, min(ColCamDist-0.35f, 0.9f));
			}
		}else{
			Source = colPoint.point;
			if(PedColDist < 0.9f + 0.3f)
				RwCameraSetNearClipPlane(Scene.camera, max(PedColDist-0.3f, 0.05f));
		}
	}
	CWorld::pIgnoreEntity = nil;

	float ViewPlaneHeight = Tan(DEGTORAD(FOV) / 2.0f);
	float ViewPlaneWidth = ViewPlaneHeight * CDraw::FindAspectRatio() * fTweakFOV;
	float Near = RwCameraGetNearClipPlane(Scene.camera);
	float radius = ViewPlaneWidth*Near;
	entity = CWorld::TestSphereAgainstWorld(Source + Front*Near, radius, nil, true, true, false, true, false, false);
	int i = 0;
	while(entity){
		CVector CamToCol = gaTempSphereColPoints[0].point - Source;
		float frontDist = DotProduct(CamToCol, Front);
		float dist = (CamToCol - Front*frontDist).Magnitude() / ViewPlaneWidth;

		// Try to decrease near clip
		dist = max(min(Near, dist), 0.1f);
		if(dist < Near)
			RwCameraSetNearClipPlane(Scene.camera, dist);

		// Move forward a bit
		if(dist == 0.1f)
			Source += (TargetCoors - Source)*0.3f;

#ifndef FIX_BUGS
		// this is totally wrong...
		radius = Tan(FOV / 2.0f) * Near;
#endif
		// Keep testing
		entity = CWorld::TestSphereAgainstWorld(Source + Front*Near, radius, nil, true, true, false, true, false, false);

		i++;
		if(i > 5)
			entity = nil;
	}

	if(CamTargetEntity->GetClump()){
		// what's going on here?
		if(RpAnimBlendClumpGetAssociation(CamTargetEntity->GetClump(), ANIM_WEAPON_PUMP) ||
		   RpAnimBlendClumpGetAssociation(CamTargetEntity->GetClump(), ANIM_WEAPON_THROW) ||
		   RpAnimBlendClumpGetAssociation(CamTargetEntity->GetClump(), ANIM_WEAPON_THROWU) ||
		   RpAnimBlendClumpGetAssociation(CamTargetEntity->GetClump(), ANIM_WEAPON_START_THROW)){
			CPed *player = FindPlayerPed();
			float PlayerDist = (Source - player->GetPosition()).Magnitude();
			if(PlayerDist < 2.75f)
				Near = PlayerDist/2.75f * 0.9f - 0.3f;
			RwCameraSetNearClipPlane(Scene.camera, max(Near, 0.1f));
		}
	}

	TheCamera.m_bCamDirectlyInFront = false;
	TheCamera.m_bCamDirectlyBehind = false;

	GetVectorsReadyForRW();

	if(((CPed*)CamTargetEntity)->CanStrafeOrMouseControl() && CDraw::FadeValue < 250 &&
	   (TheCamera.GetFadingDirection() != FADE_OUT || CDraw::FadeValue <= 100)){
		float Heading = Front.Heading();
		((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Heading;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Heading;
		TheCamera.pTargetEntity->SetHeading(Heading);
		TheCamera.pTargetEntity->GetMatrix().UpdateRW();
	}
}

void
CCam::Process_BehindCar(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsVehicle())
		return;

	CVector TargetCoors = CameraTarget;
	TargetCoors.z -= 0.2f;
	CA_MAX_DISTANCE = 9.95f;
	CA_MIN_DISTANCE = 8.5f;

	CVector Dist = Source - TargetCoors;
	float Length = Dist.Magnitude2D();
	m_fDistanceBeforeChanges = Length;
	if(Length < 0.002f)
		Length = 0.002f;
	Beta = CGeneral::GetATanOfXY(TargetCoors.x - Source.x, TargetCoors.y - Source.y);
	if(Length > CA_MAX_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MAX_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MAX_DISTANCE;
	}else if(Length < CA_MIN_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MIN_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MIN_DISTANCE;
	}
	TargetCoors.z += 0.8f;

	WorkOutCamHeightWeeCar(TargetCoors, TargetOrientation);
	RotCamIfInFrontCar(TargetCoors, TargetOrientation);
	FixCamIfObscured(TargetCoors, 1.2f, TargetOrientation);

	Front = TargetCoors - Source;
	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	ResetStatics = false;
	GetVectorsReadyForRW();
}

void
CCam::WorkOutCamHeightWeeCar(CVector &TargetCoors, float TargetOrientation)
{
	CColPoint colpoint;
	CEntity *ent;
	float TargetZOffSet = 0.0f;
	static bool PreviouslyFailedRoadHeightCheck = false;
	static float RoadHeightFix = 0.0f;
	static float RoadHeightFixSpeed = 0.0f;

	if(ResetStatics){
		RoadHeightFix = 0.0f;
		RoadHeightFixSpeed = 0.0f;
		Alpha = DEGTORAD(25.0f);
		AlphaSpeed = 0.0f;
	}
	float AlphaTarget = DEGTORAD(25.0f);
	if(CCullZones::CamNoRain() || CCullZones::PlayerNoRain())
		AlphaTarget = DEGTORAD(14.0f);
	WellBufferMe(AlphaTarget, &Alpha, &AlphaSpeed, 0.1f, 0.05f, true);
	Source.z = TargetCoors.z + CA_MAX_DISTANCE*Sin(Alpha);

	if(FindPlayerVehicle()){
		m_fRoadOffSet = 0.0f;
		bool FoundRoad = false;
		bool FoundRoof = false;
		float RoadZ = 0.0f;
		float RoofZ = 0.0f;

		if(CWorld::ProcessVerticalLine(Source, -1000.0f, colpoint, ent, true, false, false, false, false, false, nil) &&
		   ent->IsBuilding()){
			FoundRoad = true;
			RoadZ = colpoint.point.z;
		}

		if(FoundRoad){
			if(Source.z - RoadZ < 0.9f){
				PreviouslyFailedRoadHeightCheck = true;
				TargetZOffSet = RoadZ + 0.9f - Source.z;
			}else{
				if(m_bCollisionChecksOn)
					PreviouslyFailedRoadHeightCheck = false;
				else
					TargetZOffSet = 0.0f;
			}
		}else{
			if(CWorld::ProcessVerticalLine(Source, 1000.0f, colpoint, ent, true, false, false, false, false, false, nil) &&
			   ent->IsBuilding()){
				FoundRoof = true;
				RoofZ = colpoint.point.z;
			}
			if(FoundRoof){
				if(Source.z - RoofZ < 0.9f){
					PreviouslyFailedRoadHeightCheck = true;
					TargetZOffSet = RoofZ + 0.9f - Source.z;
				}else{
					if(m_bCollisionChecksOn)
						PreviouslyFailedRoadHeightCheck = false;
					else
						TargetZOffSet = 0.0f;
				}
			}
		}
	}

	if(TargetZOffSet > RoadHeightFix)
		RoadHeightFix = TargetZOffSet;
	else
		WellBufferMe(TargetZOffSet, &RoadHeightFix, &RoadHeightFixSpeed, 0.27f, 0.1f, false);

	if((colpoint.surfaceB == SURFACE_DEFAULT || colpoint.surfaceB >= SURFACE_METAL6) &&
	   colpoint.surfaceB != SURFACE_STEEL && colpoint.surfaceB != SURFACE_STONE &&
	   RoadHeightFix > 1.4f)
		RoadHeightFix = 1.4f;

	Source.z += RoadHeightFix;
}

void
CCam::WorkOutCamHeight(const CVector &TargetCoors, float TargetOrientation, float TargetHeight)
{
	static float LastTargetAlphaWithCollisionOn = 0.0f;
	static float LastTopAlphaSpeed = 0.0f;
	static float LastAlphaSpeedStep = 0.0f;
	static bool PreviousNearCheckNearClipSmall = false;

	bool CamClear = true;
	float ModeAlpha = 0.0f;

	if(ResetStatics){
		LastTargetAlphaWithCollisionOn = 0.0f;
		LastTopAlphaSpeed = 0.0f;
		LastAlphaSpeedStep = 0.0f;
		PreviousNearCheckNearClipSmall = false;
	}

	float TopAlphaSpeed = 0.15f;
	float AlphaSpeedStep = 0.015f;

	float zoomvalue = TheCamera.CarZoomValueSmooth;
	if(zoomvalue < 0.1f)
		zoomvalue = 0.1f;
	if(TheCamera.CarZoomIndicator == 1.0f)
		ModeAlpha = CGeneral::GetATanOfXY(23.0f, zoomvalue);	// near
	else if(TheCamera.CarZoomIndicator == 2.0f)
		ModeAlpha = CGeneral::GetATanOfXY(10.8f, zoomvalue);	// mid
	else if(TheCamera.CarZoomIndicator == 3.0f)
		ModeAlpha = CGeneral::GetATanOfXY(7.0f, zoomvalue);	// far


	float Length = (Source - TargetCoors).Magnitude2D();
	if(m_bCollisionChecksOn){	// there's another variable (on PC) but it's uninitialised
		CVector Forward = CamTargetEntity->GetForward();
		float CarAlpha = CGeneral::GetATanOfXY(Forward.Magnitude2D(), Forward.z);
		// this shouldn't be necessary....
		while(CarAlpha >= PI) CarAlpha -= 2*PI;
		while(CarAlpha < -PI) CarAlpha += 2*PI;

		while(Beta >= PI) Beta -= 2*PI;
		while(Beta < -PI) Beta += 2*PI;

		float deltaBeta = Beta - TargetOrientation;
		while(deltaBeta >= PI) deltaBeta -= 2*PI;
		while(deltaBeta < -PI) deltaBeta += 2*PI;

		float BehindCarNess = Cos(deltaBeta);	// 1 if behind car, 0 if side, -1 if in front
		CarAlpha = -CarAlpha * BehindCarNess;
		if(CarAlpha < -0.01f)
			CarAlpha = -0.01f;

		float DeltaAlpha = CarAlpha - Alpha;
		while(DeltaAlpha >= PI) DeltaAlpha -= 2*PI;
		while(DeltaAlpha < -PI) DeltaAlpha += 2*PI;
		// What's this?? wouldn't it make more sense to clamp?
		float AngleLimit = DEGTORAD(1.8f);
		if(DeltaAlpha < -AngleLimit)
			DeltaAlpha += AngleLimit;
		else if(DeltaAlpha > AngleLimit)
			DeltaAlpha -= AngleLimit;
		else
			DeltaAlpha = 0.0f;

		// Now the collision

		float TargetAlpha = 0.0f;
		bool FoundRoofCenter = false;
		bool FoundRoofSide1 = false;
		bool FoundRoofSide2 = false;
		bool FoundCamRoof = false;
		bool FoundCamGround = false;
		float CamRoof = 0.0f;
		float CarBottom = TargetCoors.z - TargetHeight/2.0f;

		// Check car center
		float CarRoof = CWorld::FindRoofZFor3DCoord(TargetCoors.x, TargetCoors.y, CarBottom, &FoundRoofCenter);

		// Check sides of the car
		Forward = CamTargetEntity->GetForward();	// we actually still have that...
		Forward.Normalise();	// shouldn't be necessary
		float CarSideAngle = CGeneral::GetATanOfXY(Forward.x, Forward.y) + PI/2.0f;
		float SideX = 2.5f * Cos(CarSideAngle);
		float SideY = 2.5f * Sin(CarSideAngle);
		CWorld::FindRoofZFor3DCoord(TargetCoors.x + SideX, TargetCoors.y + SideY, CarBottom, &FoundRoofSide1);
		CWorld::FindRoofZFor3DCoord(TargetCoors.x - SideX, TargetCoors.y - SideY, CarBottom, &FoundRoofSide2);

		// Now find out at what height we'd like to place the camera
		float CamGround = CWorld::FindGroundZFor3DCoord(Source.x, Source.y, TargetCoors.z + Length*Sin(Alpha + ModeAlpha) + m_fCloseInCarHeightOffset, &FoundCamGround);
		float CamTargetZ = 0.0f;
		if(FoundCamGround){
			// This is the normal case
			CamRoof = CWorld::FindRoofZFor3DCoord(Source.x, Source.y, CamGround + TargetHeight, &FoundCamRoof);
			CamTargetZ = CamGround + TargetHeight*1.5f + 0.1f;
		}else{
			FoundCamRoof = false;
			CamTargetZ = TargetCoors.z;
		}

		if(FoundRoofCenter && !FoundCamRoof && (FoundRoofSide1 || FoundRoofSide2)){
			// Car is under something but camera isn't
			// This seems weird...
			TargetAlpha = CGeneral::GetATanOfXY(CA_MAX_DISTANCE, CarRoof - CamTargetZ - 1.5f);
			CamClear = false;
		}
		if(FoundCamRoof){
			// Camera is under something
			float roof = FoundRoofCenter ? min(CamRoof, CarRoof) : CamRoof;
			// Same weirdness again?
			TargetAlpha = CGeneral::GetATanOfXY(CA_MAX_DISTANCE, roof - CamTargetZ - 1.5f);
			CamClear = false;
		}
		while(TargetAlpha >= PI) TargetAlpha -= 2*PI;
		while(TargetAlpha < -PI) TargetAlpha += 2*PI;
		if(TargetAlpha < DEGTORAD(-7.0f))
			TargetAlpha = DEGTORAD(-7.0f);

		// huh?
		if(TargetAlpha > ModeAlpha)
			CamClear = true;
		// Camera is contrained by collision in some way
		PreviousNearCheckNearClipSmall = false;
		if(!CamClear){
			PreviousNearCheckNearClipSmall = true;
			RwCameraSetNearClipPlane(Scene.camera, 0.9f);

			DeltaAlpha = TargetAlpha - (Alpha + ModeAlpha);
			while(DeltaAlpha >= PI) DeltaAlpha -= 2*PI;
			while(DeltaAlpha < -PI) DeltaAlpha += 2*PI;

			TopAlphaSpeed = 0.3f;
			AlphaSpeedStep = 0.03f;
		}

		// Now do things if CamClear...but what is that anyway?
		float CamZ = TargetCoors.z + Length*Sin(Alpha + DeltaAlpha + ModeAlpha) + m_fCloseInCarHeightOffset;
		bool FoundGround, FoundRoof;
		float CamGround2 = CWorld::FindGroundZFor3DCoord(Source.x, Source.y, CamZ, &FoundGround);
		if(FoundGround){
			if(CamClear)
				if(CamZ - CamGround2 < 1.5f){
					PreviousNearCheckNearClipSmall = true;
					RwCameraSetNearClipPlane(Scene.camera, 0.9f);

					float a;
					if(Length == 0.0f || CamGround2 + 1.5f - TargetCoors.z == 0.0f)
						a = Alpha;
					else
						a = CGeneral::GetATanOfXY(Length, CamGround2 + 1.5f - TargetCoors.z);
					while(a > PI) a -= 2*PI;
					while(a < -PI) a += 2*PI;
					DeltaAlpha = a - Alpha;
				}
		}else{
			if(CamClear){
				float CamRoof2 = CWorld::FindRoofZFor3DCoord(Source.x, Source.y, CamZ, &FoundRoof);
				if(FoundRoof && CamZ - CamRoof2 < 1.5f){
					PreviousNearCheckNearClipSmall = true;
					RwCameraSetNearClipPlane(Scene.camera, 0.9f);

					if(CamRoof2 > TargetCoors.z + 3.5f)
						CamRoof2 = TargetCoors.z + 3.5f;

					float a;
					if(Length == 0.0f || CamRoof2 + 1.5f - TargetCoors.z == 0.0f)
						a = Alpha;
					else
						a = CGeneral::GetATanOfXY(Length, CamRoof2 + 1.5f - TargetCoors.z);
					while(a > PI) a -= 2*PI;
					while(a < -PI) a += 2*PI;
					DeltaAlpha = a - Alpha;
				}
			}
		}

		LastTargetAlphaWithCollisionOn = DeltaAlpha + Alpha;
		LastTopAlphaSpeed = TopAlphaSpeed;
		LastAlphaSpeedStep = AlphaSpeedStep;
	}else{
		if(PreviousNearCheckNearClipSmall)
			RwCameraSetNearClipPlane(Scene.camera, 0.9f);
	}

	WellBufferMe(LastTargetAlphaWithCollisionOn, &Alpha, &AlphaSpeed, LastTopAlphaSpeed, LastAlphaSpeedStep, true);

	Source.z = TargetCoors.z + Sin(Alpha + ModeAlpha)*Length + m_fCloseInCarHeightOffset;
}

// Rotate cam behind the car when the car is moving forward
bool
CCam::RotCamIfInFrontCar(CVector &TargetCoors, float TargetOrientation)
{
	bool MovingForward = false;
	CPhysical *phys = (CPhysical*)CamTargetEntity;

	float ForwardSpeed = DotProduct(phys->GetForward(), phys->GetSpeed(CVector(0.0f, 0.0f, 0.0f)));
	if(ForwardSpeed > 0.02f)
		MovingForward = true;

	float Dist = (Source - TargetCoors).Magnitude2D();

	float DeltaBeta = TargetOrientation - Beta;
	while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
	while(DeltaBeta < -PI) DeltaBeta += 2*PI;

	if(Abs(DeltaBeta) > DEGTORAD(20.0f) && MovingForward && TheCamera.m_uiTransitionState == 0)
		m_bFixingBeta = true;

	CPad *pad = CPad::GetPad(0);
	if(!(pad->GetLookBehindForCar() || pad->GetLookBehindForPed() || pad->GetLookLeft() || pad->GetLookRight()))
		if(DirectionWasLooking != LOOKING_FORWARD)
			TheCamera.m_bCamDirectlyBehind = true;

	if(!m_bFixingBeta && !TheCamera.m_bUseTransitionBeta && !TheCamera.m_bCamDirectlyBehind && !TheCamera.m_bCamDirectlyInFront)
		return false;

	bool SetBeta = false;
	if(TheCamera.m_bCamDirectlyBehind || TheCamera.m_bCamDirectlyInFront || TheCamera.m_bUseTransitionBeta)
		if(&TheCamera.Cams[TheCamera.ActiveCam] == this)
			SetBeta = true;

	if(m_bFixingBeta || SetBeta){
		WellBufferMe(TargetOrientation, &Beta, &BetaSpeed, 0.15f, 0.007f, true);

		if(TheCamera.m_bCamDirectlyBehind && &TheCamera.Cams[TheCamera.ActiveCam] == this)
			Beta = TargetOrientation;
		if(TheCamera.m_bCamDirectlyInFront && &TheCamera.Cams[TheCamera.ActiveCam] == this)
			Beta = TargetOrientation + PI;
		if(TheCamera.m_bUseTransitionBeta && &TheCamera.Cams[TheCamera.ActiveCam] == this)
			Beta = m_fTransitionBeta;

		Source.x = TargetCoors.x - Cos(Beta)*Dist;
		Source.y = TargetCoors.y - Sin(Beta)*Dist;

		// Check if we're done
		DeltaBeta = TargetOrientation - Beta;
		while(DeltaBeta >= PI) DeltaBeta -= 2*PI;
		while(DeltaBeta < -PI) DeltaBeta += 2*PI;
		if(Abs(DeltaBeta) < DEGTORAD(2.0f))
			m_bFixingBeta = false;
	}
	TheCamera.m_bCamDirectlyBehind = false;
	TheCamera.m_bCamDirectlyInFront = false;
	return true;
}

// Move the cam to avoid clipping through buildings
bool
CCam::FixCamIfObscured(CVector &TargetCoors, float TargetHeight, float TargetOrientation)
{
	CVector Target = TargetCoors;
	bool UseEntityPos = false;
	CVector EntityPos;
	static CColPoint colPoint;
	static bool LastObscured = false;

	if(Mode == MODE_BEHINDCAR)
		Target.z += TargetHeight/2.0f;
	if(Mode == MODE_CAM_ON_A_STRING){
		UseEntityPos = true;
		Target.z += TargetHeight/2.0f;
		EntityPos = CamTargetEntity->GetPosition();
	}

	CVector TempSource = Source;

	bool Obscured1 = false;
	bool Obscured2 = false;
	bool Fix1 = false;
	float Dist1 = 0.0f;
	float Dist2 = 0.0f;
	CEntity *ent;
	if(m_bCollisionChecksOn || LastObscured){
		Obscured1 = CWorld::ProcessLineOfSight(Target, TempSource, colPoint, ent, true, false, false, true, false, true, true);
		if(Obscured1){
			Dist1 = (Target - colPoint.point).Magnitude2D();
			Fix1 = true;
			if(UseEntityPos)
				Obscured1 = CWorld::ProcessLineOfSight(EntityPos, TempSource, colPoint, ent, true, false, false, true, false, true, true);
		}else if(m_bFixingBeta){
			float d = (TempSource - Target).Magnitude();
			TempSource.x = Target.x - d*Cos(TargetOrientation);
			TempSource.y = Target.y - d*Sin(TargetOrientation);

			// same check again
			Obscured2 = CWorld::ProcessLineOfSight(Target, TempSource, colPoint, ent, true, false, false, true, false, true, true);
			if(Obscured2){
				Dist2 = (Target - colPoint.point).Magnitude2D();
				if(UseEntityPos)
					Obscured2 = CWorld::ProcessLineOfSight(EntityPos, TempSource, colPoint, ent, true, false, false, true, false, true, true);
			}
		}
		LastObscured = Obscured1 || Obscured2;
	}

	// nothing to do
	if(!LastObscured)
		return false;

	if(Fix1){
		Source.x = Target.x - Cos(Beta)*Dist1;
		Source.y = Target.y - Sin(Beta)*Dist1;
		if(Mode == MODE_BEHINDCAR)
			Source = colPoint.point;
	}else{
		WellBufferMe(Dist2, &m_fDistanceBeforeChanges, &DistanceSpeed, 0.2f, 0.025f, false);
		Source.x = Target.x - Cos(Beta)*m_fDistanceBeforeChanges;
		Source.y = Target.y - Sin(Beta)*m_fDistanceBeforeChanges;
	}

	if(ResetStatics){
		m_fDistanceBeforeChanges = (Source - Target).Magnitude2D();
		DistanceSpeed = 0.0f;
		Source.x = colPoint.point.x;
		Source.y = colPoint.point.y;
	}
	return true;
}

void
CCam::Process_Cam_On_A_String(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsVehicle())
		return;

	FOV = DefaultFOV;

	if(ResetStatics){
		AlphaSpeed = 0.0f;
		if(TheCamera.m_bIdleOn)
			TheCamera.m_uiTimeWeEnteredIdle = CTimer::GetTimeInMilliseconds();
	}

	CBaseModelInfo *mi = CModelInfo::GetModelInfo(CamTargetEntity->GetModelIndex());
	CVector Dimensions = mi->GetColModel()->boundingBox.max - mi->GetColModel()->boundingBox.min;
	float BaseDist = Dimensions.Magnitude2D();

	CVector TargetCoors = CameraTarget;
	TargetCoors.z += Dimensions.z - 0.1f;	// final
	Beta = CGeneral::GetATanOfXY(TargetCoors.x - Source.x, TargetCoors.y - Source.y);
	while(Alpha >= PI) Alpha -= 2*PI;
	while(Alpha < -PI) Alpha += 2*PI;
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;

	m_fDistanceBeforeChanges = (Source - TargetCoors).Magnitude2D();

	Cam_On_A_String_Unobscured(TargetCoors, BaseDist);
	WorkOutCamHeight(TargetCoors, TargetOrientation, Dimensions.z);
	RotCamIfInFrontCar(TargetCoors, TargetOrientation);
	FixCamIfObscured(TargetCoors, Dimensions.z, TargetOrientation);
	FixCamWhenObscuredByVehicle(TargetCoors);

	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	Front = TargetCoors - Source;
	Front.Normalise();
	GetVectorsReadyForRW();
	ResetStatics = false;
}

// Basic Cam on a string algorithm
void
CCam::Cam_On_A_String_Unobscured(const CVector &TargetCoors, float BaseDist)
{
	CA_MAX_DISTANCE = BaseDist + 0.1f + TheCamera.CarZoomValueSmooth;
	CA_MIN_DISTANCE = min(BaseDist*0.6f, 3.5f);

	CVector Dist = Source - TargetCoors;

	if(ResetStatics)
		Source = TargetCoors + Dist*(CA_MAX_DISTANCE + 1.0f);

	float Length = Dist.Magnitude2D();
	if(Length < 0.001f){
		// This probably shouldn't happen. reset view
		CVector Forward = CamTargetEntity->GetForward();
		Forward.z = 0.0f;
		Forward.Normalise();
		Source = TargetCoors - Forward*CA_MAX_DISTANCE;
		Dist = Source - TargetCoors;
		Length = Dist.Magnitude2D();
	}

	if(Length > CA_MAX_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MAX_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MAX_DISTANCE;
	}else if(Length < CA_MIN_DISTANCE){
		Source.x = TargetCoors.x + Dist.x/Length * CA_MIN_DISTANCE;
		Source.y = TargetCoors.y + Dist.y/Length * CA_MIN_DISTANCE;
	}
}

void
CCam::FixCamWhenObscuredByVehicle(const CVector &TargetCoors)
{
	// BUG? is this never reset
	static float HeightFixerCarsObscuring = 0.0f;
	static float HeightFixerCarsObscuringSpeed = 0.0f;
	CColPoint colPoint;
	CEntity *entity;

	float HeightTarget = 0.0f;
	if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, false, true, false, false, false, false, false)){
		CBaseModelInfo *mi = CModelInfo::GetModelInfo(entity->GetModelIndex());
		HeightTarget = mi->GetColModel()->boundingBox.max.z + 1.0f + TargetCoors.z - Source.z;
		if(HeightTarget < 0.0f)
			HeightTarget = 0.0f;
	}
	WellBufferMe(HeightTarget, &HeightFixerCarsObscuring, &HeightFixerCarsObscuringSpeed, 0.2f, 0.025f, false);
	Source.z += HeightFixerCarsObscuring;
}

void
CCam::Process_TopDown(const CVector &CameraTarget, float TargetOrientation, float SpeedVar, float TargetSpeedVar)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsVehicle())
		return;

	float Dist;
	float HeightTarget = 0.0f;
	static float AdjustHeightTargetMoveBuffer = 0.0f;
	static float AdjustHeightTargetMoveSpeed = 0.0f;
	static float NearClipDistance = 1.5f;
	const float FarClipDistance = 200.0f;
	CVector TargetFront, Target;
	CVector TestSource, TestTarget;
	CColPoint colPoint;
	CEntity *entity;

	TargetFront = CameraTarget;
	TargetFront.x += 18.0f*CamTargetEntity->GetForward().x*SpeedVar;
	TargetFront.y += 18.0f*CamTargetEntity->GetForward().y*SpeedVar;

	if(ResetStatics){
		AdjustHeightTargetMoveBuffer = 0.0f;
		AdjustHeightTargetMoveSpeed = 0.0f;
	}

	float f = Pow(0.8f, 4.0f);
	Target = f*CameraTarget + (1.0f-f)*TargetFront;
	if(Mode == MODE_GTACLASSIC)
		SpeedVar = TargetSpeedVar;
	Source = Target + CVector(0.0f, 0.0f, (40.0f*SpeedVar + 30.0f)*0.8f);
	// What is this? looks horrible
	if(Mode == MODE_GTACLASSIC)
		Source.x += (uint8)(100.0f*CameraTarget.x)/500.0f;

	TestSource = Source;
	TestTarget = TestSource;
	TestTarget.z = Target.z;
	if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false)){
		if(Source.z < colPoint.point.z+3.0f)
			HeightTarget = colPoint.point.z+3.0f - Source.z;
	}else{
		TestSource = Source;
		TestTarget = TestSource;
		TestTarget.z += 10.0f;
		if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false))
			if(Source.z < colPoint.point.z+3.0f)
				HeightTarget = colPoint.point.z+3.0f - Source.z;
	}
	WellBufferMe(HeightTarget, &AdjustHeightTargetMoveBuffer, &AdjustHeightTargetMoveSpeed, 0.2f, 0.02f, false);
	Source.z += AdjustHeightTargetMoveBuffer;

	if(RwCameraGetFarClipPlane(Scene.camera) > FarClipDistance)
		RwCameraSetFarClipPlane(Scene.camera, FarClipDistance);
	RwCameraSetNearClipPlane(Scene.camera, NearClipDistance);

	Front = CVector(-0.01f, -0.01f, -1.0f);	// look down
	Front.Normalise();
	Dist = (Source - CameraTarget).Magnitude();
	m_cvecTargetCoorsForFudgeInter = Dist*Front + Source;
	Up = CVector(0.0f, 1.0f, 0.0f);

	ResetStatics = false;
}

void
CCam::AvoidWallsTopDownPed(const CVector &TargetCoors, const CVector &Offset, float *Adjuster, float *AdjusterSpeed, float yDistLimit)
{
	float Target = 0.0f;
	float MaxSpeed = 0.13f;
	float Acceleration = 0.015f;
	float SpeedMult;
	float dy;
	CVector TestPoint2;
	CVector TestPoint1;
	CColPoint colPoint;
	CEntity *entity;

	TestPoint2 = TargetCoors + Offset;
	TestPoint1 = TargetCoors;
	TestPoint1.z = TestPoint2.z;
	if(CWorld::ProcessLineOfSight(TestPoint1, TestPoint2, colPoint, entity, true, false, false, false, false, false, false)){
		// What is this even?
		dy = TestPoint1.y - colPoint.point.y;
		if(dy > yDistLimit)
			dy = yDistLimit;
		SpeedMult = yDistLimit - Abs(dy/yDistLimit);

		Target = 2.5f;
		MaxSpeed += SpeedMult*0.3f;
		Acceleration += SpeedMult*0.03f;
	}
	WellBufferMe(Target, Adjuster, AdjusterSpeed, MaxSpeed, Acceleration, false);
}

void
CCam::Process_TopDownPed(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	float Dist;
	float HeightTarget;
	static int NumPedPosCountsSoFar = 0;
	static float PedAverageSpeed = 0.0f;
	static float AdjustHeightTargetMoveBuffer = 0.0f;
	static float AdjustHeightTargetMoveSpeed = 0.0f;
	static float PedSpeedSoFar = 0.0f;
	static float FarClipDistance = 200.0f;
	static float NearClipDistance = 1.5f;
	static float TargetAdjusterForSouth = 0.0f;
	static float TargetAdjusterSpeedForSouth = 0.0f;
	static float TargetAdjusterForNorth = 0.0f;
	static float TargetAdjusterSpeedForNorth = 0.0f;
	static float TargetAdjusterForEast = 0.0f;
	static float TargetAdjusterSpeedForEast = 0.0f;
	static float TargetAdjusterForWest = 0.0f;
	static float TargetAdjusterSpeedForWest = 0.0f;
	static CVector PreviousPlayerMoveSpeedVec;
	CVector TargetCoors, PlayerMoveSpeed;
	CVector TestSource, TestTarget;
	CColPoint colPoint;
	CEntity *entity;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;
	PlayerMoveSpeed = ((CPed*)CamTargetEntity)->GetMoveSpeed();

	if(ResetStatics){
		PreviousPlayerMoveSpeedVec = PlayerMoveSpeed;
		AdjustHeightTargetMoveBuffer = 0.0f;
		AdjustHeightTargetMoveSpeed = 0.0f;
		NumPedPosCountsSoFar = 0;
		PedSpeedSoFar = 0.0f;
		PedAverageSpeed = 0.0f;
		TargetAdjusterForWest = 0.0f;
		TargetAdjusterSpeedForWest = 0.0f;
		TargetAdjusterForEast = 0.0f;
		TargetAdjusterSpeedForEast = 0.0f;
		TargetAdjusterForNorth = 0.0f;
		TargetAdjusterSpeedForNorth = 0.0f;
		TargetAdjusterForSouth = 0.0f;
		TargetAdjusterSpeedForSouth = 0.0f;
	}

	if(RwCameraGetFarClipPlane(Scene.camera) > FarClipDistance)
		RwCameraSetFarClipPlane(Scene.camera, FarClipDistance);
	RwCameraSetNearClipPlane(Scene.camera, NearClipDistance);

	// Average ped speed
	NumPedPosCountsSoFar++;
	PedSpeedSoFar += PlayerMoveSpeed.Magnitude();
	if(NumPedPosCountsSoFar == 5){
		PedAverageSpeed = 0.4f*PedAverageSpeed + 0.6*(PedSpeedSoFar/5.0f);
		NumPedPosCountsSoFar = 0;
		PedSpeedSoFar = 0.0f;
	}
	PreviousPlayerMoveSpeedVec = PlayerMoveSpeed;

	// Zoom out depending on speed
	if(PedAverageSpeed > 0.01f && PedAverageSpeed <= 0.04f)
		HeightTarget = 2.5f;
	else if(PedAverageSpeed > 0.04f && PedAverageSpeed <= 0.145f)
		HeightTarget = 4.5f;
	else if(PedAverageSpeed > 0.145f)
		HeightTarget = 7.0f;
	else
		HeightTarget = 0.0f;

	// Zoom out if locked on target is far away
	if(FindPlayerPed()->m_pPointGunAt){
		Dist = (FindPlayerPed()->m_pPointGunAt->GetPosition() - CameraTarget).Magnitude2D();
		if(Dist > 6.0f)
			HeightTarget = max(HeightTarget, Dist/22.0f*37.0f);
	}

	Source = TargetCoors + CVector(0.0f, -1.0f, 9.0f);

	// Collision checks
	entity = nil;
	TestSource = TargetCoors + CVector(0.0f, -1.0f, 9.0f);
	TestTarget = TestSource;
	TestTarget.z = TargetCoors.z;
	if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false)){
		if(TargetCoors.z+9.0f+HeightTarget < colPoint.point.z+3.0f)
			HeightTarget = colPoint.point.z+3.0f - (TargetCoors.z+9.0f);
	}else{
		TestSource = TargetCoors + CVector(0.0f, -1.0f, 9.0f);
		TestTarget = TestSource;
		TestSource.z += HeightTarget;
		TestTarget.z = TestSource.z + 10.0f;
		if(CWorld::ProcessLineOfSight(TestTarget, TestSource, colPoint, entity, true, false, false, false, false, false, false)){
			if(TargetCoors.z+9.0f+HeightTarget < colPoint.point.z+3.0f)
				HeightTarget = colPoint.point.z+3.0f - (TargetCoors.z+9.0f);
		}
	}

	WellBufferMe(HeightTarget, &AdjustHeightTargetMoveBuffer, &AdjustHeightTargetMoveSpeed, 0.3f, 0.03f, false);
	Source.z += AdjustHeightTargetMoveBuffer;

	// Wall checks
	AvoidWallsTopDownPed(TargetCoors, CVector(0.0f, -3.0f, 3.0f), &TargetAdjusterForSouth, &TargetAdjusterSpeedForSouth, 1.0f);
	Source.y += TargetAdjusterForSouth;
	AvoidWallsTopDownPed(TargetCoors, CVector(0.0f, 3.0f, 3.0f), &TargetAdjusterForNorth, &TargetAdjusterSpeedForNorth, 1.0f);
	Source.y -= TargetAdjusterForNorth;
	// BUG: east and west flipped
	AvoidWallsTopDownPed(TargetCoors, CVector(3.0f, 0.0f, 3.0f), &TargetAdjusterForWest, &TargetAdjusterSpeedForWest, 1.0f);
	Source.x -= TargetAdjusterForWest;
	AvoidWallsTopDownPed(TargetCoors, CVector(-3.0f, 0.0f, 3.0f), &TargetAdjusterForEast, &TargetAdjusterSpeedForEast, 1.0f);
	Source.x += TargetAdjusterForEast;

	TargetCoors.y = Source.y + 1.0f;
	TargetCoors.y += TargetAdjusterForSouth;
	TargetCoors.x += TargetAdjusterForEast;
	TargetCoors.x -= TargetAdjusterForWest;

	Front = TargetCoors - Source;
	Front.Normalise();
#ifdef FIX_BUGS
	if(Front.x == 0.0f && Front.y == 0.0f)
		Front.y = 0.0001f;
#else
	// someone used = instead of == in the above check by accident
	Front.x = 0.0f;
#endif
	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	Up = CrossProduct(Front, CVector(-1.0f, 0.0f, 0.0f));
	Up.Normalise();

	ResetStatics = false;
}

// Identical to M16
void
CCam::Process_Rocket(const CVector &CameraTarget, float, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	static bool FailedTestTwelveFramesAgo = false;
	RwV3d HeadPos;
	CVector TargetCoors;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;

	if(ResetStatics){
		Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		FailedTestTwelveFramesAgo = false;
		// static DPadVertical unused
		// static DPadHorizontal unused
		m_bCollisionChecksOn = true;
		ResetStatics = false;
	}

	((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(&HeadPos, PED_HEAD);
	Source = HeadPos;
	Source.z += 0.1f;
	Source.x -= 0.19f*Cos(m_fInitialPlayerOrientation);
	Source.y -= 0.19f*Sin(m_fInitialPlayerOrientation);

	// Look around
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
	if(MouseX != 0.0f || MouseY != 0.0f){
		UseMouse = true;
		LookLeftRight = -3.0f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->SniperModeLookLeftRight();
		LookUpDown = CPad::GetPad(0)->SniperModeLookUpDown();
	}
	if(UseMouse){
		Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
		Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
	}else{
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir/17.5 * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir/14.0f * FOV/80.0f * CTimer::GetTimeStep();
	}
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;
	if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

	TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
	TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
	TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
	Front = TargetCoors - Source;
	Front.Normalise();
	Source += Front*0.4f;

	if(m_bCollisionChecksOn){
		if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
			FailedTestTwelveFramesAgo = true;
		}else{
			CVector TestPoint;
			TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
			TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
			TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
			if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else
					FailedTestTwelveFramesAgo = false;
			}
		}
	}

	if(FailedTestTwelveFramesAgo)
		RwCameraSetNearClipPlane(Scene.camera, 0.4f);
	Source -= Front*0.4f;

	GetVectorsReadyForRW();
	float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
}

// Identical to Rocket
void
CCam::Process_M16_1stPerson(const CVector &CameraTarget, float, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	static bool FailedTestTwelveFramesAgo = false;
	RwV3d HeadPos;
	CVector TargetCoors;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;

	if(ResetStatics){
		Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		FailedTestTwelveFramesAgo = false;
		// static DPadVertical unused
		// static DPadHorizontal unused
		m_bCollisionChecksOn = true;
		ResetStatics = false;
	}

	((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(&HeadPos, PED_HEAD);
	Source = HeadPos;
	Source.z += 0.1f;
	Source.x -= 0.19f*Cos(m_fInitialPlayerOrientation);
	Source.y -= 0.19f*Sin(m_fInitialPlayerOrientation);

	// Look around
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
	if(MouseX != 0.0f || MouseY != 0.0f){
		UseMouse = true;
		LookLeftRight = -3.0f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->SniperModeLookLeftRight();
		LookUpDown = CPad::GetPad(0)->SniperModeLookUpDown();
	}
	if(UseMouse){
		Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
		Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
	}else{
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir/17.5 * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir/14.0f * FOV/80.0f * CTimer::GetTimeStep();
	}
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;
	if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

	TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
	TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
	TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
	Front = TargetCoors - Source;
	Front.Normalise();
	Source += Front*0.4f;

	if(m_bCollisionChecksOn){
		if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
			FailedTestTwelveFramesAgo = true;
		}else{
			CVector TestPoint;
			TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
			TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
			TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
			if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else
					FailedTestTwelveFramesAgo = false;
			}
		}
	}

	if(FailedTestTwelveFramesAgo)
		RwCameraSetNearClipPlane(Scene.camera, 0.4f);
	Source -= Front*0.4f;

	GetVectorsReadyForRW();
	float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
}

void
CCam::Process_1stPerson(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	static float DontLookThroughWorldFixer = 0.0f;
	CVector TargetCoors;

	FOV = DefaultFOV;
	TargetCoors = CameraTarget;
	if(CamTargetEntity->m_rwObject == nil)
		return;

	if(ResetStatics){
		Beta = TargetOrientation;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = TargetOrientation;
		if(CamTargetEntity->IsPed()){
			Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
			Alpha = 0.0f;
			m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
		}
		DontLookThroughWorldFixer = 0.0f;
	}

	if(CamTargetEntity->IsPed()){
		static bool FailedTestTwelveFramesAgo = false;
		RwV3d HeadPos;

		TargetCoors = CameraTarget;

		if(ResetStatics){
			Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
			Alpha = 0.0f;
			m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
			FailedTestTwelveFramesAgo = false;
			// static DPadVertical unused
			// static DPadHorizontal unused
			m_bCollisionChecksOn = true;
			ResetStatics = false;
		}

		((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(&HeadPos, PED_HEAD);
		Source = HeadPos;
		Source.z += 0.1f;
		Source.x -= 0.19f*Cos(m_fInitialPlayerOrientation);
		Source.y -= 0.19f*Sin(m_fInitialPlayerOrientation);

		float LookLeftRight, LookUpDown;
		LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight();
		LookUpDown = CPad::GetPad(0)->LookAroundUpDown();
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir/17.5 * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		while(Beta >= PI) Beta -= 2*PI;
		while(Beta < -PI) Beta += 2*PI;
		if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
		if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

		TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
		TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
		TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
		Front = TargetCoors - Source;
		Front.Normalise();
		Source += Front*0.4f;

		if(m_bCollisionChecksOn){
			if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				CVector TestPoint;
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else{
					TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
					TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
					TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
					if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
						RwCameraSetNearClipPlane(Scene.camera, 0.4f);
						FailedTestTwelveFramesAgo = true;
					}else
						FailedTestTwelveFramesAgo = false;
				}
			}
		}

		if(FailedTestTwelveFramesAgo)
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
		Source -= Front*0.4f;

		GetVectorsReadyForRW();
		float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
	}else{
		assert(CamTargetEntity->IsVehicle());
		CVehicleModelInfo *mi = (CVehicleModelInfo*)CModelInfo::GetModelInfo(CamTargetEntity->GetModelIndex());
		CVector CamPos = mi->m_vehicleType == VEHICLE_TYPE_BOAT ? mi->m_positions[BOAT_POS_FRONTSEAT] : mi->m_positions[CAR_POS_FRONTSEAT];
		CamPos.x = 0.0f;
		CamPos.y += -0.08f;
		CamPos.z += 0.62f;
		FOV = 60.0f;
		Source = Multiply3x3(CamTargetEntity->GetMatrix(), CamPos);
		Source += CamTargetEntity->GetPosition();
		if(((CVehicle*)CamTargetEntity)->IsBoat())
			Source.z += 0.5f;

		if(((CVehicle*)CamTargetEntity)->IsUpsideDown()){
			if(DontLookThroughWorldFixer < 0.5f)
				DontLookThroughWorldFixer += 0.03f;
			else
				DontLookThroughWorldFixer = 0.5f;
		}else{
			if(DontLookThroughWorldFixer < 0.0f)
#ifdef FIX_BUGS
				DontLookThroughWorldFixer += 0.03f;
#else
				DontLookThroughWorldFixer -= 0.03f;
#endif
			else
				DontLookThroughWorldFixer = 0.0f;
		}
		Source.z += DontLookThroughWorldFixer;
		Front = CamTargetEntity->GetForward();
		Front.Normalise();
		Up = CamTargetEntity->GetUp();
		Up.Normalise();
		CVector Right = CrossProduct(Front, Up);
		Right.Normalise();
		Up = CrossProduct(Right, Front);
		Up.Normalise();
	}

	ResetStatics = false;
}

static CVector vecHeadCamOffset(0.06f, 0.05f, 0.0f);

void
CCam::Process_1rstPersonPedOnPC(const CVector&, float TargetOrientation, float, float)
{
	// static int DontLookThroughWorldFixer = 0;	// unused
	static CVector InitialHeadPos;

	if(Mode != MODE_SNIPER_RUNABOUT)
		FOV = DefaultFOV;
	TheCamera.m_1rstPersonRunCloseToAWall = false;
	if(CamTargetEntity->m_rwObject == nil)
		return;

	if(CamTargetEntity->IsPed()){
		// static bool FailedTestTwelveFramesAgo = false;	// unused
		RwV3d HeadPos = vecHeadCamOffset;
		CVector TargetCoors;

		// needs fix for SKINNING
		RwFrame *frm = ((CPed*)CamTargetEntity)->GetNodeFrame(PED_HEAD);
		while(frm){
			RwV3dTransformPoints(&HeadPos, &HeadPos, 1, RwFrameGetMatrix(frm));
			frm = RwFrameGetParent(frm);
			if(frm == RpClumpGetFrame(CamTargetEntity->GetClump()))
				frm = nil;
		}

		if(ResetStatics){
			Beta = TargetOrientation;
			Alpha = 0.0f;
			m_fInitialPlayerOrientation = TargetOrientation;
			if(CamTargetEntity->IsPed()){	// useless check
				Beta = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
				Alpha = 0.0f;
				m_fInitialPlayerOrientation = ((CPed*)CamTargetEntity)->m_fRotationCur + HALFPI;
				// FailedTestTwelveFramesAgo = false;
				m_bCollisionChecksOn = true;
			}
			// DontLookThroughWorldFixer = false;
			m_vecBufferedPlayerBodyOffset = HeadPos;
			InitialHeadPos = HeadPos;
		}

		m_vecBufferedPlayerBodyOffset.y = HeadPos.y;

		if(TheCamera.m_bHeadBob){
			m_vecBufferedPlayerBodyOffset.x =
				TheCamera.m_fGaitSwayBuffer * m_vecBufferedPlayerBodyOffset.x +
				(1.0f-TheCamera.m_fGaitSwayBuffer) * HeadPos.x;
			m_vecBufferedPlayerBodyOffset.z =
				TheCamera.m_fGaitSwayBuffer * m_vecBufferedPlayerBodyOffset.z +
				(1.0f-TheCamera.m_fGaitSwayBuffer) * HeadPos.z;
			HeadPos = RwV3d(CamTargetEntity->GetMatrix() * m_vecBufferedPlayerBodyOffset);
		}else{
			float HeadDelta = (HeadPos - InitialHeadPos).Magnitude2D();
			CVector Fwd = CamTargetEntity->GetForward();
			Fwd.z = 0.0f;
			Fwd.Normalise();
			HeadPos = RwV3d(HeadDelta*1.23f*Fwd + CamTargetEntity->GetPosition());
			HeadPos.z += 0.59f;
		}
		Source = HeadPos;

		// unused:
		// ((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(&MidPos, PED_MID);
		// Source - MidPos;

		// Look around
		bool UseMouse = false;
		float MouseX = CPad::GetPad(0)->GetMouseX();
		float MouseY = CPad::GetPad(0)->GetMouseY();
		float LookLeftRight, LookUpDown;
		if(MouseX != 0.0f || MouseY != 0.0f){
			UseMouse = true;
			LookLeftRight = -3.0f*MouseX;
			LookUpDown = 4.0f*MouseY;
		}else{
			LookLeftRight = -CPad::GetPad(0)->LookAroundLeftRight();
			LookUpDown = CPad::GetPad(0)->LookAroundUpDown();
		}
		if(UseMouse){
			Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
			Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
		}else{
			float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
			float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
			Beta += SQR(LookLeftRight/100.0f)*xdir/17.5 * FOV/80.0f * CTimer::GetTimeStep();
			Alpha += SQR(LookUpDown/150.0f)*ydir/14.0f * FOV/80.0f * CTimer::GetTimeStep();
		}
		while(Beta >= PI) Beta -= 2*PI;
		while(Beta < -PI) Beta += 2*PI;
		if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
		if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

		TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
		TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
		TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;
		Front = TargetCoors - Source;
		Front.Normalise();
		Source += Front*0.4f;

		TheCamera.m_AlphaForPlayerAnim1rstPerson = Alpha;

		GetVectorsReadyForRW();

		float Heading = Front.Heading();
		((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Heading;
		((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Heading;
		TheCamera.pTargetEntity->SetHeading(Heading);
		TheCamera.pTargetEntity->GetMatrix().UpdateRW();

		if(Mode == MODE_SNIPER_RUNABOUT){
			// no mouse wheel FOV buffering here like in normal sniper mode
			if(CPad::GetPad(0)->SniperZoomIn() || CPad::GetPad(0)->SniperZoomOut()){
				if(CPad::GetPad(0)->SniperZoomOut())
					FOV *= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
				else
					FOV /= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
			}

			TheCamera.SetMotionBlur(180, 255, 180, 120, MBLUR_SNIPER);

			if(FOV > DefaultFOV)
				FOV = DefaultFOV;
			if(FOV < 15.0f)
				FOV = 15.0f;
		}
	}

	ResetStatics = false;
	RwCameraSetNearClipPlane(Scene.camera, 0.05f);
}

void
CCam::Process_Sniper(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(CamTargetEntity->m_rwObject == nil)
		return;

#ifdef FIX_BUGS
	if(!CamTargetEntity->IsPed())
		return;
#endif

	static bool FailedTestTwelveFramesAgo = false;
	RwV3d HeadPos;
	CVector TargetCoors;
	TargetCoors = CameraTarget;

	static float TargetFOV = 0.0f;

	if(ResetStatics){
		Beta = TargetOrientation;
		Alpha = 0.0f;
		m_fInitialPlayerOrientation = TargetOrientation;
		FailedTestTwelveFramesAgo = false;
		// static DPadVertical unused
		// static DPadHorizontal unused
		m_bCollisionChecksOn = true;
		FOVSpeed = 0.0f;
		TargetFOV = FOV;
		ResetStatics = false;
	}

	((CPed*)CamTargetEntity)->m_pedIK.GetComponentPosition(&HeadPos, PED_HEAD);
	Source = HeadPos;
	Source.z += 0.1f;
	Source.x -= 0.19f*Cos(m_fInitialPlayerOrientation);
	Source.y -= 0.19f*Sin(m_fInitialPlayerOrientation);

	// Look around
	bool UseMouse = false;
	float MouseX = CPad::GetPad(0)->GetMouseX();
	float MouseY = CPad::GetPad(0)->GetMouseY();
	float LookLeftRight, LookUpDown;
	if(MouseX != 0.0f || MouseY != 0.0f){
		UseMouse = true;
		LookLeftRight = -3.0f*MouseX;
		LookUpDown = 4.0f*MouseY;
	}else{
		LookLeftRight = -CPad::GetPad(0)->SniperModeLookLeftRight();
		LookUpDown = CPad::GetPad(0)->SniperModeLookUpDown();
	}
	if(UseMouse){
		Beta += TheCamera.m_fMouseAccelHorzntl * LookLeftRight * FOV/80.0f;
		Alpha += TheCamera.m_fMouseAccelVertical * LookUpDown * FOV/80.0f;
	}else{
		float xdir = LookLeftRight < 0.0f ? -1.0f : 1.0f;
		float ydir = LookUpDown < 0.0f ? -1.0f : 1.0f;
		Beta += SQR(LookLeftRight/100.0f)*xdir/17.5 * FOV/80.0f * CTimer::GetTimeStep();
		Alpha += SQR(LookUpDown/150.0f)*ydir/14.0f * FOV/80.0f * CTimer::GetTimeStep();
	}
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;
	if(Alpha > DEGTORAD(60.0f)) Alpha = DEGTORAD(60.0f);
	if(Alpha < -DEGTORAD(89.5f)) Alpha = -DEGTORAD(89.5f);

	TargetCoors.x = 3.0f * Cos(Alpha) * Cos(Beta) + Source.x;
	TargetCoors.y = 3.0f * Cos(Alpha) * Sin(Beta) + Source.y;
	TargetCoors.z = 3.0f * Sin(Alpha) + Source.z;

	UseMouse = false;
	int ZoomInButton = ControlsManager.GetMouseButtonAssociatedWithAction(PED_SNIPER_ZOOM_IN);
	int ZoomOutButton = ControlsManager.GetMouseButtonAssociatedWithAction(PED_SNIPER_ZOOM_OUT);
	// TODO: enum? this should be mouse wheel up and down
	if(ZoomInButton == 4 || ZoomInButton == 5 || ZoomOutButton == 4 || ZoomOutButton == 5){
		if(CPad::GetPad(0)->GetMouseWheelUp() || CPad::GetPad(0)->GetMouseWheelDown()){
			if(CPad::GetPad(0)->SniperZoomIn()){
				TargetFOV = FOV - 10.0f;
				UseMouse = true;
			}
			if(CPad::GetPad(0)->SniperZoomOut()){
				TargetFOV = FOV + 10.0f;
				UseMouse = true;
			}
		}
	}
	if((CPad::GetPad(0)->SniperZoomIn() || CPad::GetPad(0)->SniperZoomOut()) && !UseMouse){
		if(CPad::GetPad(0)->SniperZoomOut()){
			FOV *= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
			TargetFOV = FOV;
			FOVSpeed = 0.0f;
		}else{
			FOV /= (255.0f*CTimer::GetTimeStep() + 10000.0f) / 10000.0f;
			TargetFOV = FOV;
			FOVSpeed = 0.0f;
		}
	}else{
		if(Abs(TargetFOV - FOV) > 0.5f)
			WellBufferMe(TargetFOV, &FOV, &FOVSpeed, 0.5f, 0.25f, false);
		else
			FOVSpeed = 0.0f;
	}

	TheCamera.SetMotionBlur(180, 255, 180, 120, MBLUR_SNIPER);

	if(FOV > DefaultFOV)
		FOV = DefaultFOV;
	if(FOV < 15.0f)
		FOV = 15.0f;

	Front = TargetCoors - Source;
	Front.Normalise();
	Source += Front*0.4f;

	if(m_bCollisionChecksOn){
		if(!CWorld::GetIsLineOfSightClear(TargetCoors, Source, true, true, false, true, false, true, true)){
			RwCameraSetNearClipPlane(Scene.camera, 0.4f);
			FailedTestTwelveFramesAgo = true;
		}else{
			CVector TestPoint;
			TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta + DEGTORAD(35.0f)) + Source.x;
			TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta + DEGTORAD(35.0f)) + Source.y;
			TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
			if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
				RwCameraSetNearClipPlane(Scene.camera, 0.4f);
				FailedTestTwelveFramesAgo = true;
			}else{
				TestPoint.x = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Cos(Beta - DEGTORAD(35.0f)) + Source.x;
				TestPoint.y = 3.0f * Cos(Alpha - DEGTORAD(20.0f)) * Sin(Beta - DEGTORAD(35.0f)) + Source.y;
				TestPoint.z = 3.0f * Sin(Alpha - DEGTORAD(20.0f)) + Source.z;
				if(!CWorld::GetIsLineOfSightClear(TestPoint, Source, true, true, false, true, false, true, true)){
					RwCameraSetNearClipPlane(Scene.camera, 0.4f);
					FailedTestTwelveFramesAgo = true;
				}else
					FailedTestTwelveFramesAgo = false;
			}
		}
	}

	if(FailedTestTwelveFramesAgo)
		RwCameraSetNearClipPlane(Scene.camera, 0.4f);
	Source -= Front*0.4f;

	GetVectorsReadyForRW();
	float Rotation = CGeneral::GetATanOfXY(Front.x, Front.y) - HALFPI;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Rotation;
	((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Rotation;
}

void
CCam::Process_Syphon(const CVector &CameraTarget, float, float, float)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsPed())
		return;

	static bool CameraObscured = false;
	// unused FailedClippingTestPrevously
	static float BetaOffset = DEGTORAD(18.0f);
	// unused AngleToGoTo
	// unused AngleToGoToSpeed
	// unused DistBetweenPedAndPlayerPreviouslyOn
	static float HeightDown = -0.5f;
	static float PreviousDistForInter;
	CVector TargetCoors;
	CVector2D vDist;
	float fDist, fAimingDist;
	float TargetAlpha;
	CColPoint colPoint;
	CEntity *entity;

	TargetCoors = CameraTarget;

	if(TheCamera.Cams[TheCamera.ActiveCam].Mode != MODE_SYPHON)
		return;

	vDist = Source - TargetCoors;
	fDist = vDist.Magnitude();
	if(fDist == 0.0f)
		Source = TargetCoors + CVector(1.0f, 1.0f, 0.0f);
	else
		Source = TargetCoors + CVector(vDist.x/fDist * 1.7f, vDist.y/fDist * 1.7f, 0.0f);
	if(fDist > 1.7f)
		fDist = 1.7f;

	Beta = CGeneral::GetATanOfXY(Source.x - TargetCoors.x, Source.y - TargetCoors.y);
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;

	float NewBeta = CGeneral::GetATanOfXY(TheCamera.m_cvecAimingTargetCoors.x - TargetCoors.x, TheCamera.m_cvecAimingTargetCoors.y - TargetCoors.y) + PI;
	if(ResetStatics){
		CameraObscured = false;
		float TestBeta1 = NewBeta - BetaOffset - Beta;
		float TestBeta2 = NewBeta + BetaOffset - Beta;
		MakeAngleLessThan180(TestBeta1);
		MakeAngleLessThan180(TestBeta2);
		if(Abs(TestBeta1) < Abs(TestBeta2))
			BetaOffset = -BetaOffset;
		// some unuseds
		ResetStatics = false;
	}
	Beta = NewBeta + BetaOffset;
	Source = TargetCoors;
	Source.x += 1.7f*Cos(Beta);
	Source.y += 1.7f*Sin(Beta);
	TargetCoors.z += m_fSyphonModeTargetZOffSet;
	fAimingDist = (TheCamera.m_cvecAimingTargetCoors - TargetCoors).Magnitude2D();
	if(fAimingDist < 6.5f)
		fAimingDist = 6.5f;
	TargetAlpha = CGeneral::GetATanOfXY(fAimingDist, TheCamera.m_cvecAimingTargetCoors.z - TargetCoors.z);
	while(TargetAlpha >= PI) TargetAlpha -= 2*PI;
	while(TargetAlpha < -PI) TargetAlpha += 2*PI;

	// inlined
	WellBufferMe(-TargetAlpha, &Alpha, &AlphaSpeed, 0.07f, 0.015f, true);

	Source.z += fDist*Sin(Alpha) + fDist*0.2f;
	if(Source.z < TargetCoors.z + HeightDown)
		Source.z = TargetCoors.z + HeightDown;

	CameraObscured = CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, false, false, true, false, true, true);
	// PreviousDistForInter unused
	if(CameraObscured){
		PreviousDistForInter = (TargetCoors - colPoint.point).Magnitude2D();
		Source = colPoint.point;
	}else
		PreviousDistForInter = 1.7f;

	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	Front = TargetCoors - Source;
	m_fMinDistAwayFromCamWhenInterPolating = Front.Magnitude2D();
	if(m_fMinDistAwayFromCamWhenInterPolating < 1.1f)
		RwCameraSetNearClipPlane(Scene.camera, max(m_fMinDistAwayFromCamWhenInterPolating - 0.35f, 0.05f));
	Front.Normalise();
	GetVectorsReadyForRW();
}

void
CCam::Process_Syphon_Crim_In_Front(const CVector &CameraTarget, float, float, float)
{
	FOV = DefaultFOV;

	if(!CamTargetEntity->IsPed())
		return;

	CVector TargetCoors = CameraTarget;
	CVector vDist;
	float fDist, TargetDist;
	float zOffset;
	float AimingAngle;
	CColPoint colPoint;
	CEntity *entity;

	TargetDist = TheCamera.m_fPedZoomValueSmooth * 0.5f + 4.0f;
	vDist = Source - TargetCoors;
	fDist = vDist.Magnitude2D();
	zOffset = TargetDist - 2.65f;
	if(zOffset < 0.0f)
		zOffset = 0.0f;
	if(zOffset == 0.0f)
		Source = TargetCoors + CVector(1.0f, 1.0f, zOffset);
	else
		Source = TargetCoors + CVector(vDist.x/fDist*TargetDist, vDist.y/fDist*TargetDist, zOffset);

	AimingAngle = CGeneral::GetATanOfXY(TheCamera.m_cvecAimingTargetCoors.x - TargetCoors.x, TheCamera.m_cvecAimingTargetCoors.y - TargetCoors.y);
	while(AimingAngle >= PI) AimingAngle -= 2*PI;
	while(AimingAngle < -PI) AimingAngle += 2*PI;

	if(TheCamera.PlayerWeaponMode.Mode == MODE_SYPHON)
		Beta = AimingAngle + m_fPlayerInFrontSyphonAngleOffSet;

	Source.x = TargetCoors.x;
	Source.y = TargetCoors.y;
	Source.x += Cos(Beta) * TargetDist;
	Source.y += Sin(Beta) * TargetDist;

	if(CWorld::ProcessLineOfSight(TargetCoors, Source, colPoint, entity, true, false, false, true, false, true, true)){
		Beta = CGeneral::GetATanOfXY(Source.x - TargetCoors.x, Source.y - TargetCoors.y);
		fDist = (TargetCoors - colPoint.point).Magnitude2D();
		Source.x = TargetCoors.x;
		Source.y = TargetCoors.y;
		Source.x += Cos(Beta) * fDist;
		Source.y += Sin(Beta) * fDist;
	}

	TargetCoors = CameraTarget;
	TargetCoors.z += m_fSyphonModeTargetZOffSet;
	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	Front = TargetCoors - Source;
	GetVectorsReadyForRW();
}

void
CCam::Process_BehindBoat(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsVehicle()){
		ResetStatics = false;
		return;
	}

	CVector TargetCoors = CameraTarget;
	float DeltaBeta = 0.0f;
	static CColPoint colPoint;
	CEntity *entity;
	static float TargetWhenChecksWereOn = 0.0f;
	static float CenterObscuredWhenChecksWereOn = 0.0f;
	static float WaterZAddition = 2.75f;
	float WaterLevel = 0.0f;
	float s, c;
	float Beta = CGeneral::GetATanOfXY(TargetCoors.x - Source.x, TargetCoors.y - Source.y);
	FOV = DefaultFOV;

	if(ResetStatics){
		CenterObscuredWhenChecksWereOn = 0.0f;
		TargetWhenChecksWereOn = 0.0f;
		Beta = TargetOrientation + PI;
	}

	CWaterLevel::GetWaterLevelNoWaves(TargetCoors.x, TargetCoors.y, TargetCoors.z, &WaterLevel);
	WaterLevel += WaterZAddition;
	static float FixerForGoingBelowGround = 0.4f;
	if(-FixerForGoingBelowGround < TargetCoors.z-WaterLevel)
		WaterLevel += TargetCoors.z-WaterLevel - FixerForGoingBelowGround;

	bool Obscured;
	if(m_bCollisionChecksOn || ResetStatics){
		CVector TestPoint;
		// Weird calculations here, also casting bool to float...
		c = Cos(TargetOrientation);
		s = Sin(TargetOrientation);
		TestPoint = TheCamera.CarZoomValueSmooth * CVector(-c, -s, 0.0f) +
			(TheCamera.CarZoomValueSmooth+7.0f) * CVector(-c, -s, 0.0f) +
			TargetCoors;
		TestPoint.z = WaterLevel + TheCamera.CarZoomValueSmooth;
		float Test1 = CWorld::GetIsLineOfSightClear(TestPoint, TargetCoors, true, false, false, true, false, true, true);

		c = Cos(TargetOrientation + 0.8f);
		s = Sin(TargetOrientation + DEGTORAD(40.0f));
		TestPoint = TheCamera.CarZoomValueSmooth * CVector(-c, -s, 0.0f) +
			(TheCamera.CarZoomValueSmooth+7.0f) * CVector(-c, -s, 0.0f) +
			TargetCoors;
		TestPoint.z = WaterLevel + TheCamera.CarZoomValueSmooth;
		float Test2 = CWorld::GetIsLineOfSightClear(TestPoint, TargetCoors, true, false, false, true, false, true, true);

		c = Cos(TargetOrientation - 0.8);
		s = Sin(TargetOrientation - DEGTORAD(40.0f));
		TestPoint = TheCamera.CarZoomValueSmooth * CVector(-c, -s, 0.0f) +
			(TheCamera.CarZoomValueSmooth+7.0f) * CVector(-c, -s, 0.0f) +
			TargetCoors;
		TestPoint.z = WaterLevel + TheCamera.CarZoomValueSmooth;
		float Test3 = CWorld::GetIsLineOfSightClear(TestPoint, TargetCoors, true, false, false, true, false, true, true);

		if(Test2 == 0.0f){
			DeltaBeta = TargetOrientation - Beta - DEGTORAD(40.0f);
			if(ResetStatics)
				Beta = TargetOrientation - DEGTORAD(40.0f);
		}else if(Test3 == 0.0f){
			DeltaBeta = TargetOrientation - Beta + DEGTORAD(40.0f);
			if(ResetStatics)
				Beta = TargetOrientation + DEGTORAD(40.0f);
		}else if(Test1 == 0.0f){
			DeltaBeta = 0.0f;
		}else if(Test2 != 0.0f && Test3 != 0.0f && Test1 != 0.0f){
			if(ResetStatics)
				Beta = TargetOrientation;
			DeltaBeta = TargetOrientation - Beta;
		}

		c = Cos(Beta);
		s = Sin(Beta);
		TestPoint.x = TheCamera.CarZoomValueSmooth * -c +
			(TheCamera.CarZoomValueSmooth + 7.0f) * -c +
			TargetCoors.x;
		TestPoint.y = TheCamera.CarZoomValueSmooth * -s +
			(TheCamera.CarZoomValueSmooth + 7.0f) * -s +
			TargetCoors.y;
		TestPoint.z = WaterLevel + TheCamera.CarZoomValueSmooth;
		Obscured = CWorld::ProcessLineOfSight(TestPoint, TargetCoors, colPoint, entity, true, false, false, true, false, true, true);
		CenterObscuredWhenChecksWereOn = Obscured;

		// now DeltaBeta == TargetWhenChecksWereOn - Beta, which we need for WellBufferMe below
		TargetWhenChecksWereOn = DeltaBeta + Beta;
	}else{
		// DeltaBeta = TargetWhenChecksWereOn - Beta;	// unneeded since we don't inline WellBufferMe
		Obscured = CenterObscuredWhenChecksWereOn != 0.0f;
	}

	if(Obscured){
		CWorld::ProcessLineOfSight(Source, TargetCoors, colPoint, entity, true, false, false, true, false, true, true);
		Source = colPoint.point;
	}else{
		// inlined
		WellBufferMe(TargetWhenChecksWereOn, &Beta, &BetaSpeed, 0.07f, 0.015f, true);

		s = Sin(Beta);
		c = Cos(Beta);
		Source = TheCamera.CarZoomValueSmooth * CVector(-c, -s, 0.0f) +
			(TheCamera.CarZoomValueSmooth+7.0f) * CVector(-c, -s, 0.0f) +
			TargetCoors;
		Source.z = WaterLevel + TheCamera.CarZoomValueSmooth;
	}

	if(TheCamera.CarZoomValueSmooth < 0.05f){
		static float AmountUp = 2.2f;
		TargetCoors.z += AmountUp * (0.0f - TheCamera.CarZoomValueSmooth);
	}
	TargetCoors.z += TheCamera.CarZoomValueSmooth + 0.5f;
	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	Front = TargetCoors - Source;
	GetVectorsReadyForRW();
	ResetStatics = false;
}

void
CCam::Process_Fight_Cam(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	if(!CamTargetEntity->IsPed())
		return;

	FOV = DefaultFOV;
	float BetaLeft, BetaRight, DeltaBetaLeft, DeltaBetaRight;
	float BetaFix;
	float Dist;
	float BetaMaxSpeed = 0.015f;
	float BetaAcceleration = 0.007f;
	static bool PreviouslyFailedBuildingChecks = false;
	float TargetCamHeight;
	CVector TargetCoors;

	m_fMinDistAwayFromCamWhenInterPolating = 4.0f;
	Front = Source - CameraTarget;
	Beta = CGeneral::GetATanOfXY(Front.x, Front.y);
	while(TargetOrientation >= PI) TargetOrientation -= 2*PI;
	while(TargetOrientation < -PI) TargetOrientation += 2*PI;
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;

	// Figure out Beta
	BetaLeft = TargetOrientation - HALFPI;
	BetaRight = TargetOrientation + HALFPI;
	DeltaBetaLeft = Beta - BetaLeft;
	DeltaBetaRight = Beta - BetaRight;
	while(DeltaBetaLeft >= PI) DeltaBetaLeft -= 2*PI;
	while(DeltaBetaLeft < -PI) DeltaBetaLeft += 2*PI;
	while(DeltaBetaRight >= PI) DeltaBetaRight -= 2*PI;
	while(DeltaBetaRight < -PI) DeltaBetaRight += 2*PI;

	if(ResetStatics){
		if(Abs(DeltaBetaLeft) < Abs(DeltaBetaRight))
			m_fTargetBeta = DeltaBetaLeft;
		else
			m_fTargetBeta = DeltaBetaRight;
		m_fBufferedTargetOrientation = TargetOrientation;
		m_fBufferedTargetOrientationSpeed = 0.0f;
		m_bCollisionChecksOn = true;
		BetaSpeed = 0.0f;
	}else if(CPad::GetPad(0)->WeaponJustDown()){
		if(Abs(DeltaBetaLeft) < Abs(DeltaBetaRight))
			m_fTargetBeta = DeltaBetaLeft;
		else
			m_fTargetBeta = DeltaBetaRight;
	}

	// Check collisions
	BetaFix = 0.0f;
	Dist = Front.Magnitude2D();
	if(m_bCollisionChecksOn || PreviouslyFailedBuildingChecks){
		BetaFix = GetPedBetaAngleForClearView(CameraTarget, Dist+0.25f, 0.0f, true, false, false, true, false);
		if(BetaFix == 0.0f){
			BetaFix = GetPedBetaAngleForClearView(CameraTarget, Dist+0.5f, DEGTORAD(24.0f), true, false, false, true, false);
			if(BetaFix == 0.0f)
				BetaFix = GetPedBetaAngleForClearView(CameraTarget, Dist+0.5f, -DEGTORAD(24.0f), true, false, false, true, false);
		}
	}
	if(BetaFix != 0.0f){
		BetaMaxSpeed = 0.1f;
		PreviouslyFailedBuildingChecks = true;
		BetaAcceleration = 0.025f;
		m_fTargetBeta = Beta + BetaFix;
	}
	WellBufferMe(m_fTargetBeta, &Beta, &BetaSpeed, BetaMaxSpeed, BetaAcceleration, true);

	Source = CameraTarget + 4.0f*CVector(Cos(Beta), Sin(Beta), 0.0f);
	Source.z -= 0.5f;

	WellBufferMe(TargetOrientation, &m_fBufferedTargetOrientation, &m_fBufferedTargetOrientationSpeed, 0.07f, 0.004f, true);
	TargetCoors = CameraTarget + 0.5f*CVector(Cos(m_fBufferedTargetOrientation), Sin(m_fBufferedTargetOrientation), 0.0f);

	TargetCamHeight = CameraTarget.z - Source.z + max(m_fPedBetweenCameraHeightOffset, m_fRoadOffSet + m_fDimensionOfHighestNearCar) - 0.5f;
	if(TargetCamHeight > m_fCamBufferedHeight)
		WellBufferMe(TargetCamHeight, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.15f, 0.04f, false);
	else
		WellBufferMe(0.0f, &m_fCamBufferedHeight, &m_fCamBufferedHeightSpeed, 0.08f, 0.0175f, false);
	Source.z += m_fCamBufferedHeight;

	m_cvecTargetCoorsForFudgeInter = TargetCoors;
	Front = TargetCoors - Source;
	Front.Normalise();
	GetVectorsReadyForRW();

	ResetStatics = false;
}

/*
// Spline format is this, but game doesn't seem to use any kind of struct:
struct Spline
{
	float numFrames;
	struct {
		float time;
		float f[3];	// CVector for Vector spline
	} frames[1];	// numFrames
};
*/

// These two functions are pretty ugly

#define MS(t) (uint32)((t)*1000.0f)

void
FindSplinePathPositionFloat(float *out, float *spline, uint32 time, uint32 &marker)
{
	// marker is at time
	uint32 numFrames = spline[0];
	uint32 timeDelta = MS(spline[marker] - spline[marker-4]);
	uint32 endTime = MS(spline[4*(numFrames-1) + 1]);
	if(time < endTime){
		bool canAdvance = true;
		if((marker-1)/4 > numFrames){
			canAdvance = false;
			marker = 4*(numFrames-1) + 1;
		}
		// skipping over small time deltas apparently?
		while(timeDelta <= 75 && canAdvance){
			marker += 4;
			if((marker-1)/4 > numFrames){
				canAdvance = false;
				marker = 4*(numFrames-1) + 1;
			}
			timeDelta = (spline[marker] - spline[marker-4]) * 1000.0f;
		}
	}
	float a = ((float)time - (float)MS(spline[marker-4])) / (float)MS(spline[marker] - spline[marker-4]);
	a = clamp(a, 0.0f, 1.0f);
	float b = 1.0f - a;
	*out =	b*b*b * spline[marker-3] +
		3.0f*a*b*b * spline[marker-1] +
		3.0f*a*a*b * spline[marker+2] +
		a*a*a * spline[marker+1];
}

void
FindSplinePathPositionVector(CVector *out, float *spline, uint32 time, uint32 &marker)
{
	// marker is at time
	uint32 numFrames = spline[0];
	uint32 timeDelta = MS(spline[marker] - spline[marker-10]);
	uint32 endTime = MS(spline[10*(numFrames-1) + 1]);
	if(time < endTime){
		bool canAdvance = true;
		if((marker-1)/10 > numFrames){
			canAdvance = false;
			marker = 10*(numFrames-1) + 1;
		}
		// skipping over small time deltas apparently?
		while(timeDelta <= 75 && canAdvance){
			marker += 10;
			if((marker-1)/10 > numFrames){
				canAdvance = false;
				marker = 10*(numFrames-1) + 1;
			}
			timeDelta = (spline[marker] - spline[marker-10]) * 1000.0f;
		}
	}

	if((marker-1)/10 > numFrames){
		printf("Arraymarker %i \n", marker);
		printf("Path zero %i \n", numFrames);
	}

	float a = ((float)time - (float)MS(spline[marker-10])) / (float)MS(spline[marker] - spline[marker-10]);
	a = clamp(a, 0.0f, 1.0f);
	float b = 1.0f - a;
	out->x =
		b*b*b * spline[marker-9] +
		3.0f*a*b*b * spline[marker-3] +
		3.0f*a*a*b * spline[marker+4] +
		a*a*a * spline[marker+1];
	out->y =
		b*b*b * spline[marker-8] +
		3.0f*a*b*b * spline[marker-2] +
		3.0f*a*a*b * spline[marker+5] +
		a*a*a * spline[marker+2];
	out->z =
		b*b*b * spline[marker-7] +
		3.0f*a*b*b * spline[marker-1] +
		3.0f*a*a*b * spline[marker+6] +
		a*a*a * spline[marker+3];
	*out += TheCamera.m_vecCutSceneOffset;
}

void
CCam::Process_FlyBy(const CVector&, float, float, float)
{
	float UpAngle = 0.0f;
	static float FirstFOVValue = 0.0f;
	static float PsuedoFOV;
	static uint32 ArrayMarkerFOV;
	static uint32 ArrayMarkerUp;
	static uint32 ArrayMarkerSource;
	static uint32 ArrayMarkerFront;

	if(TheCamera.m_bcutsceneFinished)
		return;

	Up = CVector(0.0f, 0.0f, 1.0f);
	if(TheCamera.m_bStartingSpline)
		m_fTimeElapsedFloat += CTimer::GetTimeStepInMilliseconds();
	else{
		m_fTimeElapsedFloat = 0.0f;
		m_uiFinishTime = MS(TheCamera.m_arrPathArray[2].m_arr_PathData[10*((int)TheCamera.m_arrPathArray[2].m_arr_PathData[0]-1) + 1]);
		TheCamera.m_bStartingSpline = true;
		FirstFOVValue = TheCamera.m_arrPathArray[0].m_arr_PathData[2];
		PsuedoFOV = TheCamera.m_arrPathArray[0].m_arr_PathData[2];
		ArrayMarkerFOV = 5;
		ArrayMarkerUp = 5;
		ArrayMarkerSource = 11;
		ArrayMarkerFront = 11;
	}

	float fTime = m_fTimeElapsedFloat;
	uint32 uiFinishTime = m_uiFinishTime;
	uint32 uiTime = fTime;
	if(uiTime < uiFinishTime){
		TheCamera.m_fPositionAlongSpline = (float) uiTime / uiFinishTime;

		while(uiTime >= (TheCamera.m_arrPathArray[2].m_arr_PathData[ArrayMarkerSource] - TheCamera.m_arrPathArray[2].m_arr_PathData[1])*1000.0f)
			ArrayMarkerSource += 10;
		FindSplinePathPositionVector(&Source, TheCamera.m_arrPathArray[2].m_arr_PathData, uiTime, ArrayMarkerSource);

		while(uiTime >= (TheCamera.m_arrPathArray[3].m_arr_PathData[ArrayMarkerFront] - TheCamera.m_arrPathArray[3].m_arr_PathData[1])*1000.0f)
			ArrayMarkerFront += 10;
		FindSplinePathPositionVector(&Front, TheCamera.m_arrPathArray[3].m_arr_PathData, uiTime, ArrayMarkerFront);

		while(uiTime >= (TheCamera.m_arrPathArray[1].m_arr_PathData[ArrayMarkerUp] - TheCamera.m_arrPathArray[1].m_arr_PathData[1])*1000.0f)
			ArrayMarkerUp += 4;
		FindSplinePathPositionFloat(&UpAngle, TheCamera.m_arrPathArray[1].m_arr_PathData, uiTime, ArrayMarkerUp);
		UpAngle = DEGTORAD(UpAngle) + HALFPI;
		Up.x = Cos(UpAngle);
		Up.z = Sin(UpAngle);

		while(uiTime >= (TheCamera.m_arrPathArray[0].m_arr_PathData[ArrayMarkerFOV] - TheCamera.m_arrPathArray[0].m_arr_PathData[1])*1000.0f)
			ArrayMarkerFOV += 4;
		FindSplinePathPositionFloat(&PsuedoFOV, TheCamera.m_arrPathArray[0].m_arr_PathData, uiTime, ArrayMarkerFOV);

		m_cvecTargetCoorsForFudgeInter = Front;
		Front = Front - Source;
		Front.Normalise();
		CVector Left = CrossProduct(Up, Front);
		Up = CrossProduct(Front, Left);
		Up.Normalise();
		FOV = PsuedoFOV;
	}else{
		// end
		ArrayMarkerSource = (TheCamera.m_arrPathArray[2].m_arr_PathData[0] - 1)*10 + 1;
		ArrayMarkerFront = (TheCamera.m_arrPathArray[3].m_arr_PathData[0] - 1)*10 + 1;
		ArrayMarkerUp = (TheCamera.m_arrPathArray[1].m_arr_PathData[0] - 1)*4 + 1;
		ArrayMarkerFOV = (TheCamera.m_arrPathArray[0].m_arr_PathData[0] - 1)*4 + 1;

		FindSplinePathPositionVector(&Source, TheCamera.m_arrPathArray[2].m_arr_PathData, uiTime, ArrayMarkerSource);
		FindSplinePathPositionVector(&Front, TheCamera.m_arrPathArray[3].m_arr_PathData, uiTime, ArrayMarkerFront);
		FindSplinePathPositionFloat(&UpAngle, TheCamera.m_arrPathArray[1].m_arr_PathData, uiTime, ArrayMarkerUp);
		UpAngle = DEGTORAD(UpAngle) + HALFPI;
		Up.x = Cos(UpAngle);
		Up.z = Sin(UpAngle);
		FindSplinePathPositionFloat(&PsuedoFOV, TheCamera.m_arrPathArray[0].m_arr_PathData, uiTime, ArrayMarkerFOV);

		TheCamera.m_fPositionAlongSpline = 1.0f;
		ArrayMarkerFOV = 0;
		ArrayMarkerUp = 0;
		ArrayMarkerSource = 0;
		ArrayMarkerFront = 0;

		m_cvecTargetCoorsForFudgeInter = Front;
		Front = Front - Source;
		Front.Normalise();
		CVector Left = CrossProduct(Up, Front);
		Up = CrossProduct(Front, Left);
		Up.Normalise();
		FOV = PsuedoFOV;
	}
}

void
CCam::Process_WheelCam(const CVector&, float, float, float)
{
	FOV = DefaultFOV;

	if(CamTargetEntity->IsPed()){
		// what? ped with wheels or what?
		Source = Multiply3x3(CamTargetEntity->GetMatrix(), CVector(-0.3f, -0.5f, 0.1f));
		Source += CamTargetEntity->GetPosition();
		Front = CVector(1.0f, 0.0f, 0.0f);
	}else{
		Source = Multiply3x3(CamTargetEntity->GetMatrix(), CVector(-1.4f, -2.3f, 0.3f));
		Source += CamTargetEntity->GetPosition();
		Front = CamTargetEntity->GetForward();
	}

	CVector NewUp(0.0f, 0.0f, 1.0f);
	CVector Left = CrossProduct(Front, NewUp);
	Left.Normalise();
	NewUp = CrossProduct(Left, Front);

	float Roll = Cos((CTimer::GetTimeInMilliseconds()&0x1FFFF)/(float)0x1FFFF * TWOPI);
	Up = Cos(Roll*0.4f)*NewUp + Sin(Roll*0.4f)*Left;
}

void
CCam::Process_Fixed(const CVector &CameraTarget, float, float, float)
{
	Source = m_cvecCamFixedModeSource;
	Front = CameraTarget - Source;
	m_cvecTargetCoorsForFudgeInter = CameraTarget;
	GetVectorsReadyForRW();

	Up = CVector(0.0f, 0.0f, 1.0f) + m_cvecCamFixedModeUpOffSet;
	Up.Normalise();
	CVector Right = CrossProduct(Front, Up);
	Right.Normalise();
	Up = CrossProduct(Right, Front);

	FOV = DefaultFOV;
	if(TheCamera.m_bUseSpecialFovTrain)
		FOV = TheCamera.m_fFovForTrain;

	if(CMenuManager::m_ControlMethod == 0 && Using3rdPersonMouseCam()){
		CPed *player = FindPlayerPed();
		if(player && player->CanStrafeOrMouseControl()){
			float Heading = Front.Heading();
			((CPed*)TheCamera.pTargetEntity)->m_fRotationCur = Heading;
			((CPed*)TheCamera.pTargetEntity)->m_fRotationDest = Heading;
			TheCamera.pTargetEntity->SetHeading(Heading);
			TheCamera.pTargetEntity->GetMatrix().UpdateRW();
		}
	}
}

void
CCam::Process_Player_Fallen_Water(const CVector &CameraTarget, float TargetOrientation, float, float)
{
	CColPoint colPoint;
	CEntity *entity = nil;

	FOV = DefaultFOV;
	Source = CameraTarget;
	Source.x += -4.5f*Cos(TargetOrientation);
	Source.y += -4.5f*Sin(TargetOrientation);
	Source.z = m_vecLastAboveWaterCamPosition.z + 4.0f;

	m_cvecTargetCoorsForFudgeInter = CameraTarget;
	Front = CameraTarget - Source;
	Front.Normalise();
	if(CWorld::ProcessLineOfSight(CameraTarget, Source, colPoint, entity, true, false, false, true, false, true, true))
		Source = colPoint.point;
	GetVectorsReadyForRW();
	Front = CameraTarget - Source;
	Front.Normalise();
}

// unused
void
CCam::Process_Circle(const CVector &CameraTarget, float, float, float)
{
	FOV = DefaultFOV;

	Front.x =  Cos(0.7f) * Cos((CTimer::GetTimeInMilliseconds()&0xFFF)/(float)0xFFF * TWOPI);
	Front.y =  Cos(0.7f) * Sin((CTimer::GetTimeInMilliseconds()&0xFFF)/(float)0xFFF * TWOPI);
	Front.z = -Sin(0.7f);
	Source = CameraTarget - 4.0f*Front;
	Source.z += 1.0f;
	GetVectorsReadyForRW();
}

void
CCam::Process_SpecialFixedForSyphon(const CVector &CameraTarget, float, float, float)
{
	Source = m_cvecCamFixedModeSource;
	m_cvecTargetCoorsForFudgeInter = CameraTarget;
	m_cvecTargetCoorsForFudgeInter.z += m_fSyphonModeTargetZOffSet;
	Front = CameraTarget - Source;
	Front.z += m_fSyphonModeTargetZOffSet;

	GetVectorsReadyForRW();

	Up += m_cvecCamFixedModeUpOffSet;
	Up.Normalise();
	CVector Left = CrossProduct(Up, Front);
	Left.Normalise();
	Front = CrossProduct(Left, Up);
	Front.Normalise();
	FOV = DefaultFOV;
}

#ifdef IMPROVED_CAMERA

#define KEYJUSTDOWN(k) ControlsManager.GetIsKeyboardKeyJustDown((RsKeyCodes)k)
#define KEYDOWN(k) ControlsManager.GetIsKeyboardKeyDown((RsKeyCodes)k)
#define CTRLJUSTDOWN(key) \
	       ((KEYDOWN(rsLCTRL) || KEYDOWN(rsRCTRL)) && KEYJUSTDOWN((RsKeyCodes)key) || \
	        (KEYJUSTDOWN(rsLCTRL) || KEYJUSTDOWN(rsRCTRL)) && KEYDOWN((RsKeyCodes)key))
#define CTRLDOWN(key) ((KEYDOWN(rsLCTRL) || KEYDOWN(rsRCTRL)) && KEYDOWN((RsKeyCodes)key))


void
CCam::Process_Debug(const CVector&, float, float, float)
{
	static float Speed = 0.0f;
	static float PanSpeedX = 0.0f;
	static float PanSpeedY = 0.0f;
	CVector TargetCoors;

	RwCameraSetNearClipPlane(Scene.camera, 0.9f);
	FOV = DefaultFOV;
	Alpha += DEGTORAD(CPad::GetPad(1)->GetLeftStickY()) / 50.0f;
	Beta  += DEGTORAD(CPad::GetPad(1)->GetLeftStickX())*1.5f / 19.0f;
	if(CPad::GetPad(0)->GetLeftMouse()){
		Alpha += DEGTORAD(CPad::GetPad(0)->GetMouseY()/2.0f);
		Beta -= DEGTORAD(CPad::GetPad(0)->GetMouseX()/2.0f);
	}

	TargetCoors.x = Source.x + Cos(Alpha) * Cos(Beta) * 3.0f;
	TargetCoors.y = Source.y + Cos(Alpha) * Sin(Beta) * 3.0f;
	TargetCoors.z = Source.z + Sin(Alpha) * 3.0f;

	if(Alpha > DEGTORAD(89.5f)) Alpha = DEGTORAD(89.5f);
	if(Alpha < DEGTORAD(-89.5f)) Alpha = DEGTORAD(-89.5f);

	if(CPad::GetPad(1)->GetSquare() || KEYDOWN('W'))
		Speed += 0.1f;
	else if(CPad::GetPad(1)->GetCross() || KEYDOWN('S'))
		Speed -= 0.1f;
	else
		Speed = 0.0f;
	if(Speed > 70.0f) Speed = 70.0f;
	if(Speed < -70.0f) Speed = -70.0f;


	if(KEYDOWN(rsRIGHT) || KEYDOWN('D'))
		PanSpeedX += 0.1f;
	else if(KEYDOWN(rsLEFT) || KEYDOWN('A'))
		PanSpeedX -= 0.1f;
	else
		PanSpeedX = 0.0f;
	if(PanSpeedX > 70.0f) PanSpeedX = 70.0f;
	if(PanSpeedX < -70.0f) PanSpeedX = -70.0f;


	if(KEYDOWN(rsUP))
		PanSpeedY += 0.1f;
	else if(KEYDOWN(rsDOWN))
		PanSpeedY -= 0.1f;
	else
		PanSpeedY = 0.0f;
	if(PanSpeedY > 70.0f) PanSpeedY = 70.0f;
	if(PanSpeedY < -70.0f) PanSpeedY = -70.0f;


	Front = TargetCoors - Source;
	Front.Normalise();
	Source = Source + Front*Speed;

	Up = CVector{ 0.0f, 0.0f, 1.0f };
	CVector Right = CrossProduct(Front, Up);
	Up = CrossProduct(Right, Front);
	Source = Source + Up*PanSpeedY + Right*PanSpeedX;

	if(Source.z < -450.0f)
		Source.z = -450.0f;

	if(CPad::GetPad(1)->GetRightShoulder2JustDown() || KEYJUSTDOWN(rsENTER)){
		if(FindPlayerVehicle())
			FindPlayerVehicle()->Teleport(Source);
		else
			CWorld::Players[CWorld::PlayerInFocus].m_pPed->GetPosition() = Source;	
	}

	// stay inside sectors
	while(CWorld::GetSectorX(Source.x) > 95.0f)
		Source.x -= 1.0f;
	while(CWorld::GetSectorX(Source.x) < 5.0f)
		Source.x += 1.0f;
	while(CWorld::GetSectorY(Source.y) > 95.0f)
		Source.y -= 1.0f;
	while(CWorld::GetSectorY(Source.y) < 5.0f)
		Source.y += 1.0f;
	GetVectorsReadyForRW();

	CPad::GetPad(0)->DisablePlayerControls = PLAYERCONTROL_DISABLED_1;

	if(CPad::GetPad(1)->GetLeftShockJustDown() && gbBigWhiteDebugLightSwitchedOn)
		CShadows::StoreShadowToBeRendered(SHADOWTYPE_ADDITIVE, gpShadowExplosionTex, &Source,
			12.0f, 0.0f, 0.0f, -12.0f,
			128, 128, 128, 128, 1000.0f, false, 1.0f);

	if(CHud::m_Wants_To_Draw_Hud){
		char str[256];
		sprintf(str, "CamX: %f CamY: %f  CamZ:  %f", Source.x, Source.y, Source.z);
		sprintf(str, "Frontx: %f, Fronty: %f, Frontz: %f ", Front.x, Front.y, Front.z);
		sprintf(str, "Look@: %f, Look@: %f, Look@: %f ", Front.x + Source.x, Front.y + Source.y, Front.z + Source.z);
	}
}
#else
void
CCam::Process_Debug(const CVector&, float, float, float)
{
	static float Speed = 0.0f;
	CVector TargetCoors;

	RwCameraSetNearClipPlane(Scene.camera, 0.9f);
	FOV = DefaultFOV;
	Alpha += DEGTORAD(CPad::GetPad(1)->GetLeftStickY()) / 50.0f;
	Beta  += DEGTORAD(CPad::GetPad(1)->GetLeftStickX())*1.5f / 19.0f;

	TargetCoors.x = Source.x + Cos(Alpha) * Cos(Beta) * 3.0f;
	TargetCoors.y = Source.y + Cos(Alpha) * Sin(Beta) * 3.0f;
	TargetCoors.z = Source.z + Sin(Alpha) * 3.0f;

	if(Alpha > DEGTORAD(89.5f)) Alpha = DEGTORAD(89.5f);
	if(Alpha < DEGTORAD(-89.5f)) Alpha = DEGTORAD(-89.5f);

	if(CPad::GetPad(1)->GetSquare() || CPad::GetPad(1)->GetLeftMouse())
		Speed += 0.1f;
	else if(CPad::GetPad(1)->GetCross() || CPad::GetPad(1)->GetRightMouse())
		Speed -= 0.1f;
	else
		Speed = 0.0f;
	if(Speed > 70.0f) Speed = 70.0f;
	if(Speed < -70.0f) Speed = -70.0f;

	Front = TargetCoors - Source;
	Front.Normalise();
	Source = Source + Front*Speed;

	if(Source.z < -450.0f)
		Source.z = -450.0f;

	if(CPad::GetPad(1)->GetRightShoulder2JustDown()){
		if(FindPlayerVehicle())
			FindPlayerVehicle()->Teleport(Source);
		else
			CWorld::Players[CWorld::PlayerInFocus].m_pPed->GetPosition() = Source;
			
	}

	// stay inside sectors
	while(CWorld::GetSectorX(Source.x) > 95.0f)
		Source.x -= 1.0f;
	while(CWorld::GetSectorX(Source.x) < 5.0f)
		Source.x += 1.0f;
	while(CWorld::GetSectorY(Source.y) > 95.0f)
		Source.y -= 1.0f;
	while(CWorld::GetSectorY(Source.y) < 5.0f)
		Source.y += 1.0f;
	GetVectorsReadyForRW();

	if(CPad::GetPad(1)->GetLeftShockJustDown() && gbBigWhiteDebugLightSwitchedOn)
		CShadows::StoreShadowToBeRendered(SHADOWTYPE_ADDITIVE, gpShadowExplosionTex, &Source,
			12.0f, 0.0f, 0.0f, -12.0f,
			128, 128, 128, 128, 1000.0f, false, 1.0f);

	if(CHud::m_Wants_To_Draw_Hud){
		char str[256];
		sprintf(str, "CamX: %f CamY: %f  CamZ:  %f", Source.x, Source.y, Source.z);
		sprintf(str, "Frontx: %f, Fronty: %f, Frontz: %f ", Front.x, Front.y, Front.z);
		sprintf(str, "Look@: %f, Look@: %f, Look@: %f ", Front.x + Source.x, Front.y + Source.y, Front.z + Source.z);
	}
}
#endif

void
CCam::Process_Editor(const CVector&, float, float, float)
{
	static float Speed = 0.0f;
	CVector TargetCoors;

	if(ResetStatics){
		Source = CVector(796.0f, -937.0, 40.0f);
		CamTargetEntity = nil;
	}
	ResetStatics = false;

	RwCameraSetNearClipPlane(Scene.camera, 0.9f);
	FOV = DefaultFOV;
	Alpha += DEGTORAD(CPad::GetPad(1)->GetLeftStickY()) / 50.0f;
	Beta  += DEGTORAD(CPad::GetPad(1)->GetLeftStickX())*1.5f / 19.0f;

	if(CamTargetEntity && CSceneEdit::m_bCameraFollowActor){
		TargetCoors = CamTargetEntity->GetPosition();
	}else if(CSceneEdit::m_bRecording){
		TargetCoors.x = Source.x + Cos(Alpha) * Cos(Beta) * 7.0f;
		TargetCoors.y = Source.y + Cos(Alpha) * Sin(Beta) * 7.0f;
		TargetCoors.z = Source.z + Sin(Alpha) * 7.0f;
	}else
		TargetCoors = CSceneEdit::m_vecCamHeading + Source;
	CSceneEdit::m_vecCurrentPosition = TargetCoors;
	CSceneEdit::m_vecCamHeading = TargetCoors - Source;

	if(Alpha > DEGTORAD(89.5f)) Alpha = DEGTORAD(89.5f);
	if(Alpha < DEGTORAD(-89.5f)) Alpha = DEGTORAD(-89.5f);

	if(CPad::GetPad(1)->GetSquare() || CPad::GetPad(1)->GetLeftMouse())
		Speed += 0.1f;
	else if(CPad::GetPad(1)->GetCross() || CPad::GetPad(1)->GetRightMouse())
		Speed -= 0.1f;
	else
		Speed = 0.0f;
	if(Speed > 70.0f) Speed = 70.0f;
	if(Speed < -70.0f) Speed = -70.0f;

	Front = TargetCoors - Source;
	Front.Normalise();
	Source = Source + Front*Speed;

	if(Source.z < -450.0f)
		Source.z = -450.0f;

	if(CPad::GetPad(1)->GetRightShoulder2JustDown()){
		if(FindPlayerVehicle())
			FindPlayerVehicle()->Teleport(Source);
		else
			CWorld::Players[CWorld::PlayerInFocus].m_pPed->GetPosition() = Source;
			
	}

	// stay inside sectors
	while(CWorld::GetSectorX(Source.x) > 95.0f)
		Source.x -= 1.0f;
	while(CWorld::GetSectorX(Source.x) < 5.0f)
		Source.x += 1.0f;
	while(CWorld::GetSectorY(Source.y) > 95.0f)
		Source.y -= 1.0f;
	while(CWorld::GetSectorY(Source.y) < 5.0f)
		Source.y += 1.0f;
	GetVectorsReadyForRW();

	if(CPad::GetPad(1)->GetLeftShockJustDown() && gbBigWhiteDebugLightSwitchedOn)
		CShadows::StoreShadowToBeRendered(SHADOWTYPE_ADDITIVE, gpShadowExplosionTex, &Source,
			12.0f, 0.0f, 0.0f, -12.0f,
			128, 128, 128, 128, 1000.0f, false, 1.0f);

	if(CHud::m_Wants_To_Draw_Hud){
		char str[256];
		sprintf(str, "CamX: %f CamY: %f  CamZ:  %f", Source.x, Source.y, Source.z);
		sprintf(str, "Frontx: %f, Fronty: %f, Frontz: %f ", Front.x, Front.y, Front.z);
		sprintf(str, "Look@: %f, Look@: %f, Look@: %f ", Front.x + Source.x, Front.y + Source.y, Front.z + Source.z);
	}
}

void
CCam::Process_ModelView(const CVector &CameraTarget, float, float, float)
{
	CVector TargetCoors = CameraTarget;
	float Angle = Atan2(Front.x, Front.y);
	FOV = DefaultFOV;

	Angle += CPad::GetPad(0)->GetLeftStickX()/1280.0f;
	if(Distance < 10.0f)
		Distance += CPad::GetPad(0)->GetLeftStickY()/1000.0f;
	else
		Distance += CPad::GetPad(0)->GetLeftStickY() * ((Distance - 10.0f)/20.0f + 1.0f) / 1000.0f;
	if(Distance < 1.5f)
		Distance = 1.5f;

	Front.x = Cos(0.3f) * Sin(Angle);
	Front.y = Cos(0.3f) * Cos(Angle);
	Front.z = -Sin(0.3f);
	Source = CameraTarget - Distance*Front;

	GetVectorsReadyForRW();
}

void
CCam::ProcessPedsDeadBaby(void)
{
	float Distance = 0.0f;
	static bool SafeToRotate = false;
	CVector TargetDist, TestPoint;

	FOV = DefaultFOV;
	TargetDist = Source - CamTargetEntity->GetPosition();
	Distance = TargetDist.Magnitude();
	Beta = CGeneral::GetATanOfXY(TargetDist.x, TargetDist.y);
	while(Beta >= PI) Beta -= 2*PI;
	while(Beta < -PI) Beta += 2*PI;

	if(ResetStatics){
		TestPoint = CamTargetEntity->GetPosition() +
			CVector(4.0f * Cos(Alpha) * Cos(Beta),
			        4.0f * Cos(Alpha) * Sin(Beta),
			        4.0f * Sin(Alpha));
		bool Safe1 = CWorld::GetIsLineOfSightClear(TestPoint, CamTargetEntity->GetPosition(), true, false, false, true, false, true, true);

		TestPoint = CamTargetEntity->GetPosition() +
			CVector(4.0f * Cos(Alpha) * Cos(Beta + DEGTORAD(120.0f)),
			        4.0f * Cos(Alpha) * Sin(Beta + DEGTORAD(120.0f)),
			        4.0f * Sin(Alpha));
		bool Safe2 = CWorld::GetIsLineOfSightClear(TestPoint, CamTargetEntity->GetPosition(), true, false, false, true, false, true, true);

		TestPoint = CamTargetEntity->GetPosition() +
			CVector(4.0f * Cos(Alpha) * Cos(Beta - DEGTORAD(120.0f)),
			        4.0f * Cos(Alpha) * Sin(Beta - DEGTORAD(120.0f)),
			        4.0f * Sin(Alpha));
		bool Safe3 = CWorld::GetIsLineOfSightClear(TestPoint, CamTargetEntity->GetPosition(), true, false, false, true, false, true, true);

		SafeToRotate = Safe1 && Safe2 && Safe3;

		ResetStatics = false;
	}

	if(SafeToRotate)
		WellBufferMe(Beta + DEGTORAD(175.0f), &Beta, &BetaSpeed, 0.015f, 0.007f, true);

	WellBufferMe(DEGTORAD(89.5f), &Alpha, &AlphaSpeed, 0.015f, 0.07f, true);
	WellBufferMe(35.0f, &Distance, &DistanceSpeed, 0.006f, 0.007f, false);

	Source = CamTargetEntity->GetPosition() +
		CVector(Distance * Cos(Alpha) * Cos(Beta),
		        Distance * Cos(Alpha) * Sin(Beta),
		        Distance * Sin(Alpha));
	m_cvecTargetCoorsForFudgeInter = CamTargetEntity->GetPosition();
	Front = CamTargetEntity->GetPosition() - Source;
	Front.Normalise();
	GetVectorsReadyForRW();
}

bool
CCam::ProcessArrestCamOne(void)
{
	FOV = 45.0f;
	if(ResetStatics)
		return true;

#ifdef FIX_BUGS
	if(!CamTargetEntity->IsPed())
		return true;
#endif

	bool found;
	float Ground;
	CVector PlayerCoors = TheCamera.pTargetEntity->GetPosition();
	CVector CopCoors = ((CPlayerPed*)TheCamera.pTargetEntity)->m_pArrestingCop->GetPosition();
	Beta = CGeneral::GetATanOfXY(PlayerCoors.x - CopCoors.x, PlayerCoors.y - CopCoors.y);

	Source = PlayerCoors + 9.5f*CVector(Cos(Beta), Sin(Beta), 0.0f);
	Source.z += 6.0f;
	Ground = CWorld::FindGroundZFor3DCoord(Source.x, Source.y, Source.z, &found);
	if(!found){
		Ground = CWorld::FindRoofZFor3DCoord(Source.x, Source.y, Source.z, &found);
		if(!found)
			return false;
	}
	Source.z = Ground + 0.25f;
	if(!CWorld::GetIsLineOfSightClear(Source, CopCoors, true, true, false, true, false, true, true)){
		Beta += DEGTORAD(115.0f);
		Source = PlayerCoors + 9.5f*CVector(Cos(Beta), Sin(Beta), 0.0f);
		Source.z += 6.0f;
		Ground = CWorld::FindGroundZFor3DCoord(Source.x, Source.y, Source.z, &found);
		if(!found){
			Ground = CWorld::FindRoofZFor3DCoord(Source.x, Source.y, Source.z, &found);
			if(!found)
				return false;
		}
		Source.z = Ground + 0.25f;

		CopCoors.z += 0.35f;
		Front = CopCoors - Source;
		if(!CWorld::GetIsLineOfSightClear(Source, CopCoors, true, true, false, true, false, true, true))
			return false;
	}
	CopCoors.z += 0.35f;
	m_cvecTargetCoorsForFudgeInter = CopCoors;
	Front = CopCoors - Source;
	ResetStatics = false;
	GetVectorsReadyForRW();
	return true;
}

bool
CCam::ProcessArrestCamTwo(void)
{
	CPed *player = CWorld::Players[CWorld::PlayerInFocus].m_pPed;
	if(!ResetStatics)
		return true;
	ResetStatics = false;

	CVector TargetCoors, ToCamera;
	float BetaOffset;
	float SourceX, SourceY;
	CCam *ActiveCam = &TheCamera.Cams[TheCamera.ActiveCam];
	if(&ActiveCam[1] == this){
		SourceX = TheCamera.Cams[(TheCamera.ActiveCam + 1) % 2].Source.x;
		SourceY = TheCamera.Cams[(TheCamera.ActiveCam + 1) % 2].Source.y;
	}else{
		SourceX = ActiveCam[1].Source.x;
		SourceY = ActiveCam[1].Source.y;
	}

	for(int i = 0; i <= 1; i++){
		int Dir = i == 0 ? 1 : -1;

		TargetCoors = player->GetPosition();
		Beta = CGeneral::GetATanOfXY(TargetCoors.x-SourceX, TargetCoors.y-SourceY);
		BetaOffset = DEGTORAD(Dir*80);
		Source = TargetCoors + 11.5f*CVector(Cos(Beta+BetaOffset), Sin(Beta+BetaOffset), 0.0f);

		ToCamera = Source - TargetCoors;
		ToCamera.Normalise();
		TargetCoors.x += 0.4f*ToCamera.x;
		TargetCoors.y += 0.4f*ToCamera.y;
		if(CWorld::GetIsLineOfSightClear(Source, TargetCoors, true, true, false, true, false, true, true)){
			Source.z += 5.5f;
			TargetCoors += CVector(-0.8f*ToCamera.x, -0.8f*ToCamera.y, 2.2f);
			m_cvecTargetCoorsForFudgeInter = TargetCoors;
			Front = TargetCoors - Source;
			ResetStatics = false;
			GetVectorsReadyForRW();
			return true;
		}
	}
	return false;
}

STARTPATCHES
	InjectHook(0x456F40, WellBufferMe, PATCH_JUMP);
	InjectHook(0x458410, &CCam::Init, PATCH_JUMP);
	InjectHook(0x4582F0, &CCam::GetVectorsReadyForRW, PATCH_JUMP);
	InjectHook(0x457710, &CCam::DoAverageOnVector, PATCH_JUMP);
	InjectHook(0x458060, &CCam::GetPedBetaAngleForClearView, PATCH_JUMP);
	InjectHook(0x457210, &CCam::Cam_On_A_String_Unobscured, PATCH_JUMP);
	InjectHook(0x457A80, &CCam::FixCamWhenObscuredByVehicle, PATCH_JUMP);
	InjectHook(0x457B90, &CCam::FixCamIfObscured, PATCH_JUMP);
	InjectHook(0x465DA0, &CCam::RotCamIfInFrontCar, PATCH_JUMP);
	InjectHook(0x4662D0, &CCam::WorkOutCamHeightWeeCar, PATCH_JUMP);
	InjectHook(0x466650, &CCam::WorkOutCamHeight, PATCH_JUMP);
	InjectHook(0x458600, &CCam::LookBehind, PATCH_JUMP);
	InjectHook(0x458C40, &CCam::LookLeft, PATCH_JUMP);
	InjectHook(0x458FB0, &CCam::LookRight, PATCH_JUMP);
	InjectHook(0x4574C0, &CCam::ClipIfPedInFrontOfPlayer, PATCH_JUMP);
	InjectHook(0x459300, &CCam::KeepTrackOfTheSpeed, PATCH_JUMP);
	InjectHook(0x458580, &CCam::IsTargetInWater, PATCH_JUMP);
	InjectHook(0x4570C0, &CCam::AvoidWallsTopDownPed, PATCH_JUMP);
	InjectHook(0x4595B0, &CCam::PrintMode, PATCH_JUMP);

	InjectHook(0x467400, &CCam::ProcessSpecialHeightRoutines, PATCH_JUMP);
	InjectHook(0x4596A0, &CCam::Process, PATCH_JUMP);
	InjectHook(0x45E3A0, &CCam::Process_FollowPed, PATCH_JUMP);
	InjectHook(0x45FF70, &CCam::Process_FollowPedWithMouse, PATCH_JUMP);
	InjectHook(0x45BE60, &CCam::Process_BehindCar, PATCH_JUMP);
	InjectHook(0x45C090, &CCam::Process_Cam_On_A_String, PATCH_JUMP);
	InjectHook(0x463EB0, &CCam::Process_TopDown, PATCH_JUMP);
	InjectHook(0x464390, &CCam::Process_TopDownPed, PATCH_JUMP);
	InjectHook(0x461AF0, &CCam::Process_Rocket, PATCH_JUMP);
	InjectHook(0x460E00, &CCam::Process_M16_1stPerson, PATCH_JUMP);
	InjectHook(0x459FA0, &CCam::Process_1stPerson, PATCH_JUMP);
	InjectHook(0x462420, &CCam::Process_Sniper, PATCH_JUMP);
	InjectHook(0x463130, &CCam::Process_Syphon, PATCH_JUMP);
	InjectHook(0x463A70, &CCam::Process_Syphon_Crim_In_Front, PATCH_JUMP);
	InjectHook(0x45B470, &CCam::Process_BehindBoat, PATCH_JUMP);
	InjectHook(0x45D2F0, &CCam::Process_Fight_Cam, PATCH_JUMP);
	InjectHook(0x45DC20, &CCam::Process_FlyBy, PATCH_JUMP);
	InjectHook(0x464D10, &CCam::Process_WheelCam, PATCH_JUMP);
	InjectHook(0x45DA20, &CCam::Process_Fixed, PATCH_JUMP);
	InjectHook(0x461940, &CCam::Process_Player_Fallen_Water, PATCH_JUMP);
	InjectHook(0x45C400, &CCam::Process_Circle, PATCH_JUMP);
	InjectHook(0x462FC0, &CCam::Process_SpecialFixedForSyphon, PATCH_JUMP);
	InjectHook(0x45CCC0, &CCam::Process_Debug, PATCH_JUMP);
	InjectHook(0x4656C0, &CCam::ProcessPedsDeadBaby, PATCH_JUMP);
	InjectHook(0x465000, &CCam::ProcessArrestCamOne, PATCH_JUMP);
	InjectHook(0x4653C0, &CCam::ProcessArrestCamTwo, PATCH_JUMP);

	InjectHook(0x456CE0, &FindSplinePathPositionFloat, PATCH_JUMP);
	InjectHook(0x4569A0, &FindSplinePathPositionVector, PATCH_JUMP);

	InjectHook(0x473250, &CCamera::dtor, PATCH_JUMP);
ENDPATCHES
