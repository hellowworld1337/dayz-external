#define _CRT_SECURE_NO_WARNINGS
#define STATIC_BY_BASE(Value) (QWORD)(GameModule.ModBase + Value)

#include "Common.h"
#include "imgui/imgui.hpp"
#include "imgui/imgui_impl_win32.hpp"
#include "imgui/imgui_impl_dx11.hpp"
#include "CRC32.h"
#include "Offsets.h"
#include "Math.h"
#include "SDK.h"
#include "Core.h"
#include "Overlay/Overlay.h"
#include "set.h"
#include "kdmapper.hpp"
#include "Lolicore.h"
#undef max
#define VECTOR(Vector) Vector.X, Vector.Y, Vector.Z

/*
* class worldData
{
public:
	char pad_0000[3400]; //0x0000
	uint64_t bulletTable; //0x0D48
	uint32_t bulletTableSize; //0x0D50
	char pad_0D54[316]; //0x0D54
	uint64_t nearAnimalTable; //0x0E90
	uint32_t nearAnimalTableSize; //0x0E98
	char pad_0E9C[316]; //0x0E9C
	uint64_t farAnimalTable; //0x0FD8
	uint32_t farAnimalTableSize; //0x0FE0
	char pad_0FE4[3932]; //0x0FE4
	uint64_t slowAnimalTable; //0x1F40
	uint32_t slowAnimalTableSize; //0x1F48
	char pad_1F4C[68]; //0x1F4C
	uint64_t itemTable; //0x1F90
	uint32_t itemTableSize; //0x1F98
	char pad_1F9C[2292]; //0x1F9C
	uint64_t cameraOn; //0x2890
	char pad_2898[2032]; //0x2898
}; //Size: 0x3088
*/


// float AIM_FOV = 50.f;

struct GlobalData_t
{
	QWORD m_World = 0x0;
//	QWORD m_Camera = 0x0;
	QWORD m_NetworkClient = 0x0;
	QWORD m_WinEngine = 0x0;

	QWORD m_dwOffsetEntitySkeleton = 0x0;
	QWORD m_dwOffsetEntitySimpleSkeleton = 0x0;

	QWORD m_dwFrequencyPtr = 0x0;
};

GlobalData_t Globals = {};

void SpeedHack()
{
	static DWORD64 m_dwOriginalFrequency = Driver.Read<DWORD_PTR>(Globals.m_dwFrequencyPtr);
	if (!m_dwOriginalFrequency)
		return;

//	TRACE("[FACE] [DBG] m_dwOriginalFrequency: %d", m_dwOriginalFrequency);

	static bool m_bWasEnabled = false;

	if (GetAsyncKeyState(VK_LMENU))
	{
		float m_flSpeed = 2.f; // 1.0f -> 30.0f
		if (m_flSpeed < 1.0f)
			m_flSpeed = 1.0f;
		if (m_flSpeed > 3.0f)
			m_flSpeed = 3.0f;

		DWORD64 NewFreq = static_cast<DWORD64>((float)(m_dwOriginalFrequency) * 1.2);

		Driver.Write<DWORD_PTR>(Globals.m_dwFrequencyPtr, NewFreq);
		//*m_pFrequency = static_cast<DWORD64>((float)(m_dwOriginalFrequency) / m_flSpeed);
		TRACE("[DBG] Old: %d New: %d (Diff: %d)", m_dwOriginalFrequency, NewFreq, NewFreq - m_dwOriginalFrequency);
		m_bWasEnabled = true;
	}
	else if (m_bWasEnabled)
	{
		Driver.Write<DWORD_PTR>(Globals.m_dwFrequencyPtr, m_dwOriginalFrequency);
		m_bWasEnabled = false;
	}
}

struct EntityData_t
{
	Enfusion::CEntity* m_pEntity = nullptr;

	Math::Vector3D m_vecRootPos = {};

};

bool GetBonePosition(Enfusion::CEntity* pEntity, /*DWORD64 pVisual,*/ DWORD nIndex, Math::Vector3D* pOut, bool bSimple)
{
	void* pAnimEntity =  Driver.Read<void*>((QWORD)pEntity + (bSimple ? Globals.m_dwOffsetEntitySimpleSkeleton : Globals.m_dwOffsetEntitySkeleton));

	if (!pEntity)
		return false;

	//("[%s] m_pEntity: 0x%p\n", __FUNCTION__, m_pEntity);

	void* pAnimClass =  Driver.Read<void*>((QWORD)pAnimEntity + 0xA0); // 0xa0 8

	if (!pAnimClass)
		return false;

	//	TRACE("[%s] pAnimClass: 0x%p\n", __FUNCTION__, pAnimClass);

	//DWORD32 dwCheck =  Driver.Read<DWORD32>((QWORD)pAnimClass + (0x7E4 + nIndex * 4));

//	TRACE("[%s] Check: @ 0x%p\n", __FUNCSIG__, Check);

//	if (dwCheck == 0xFFFFFFFF)
	//	return false;

	QWORD MatrixClass =  Driver.Read<QWORD>((QWORD)pAnimClass + 0xBF0);

	if (!MatrixClass)
		return false;
	
	Math::matrix4x4 matrix_a =  Driver.Read<Math::matrix4x4>(reinterpret_cast<QWORD>(pEntity->GetRendererVisualState()) + 0x8);
	Math::Vector3D matrix_b =  Driver.Read<Math::Vector3D>(MatrixClass + 0x54 + nIndex /*realIndex*/ * sizeof(Math::matrix4x4)); //Only the last 3 of matrix needed

	pOut->X = (matrix_a.m[0] * matrix_b.X) + (matrix_a.m[3] * matrix_b.Y) + (matrix_a.m[6] * matrix_b.Z) + matrix_a.m[9];
	pOut->Y = (matrix_a.m[1] * matrix_b.X) + (matrix_a.m[4] * matrix_b.Y) + (matrix_a.m[7] * matrix_b.Z) + matrix_a.m[10];
	pOut->Z = (matrix_a.m[2] * matrix_b.X) + (matrix_a.m[5] * matrix_b.Y) + (matrix_a.m[8] * matrix_b.Z) + matrix_a.m[11];

	return true;
}
#define pow(n) (n)*(n)
__forceinline float Calc2D_Dist(const Math::Vector3D& Src, const Math::Vector3D& Dst) {
	return sqrtf(pow(Src.X - Dst.X, 2) + pow(Src.Y - Dst.Y, 2));
}
namespace Aimbotz
{
	Enfusion::CEntity* m_pTarget = nullptr;
	float m_flAimFov = std::numeric_limits<FLOAT>::max();
	Math::Vector3D m_vecAimPos = Math::Vector3D::ZeroVector;

	__forceinline float _abs(float m_flValue)
	{
		if (m_flValue >= 0.0f)
			return m_flValue;
		return -m_flValue;
	}

	float W2SDistance(Enfusion::CCamera * pCamera, Math::Vector3D vecPosition)
	{
		Math::Vector3D v = {};

		Enfusion::CWinEngine* pWinEngine = reinterpret_cast<Enfusion::CWinEngine*>(Globals.m_WinEngine);
		
		if (!Core::WorldToScreen(pCamera, vecPosition, v))
			return pWinEngine->GetRight() / 2 + (pWinEngine->GetBottom() / 2);

		// return (abs(v.X - (pWinEngine->GetRight() / 2)) + (v.Y - (pWinEngine->GetBottom() / 2)));
	//	return ( abs(pWinEngine->GetRight() / 2  - v.X) + abs(pWinEngine->GetBottom() / 2) - v.Y);
		//return (abs(pWinEngine->GetRight() / 2 + (pWinEngine->GetBottom() / 2) ) - abs(v.X + v.Y));

		return Calc2D_Dist(Math::Vector3D( pWinEngine->GetRight() / 2 , pWinEngine->GetBottom() / 2 , 0.f)  , v);
	}

	Enfusion::CEntity* TryGetTarget(Enfusion::CCamera* pCamera, Enfusion::CEntity* pEntity, Enfusion::CEntity* pLocalPlayer)
	{
		Enfusion::CEntity* pTempEntity = nullptr;

	//	if (pEntity->IsDead())
	//		return false;
		Enfusion::CEntityVisualState* pLocalVisualState = pLocalPlayer->GetRendererVisualState();

		if (!pLocalVisualState)
			return nullptr;

		Enfusion::CEntityVisualState* pVisualState = pEntity->GetRendererVisualState();

	/*	if (!pVisualState)
		{
			pVisualState = pEntity->GetFutureVisualState();
		}*/

		if (!pVisualState)
			return nullptr;

		Enfusion::CDayZPlayerType* pEntType = pEntity->GetRendererEntityType();

		if (!pEntType)
			return nullptr;

		std::string szConfigName = pEntType->GetConfigName();
	//	TRACE("[Aim] Current ent: %s", szConfigName.c_str());

		Hash::hash32_t hhConfigName = Hash::FNV1a::Get(szConfigName.c_str());

		float flDistance = pLocalVisualState->GetPosition().Distance(pVisualState->GetPosition());

		if (flDistance > /*nAimbotDistance*/ set::MagicDistance)
			return nullptr;

		if (hhConfigName == CONST_HASH("dayzplayer") /*&& bPlayerAimbot*/)
		{
			Math::Vector3D vecAimSpot = {};

			if (!GetBonePosition(pEntity, 24, &vecAimSpot, false))
				return nullptr;

			float flFov = W2SDistance(pCamera, vecAimSpot);
			if (flFov <= m_flAimFov)
			{
				m_flAimFov = flFov;
				pTempEntity = pEntity;
				m_vecAimPos = vecAimSpot;
			}
		}
		
		//if (hhConfigName == CONST_HASH("dayzinfected") /*szConfigName.find("dayzinfected") != std::string::npos *//*&& bZombieAimbot*/)
		//{
		//	Math::Vector3D vecAimSpot = {};

		//	if (!GetBonePosition(pEntity, 21, &vecAimSpot, true))
		//		return nullptr;

		//	float flFov = W2SDistance(pCamera, vecAimSpot);
		//	if (flFov <= m_flAimFov)
		//	{
		//		m_flAimFov = flFov;
		//		pTempEntity = pEntity;
		//		m_vecAimPos = vecAimSpot;
		//	}
		//}

		if (pTempEntity && m_flAimFov)
		{
			m_pTarget = pTempEntity;
	//		TRACE("[Aim] Target [0x%p] (%s) Fov: %f {%f %f %f}", pTempEntity, szConfigName.c_str(), m_flAimFov, VECTOR(m_vecAimPos));
		}

		return pTempEntity;
	}

	void Run(Enfusion::CCamera* pCamera, Enfusion::CWorld * pWorld, Enfusion::CEntity * pLocalPlayer)
	{
		m_flAimFov = set::AimFov;/*Menu.m_flFov*/ /* AIM_FOV * 10.0f;*/
		m_vecAimPos = Math::Vector3D{};
		Enfusion::CEntity* pAimEntity = nullptr;

		Enfusion::CList<Enfusion::CEntity> BulletList = { pWorld->GetBulletTablePtr(), pWorld->GetBulletTableSize() };
		Enfusion::CList<Enfusion::CEntity> NearAnimalList = { pWorld->GetNearAnimalTablePtr(), pWorld->GetNearAnimalTableSize() };
		Enfusion::CList<Enfusion::CEntity> FarAnimalList = { pWorld->GetFarAnimalTablePtr(), pWorld->GetFarAnimalTableSize() };

		uint32_t NearAnimalListSize = NearAnimalList.GetSize();
		uint32_t FarAnimalListSize = FarAnimalList.GetSize();
		uint32_t BulletListSize = BulletList.GetSize();

		for (uint32_t i = 0; i < NearAnimalListSize; i++)
		{
			Enfusion::CEntity* pEntity = NearAnimalList.Get(i);

			if (!pEntity || !Core::IsValidPointer(pEntity)) continue;

			if (pEntity == pLocalPlayer) continue;

			pAimEntity = TryGetTarget(pCamera, pEntity, pLocalPlayer);
		}

		/*if (!pAimEntity)
		{
			for (uint32_t i = 0; i < FarAnimalListSize; i++)
			{
				Enfusion::CEntity* pEntity = FarAnimalList.Get(i);

				if (!pEntity || !Core::IsValidPointer(pEntity)) continue;

				if (pEntity == pLocalPlayer) continue;

				pAimEntity = TryGetTarget(pCamera, pEntity, pLocalPlayer);
			}
		}*/

		if (pAimEntity)
		{
			for (uint32_t i = 0; i < BulletListSize; i++)
			{
				Enfusion::CEntity* pBullet = BulletList.Get(i);

				if (!pBullet || !Core::IsValidPointer(pBullet))
					continue;

				Enfusion::CEntityVisualState* pVisualState = pBullet->GetRendererVisualState();

				if (!pVisualState)
					continue;
				TRACE("[Aim] Set Bullet Pos [0x%p] {%f %f %f}", pBullet, VECTOR(m_vecAimPos));
				pVisualState->SetPosition(m_vecAimPos);



			}
		}
	}
}

void DrawBone(Overlay * pOverlay, Enfusion::CCamera* pCamera, Enfusion::CEntity* pEntity, int iStart, int iEnd, bool bSimple = false)
{
	Math::Vector3D m_vecStart = {};
	Math::Vector3D m_vecStartScreen = {};
	Math::Vector3D m_vecEnd = {};
	Math::Vector3D m_vecEndScreen = {};
	if (GetBonePosition(pEntity, iStart, &m_vecStart, bSimple) && GetBonePosition(pEntity, iEnd, &m_vecEnd, bSimple))
	{
		if (!Core::WorldToScreen(pCamera, m_vecStart, m_vecStartScreen) || !Core::WorldToScreen(pCamera, m_vecEnd, m_vecEndScreen))
			return;

	if (m_vecStartScreen.Z > 1.0f && m_vecEndScreen.Z > 1.0f)
		{
			pOverlay->Line({ m_vecStartScreen.X - 1, m_vecStartScreen.Y }, { m_vecEndScreen.X - 1, m_vecEndScreen.Y }, { 0, 0, 0, 255 });
			pOverlay->Line({ m_vecStartScreen.X + 1, m_vecStartScreen.Y }, { m_vecEndScreen.X + 1, m_vecEndScreen.Y }, { 0, 0, 0, 255 });
			pOverlay->Line({ m_vecStartScreen.X, m_vecStartScreen.Y }, { m_vecEndScreen.X, m_vecEndScreen.Y }, { 139,0,255,255 });
		}
	}
}

bool menu;

int M_Index = 0;
int Changes = 0;
bool updated = false;

const int PlayerESP = 0;
const int WeaponESP = 1;
const int ClothESP = 2;
const int houseESP = 3;
const int InvESP = 4;
//const int ItemESP = 5;
const int otherESP = 5;
const int Aimbot = 6;
const int AimbotFov = 7;
const int PlayerDistance = 8;
const int ItemDistance = 9;
const int MagicDistance = 10;

const char* bools[2] = { "[-]", "[+]" };



void Switch(int index)
{
	if (index == PlayerESP)
	{
		set::PlayerESP = !set::PlayerESP;
	}

	if (index == WeaponESP)
	{
		set::WeaponESP = !set::WeaponESP;
	}
	if (index == ClothESP)
	{
		set::ClothESP = !set::ClothESP;
	}
	if (index == houseESP)
	{
		set::houseESP = !set::houseESP;
	}
	if (index == InvESP)
	{
		set::InvESP = !set::InvESP;
	}

	if (index == otherESP)
	{
		set::otherESP = !set::otherESP;
	}
	if (index == Aimbot)
	{
		set::MagicBullet = !set::MagicBullet;
	}

	if (index == AimbotFov)
	{
		if (GetAsyncKeyState(37))
		{
			if (set::AimFov > 1)
			{
				set::AimFov -= 5;
			}
		}
		else if (GetAsyncKeyState(39))
		{
			if (set::AimFov < 320)
			{
				set::AimFov += 5;
			}
		}
	}
	if (index == PlayerDistance)
	{
		if (GetAsyncKeyState(37))
		{
			if (set::PlayerDistance > 1)
			{
				set::PlayerDistance -= 50;
			}
		}
		else if (GetAsyncKeyState(39))
		{
			if (set::PlayerDistance < 1000)
			{
				set::PlayerDistance += 50;
			}
		}
	}
	if (index == ItemDistance)
	{
		if (GetAsyncKeyState(37))
		{
			if (set::ItemDistance > 1)
			{
				set::ItemDistance -= 25;
			}
		}
		else if (GetAsyncKeyState(39))
		{
			if (set::ItemDistance < 150)
			{
				set::ItemDistance += 25;
			}
		}
	}
	if (index == MagicDistance)
	{
		if (GetAsyncKeyState(37))
		{
			if (set::MagicDistance > 25)
			{
				set::MagicDistance -= 25;
			}
		}
		else if (GetAsyncKeyState(39))
		{
			if (set::MagicDistance < 100)
			{
				set::MagicDistance += 25;
			}
		}
	}




}
void Navigation()
{
	if (GetAsyncKeyState(40)) // UP ARROW
	{
		if (M_Index < 11)
			M_Index++;
		Changes++;
		updated = true;
	Sleep(200);
	}
	if (GetAsyncKeyState(38)) // DOWN ARROW
	{
		if (M_Index > 0)
			M_Index--;
		Changes++;
		updated = true;
		Sleep(200);
    }
	if (GetAsyncKeyState(39))
	{// LEFT ARROW
		Switch(M_Index);
			Changes++;
			updated = true;
			Sleep(150);
	}
	if (GetAsyncKeyState(37))
	{// RIGHT ARROW
		Switch(M_Index);
			Changes++;
			updated = true;

			Sleep(150);

	}

		
	
}
void AddContainsToggleMenu(float MenuPos, const char* text, const int index, bool Functions)
{
	if (M_Index == index)
	{
		pOverlay->Text({ 30, MenuPos }, TextLeft, { 0, 255, 0 }, text);
		pOverlay->Text({ 10, MenuPos }, TextLeft, { 255, 0, 0 }, bools[Functions]);

	}
	else
	{
		pOverlay->Text({ 30, MenuPos }, TextLeft, { 0, 255, 255 }, text);
		pOverlay->Text({ 10, MenuPos }, TextLeft, { 255, 0, 0 }, bools[Functions]);
	}
}
__forceinline void menushka()
{
	int iWidth = GetSystemMetrics(SM_CXSCREEN);  // разрешение экрана по горизонтали
	int iHeight = GetSystemMetrics(SM_CYSCREEN); // разрешение экрана по вертикали+
	float MenuPos = iHeight / 2;
	if (set::menu)
	{
		AddContainsToggleMenu(MenuPos, "Player ESP ", PlayerESP, set::PlayerESP);

		MenuPos += 15;
		AddContainsToggleMenu(MenuPos, "Weapon ESP  ", WeaponESP, set::WeaponESP);

		MenuPos += 15;
		AddContainsToggleMenu(MenuPos, "Cloth ESP ", ClothESP, set::ClothESP);

		MenuPos += 15;
		AddContainsToggleMenu(MenuPos, "House ESP  ", houseESP, set::houseESP);

		MenuPos += 15;
		AddContainsToggleMenu(MenuPos, "Inventory ESP  ", InvESP, set::InvESP);

		MenuPos += 15;
		AddContainsToggleMenu(MenuPos, "Other ESP ", otherESP, set::otherESP);

		MenuPos += 15;

		AddContainsToggleMenu(MenuPos, "Aimbot ", Aimbot, set::MagicBullet);
		MenuPos += 15;

		char wFOV[256];
		int Fovs = set::AimFov;
		sprintf_s(wFOV, "Aim Fov : %d", Fovs);
		if (M_Index == AimbotFov)
		{
			pOverlay->Text({ 25, MenuPos }, TextLeft, { 0, 255, 0 }, wFOV);

		}
		else
		{
			pOverlay->Text({ 25, MenuPos }, TextLeft, { 255, 255, 255 }, wFOV);


		}	MenuPos += 15;
				
		
		char wPlayerDistance[256];
		int PlayerDistances = set::PlayerDistance;
		sprintf_s(wPlayerDistance, "PlayerDistance : %d", PlayerDistances);
		if (M_Index == PlayerDistance)
		{
			pOverlay->Text({ 25, MenuPos }, TextLeft, { 0, 255, 0 }, wPlayerDistance);

		}
		else
		{
		

			pOverlay->Text({ 25, MenuPos }, TextLeft, { 255, 255, 255 }, wPlayerDistance);


		}	
		MenuPos += 15;
		char wItemDistances[256];
		int ItemDistances = set::ItemDistance;
		sprintf_s(wItemDistances, "ItemDistance : %d", ItemDistances);
		if (M_Index == ItemDistance)
		{
			pOverlay->Text({ 25, MenuPos }, TextLeft, { 0, 255, 0 }, wItemDistances);

		}
		else
		{


			pOverlay->Text({ 25, MenuPos }, TextLeft, { 255, 255, 255 }, wItemDistances);


		}	MenuPos += 15;
		char wMagicDistance[256];
		int MagicDistances = set::MagicDistance;
		sprintf_s(wMagicDistance, "MagicDistance : %d", MagicDistances);
		if (M_Index == MagicDistance)
		{
			pOverlay->Text({ 25, MenuPos }, TextLeft, { 0, 255, 0 }, wMagicDistance);

		}
		else
		{

			//	const int MagicDistance = 10;
			pOverlay->Text({ 25, MenuPos }, TextLeft, { 255, 255, 255 }, wMagicDistance);


		}	MenuPos += 15;


	}
}
bool ProcessLoot(Overlay* pOverlay, Enfusion::CCamera* pCamera, Enfusion::CEntity* pEntity, Enfusion::CEntity* pLocalPlayer) // MENU меню тут
{
float iWidth = GetSystemMetrics(SM_CXSCREEN);  // разрешение экрана по горизонтали
float iHeight = GetSystemMetrics(SM_CYSCREEN); // разрешение экрана по вертикали

pOverlay->Line(ImVec2{ iWidth / 2, iHeight / 2 }, ImVec2{ iWidth / 2 + 4, iHeight / 2 + 4 }, { 0, 0, 0, 255 });
pOverlay->Line(ImVec2{ iWidth / 2, iHeight / 2 }, ImVec2{ iWidth / 2 + 4, iHeight / 2 - 4 }, { 0, 0, 0, 255 });
pOverlay->Line(ImVec2{ iWidth / 2, iHeight / 2 }, ImVec2{ iWidth / 2 - 4, iHeight / 2 - 4 }, { 0, 0, 0, 255 });
pOverlay->Line(ImVec2{ iWidth / 2, iHeight / 2 }, ImVec2{ iWidth / 2 - 4, iHeight / 2 + 4 }, { 0, 0, 0, 255 });
pOverlay->Line(ImVec2{ iWidth / 2 + 4 , iHeight / 2 + 4 }, ImVec2{ iWidth / 2 + 4, iHeight / 2 + 4 }, { 255, 0, 0, 255 });
pOverlay->Line(ImVec2{ iWidth / 2 + 4, iHeight / 2 - 4 }, ImVec2{ iWidth / 2 + 4, iHeight / 2 - 4 }, { 255, 0, 0, 255 });
pOverlay->Line(ImVec2{ iWidth / 2 - 4, iHeight / 2 - 4 }, ImVec2{ iWidth / 2 - 4, iHeight / 2 - 4 }, { 255, 0, 0, 255 });
pOverlay->Line(ImVec2{ iWidth / 2 - 4, iHeight / 2 + 4 }, ImVec2{ iWidth / 2 - 4, iHeight / 2 + 4 }, { 255, 0, 0, 255 });


	float MenuPos = iHeight / 2;
	if (GetAsyncKeyState(VK_INSERT) && 0x8000)
	{
		set::menu = !set::menu;

	};


	
	if (set::menu) {

		pOverlay->RectFilled({ 5, MenuPos -5 }, { 125 ,180 }, ImColor{ 24, 23,28, 150 });
		pOverlay->Rect({ 5, MenuPos - 5}, { 126 ,181 }, ImColor{ 255, 92,219, 250 });

		
	};



///////////////////////////////////////////////////////////////////
///////////////////////////// MENU//////////////////////
///////////////////////////////////////////////////////////////////

	Navigation();
	menushka();
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////




	Enfusion::CDayZPlayerType* pEntityType = pEntity->GetRendererEntityType();

	if (!pEntityType)
		return false;

	std::string pszTypeName = pEntityType->GetTypeName();

	Enfusion::CEntityVisualState* pVisualState = pEntity->GetRendererVisualState();

	if (!pVisualState)
		return false;

	//	printf("pVisualState: 0x%p\n", pVisualState);

	Math::Vector3D Location = pVisualState->GetPosition();

	Math::Vector3D ScreenLocation = Math::Vector3D::ZeroVector;
	if (!Core::WorldToScreen(pCamera, Location, ScreenLocation))
		return false;

	std::string CleanName = pEntityType->GetCleanName();
	std::string ConfigName = pEntityType->GetConfigName();

	Enfusion::CEntityVisualState* pLocalVisualState = pLocalPlayer->GetRendererVisualState();

	if (!pLocalVisualState)
	{
		TRACE("[ERR] pLocalVisualState is nullptr!");
		return false;
	}

	float m_flDistance = pLocalVisualState->GetPosition().Distance(Location);


		


if (m_flDistance > set::ItemDistance)
			return false;


	// working good
	//{
if (set::ItemESP)
{

	auto ShouldBeRendered = [&]() -> bool
	{

		if (!Core::strcmp("clothing", ConfigName.c_str()) && set::ClothESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });

			return true;
		}
		if (!Core::strcmp("house", ConfigName.c_str()) && set::houseESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });

			return true;
		}
		if (!Core::strcmp("inventoryItem", ConfigName.c_str()) && set::InvESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });

			return true;	
		}
		if (!Core::strcmp("ItemBook", ConfigName.c_str()) && set::otherESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });

			return true;
		}
		if (!Core::strcmp("ItemCompass", ConfigName.c_str()) && set::otherESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
		    pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });
		
			return true;
		}
		if (!Core::strcmp("ItemMap", ConfigName.c_str()) && set::otherESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });

			return true;
		}
		if (!Core::strcmp("itemoptics", ConfigName.c_str()) && set::WeaponESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });

			return true;
		}
		if (!Core::strcmp("ItemTransmitter", ConfigName.c_str()) && set::otherESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 });

			return true;
		}
		if (!Core::strcmp("ProxyMagazines", ConfigName.c_str()) && set::WeaponESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 10, 10,250, 250 });
			return true;

		}
	
		if (!Core::strcmp("Weapon", ConfigName.c_str()) && set::WeaponESP)
		{
			pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
			pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 250, 10,10, 250 });
			// pOverlay->RectFilled({ ScreenLocation.X - 5, ScreenLocation.Y }, { 105,  15 }, ImColor{ 255, 255,28, 150 });
			
			return true;
		}
		

	//	if (set::otherESP)
		//	return true;

		return false;


	};
	ImColor Color = ShouldBeRendered() ? ImColor{ 0, 191,255, 255 } : ImColor{ 255, 255, 255, 0 };
	pOverlay->Text({ ScreenLocation.X, ScreenLocation.Y - 6 }, TextLayout::TextCenter, Color, "%s [%d]", CleanName.c_str(), static_cast<int>(m_flDistance));
}

	//if (!ShouldBeRendered())
	//	return false;
//	printf("pEntityType: 0x%p @ m_szName: %s\n", pEntityType, m_szName);

	//if (strlen(m_szName))

//	ImColor Color = ShouldBeRendered() ? ImColor{0, 191,255, 255} : ImColor{255, 255, 255, 200};
	if (set::ItemESP)
	{
		
	//	pOverlay->RectFilled({ ScreenLocation.X - 10, ScreenLocation.Y }, { 15,  15 }, ImColor{ 24, 23,28, 150 });
	//	pOverlay->Rect({ ScreenLocation.X - 9, ScreenLocation.Y }, { 16,  16 }, ImColor{ 255, 92,219, 250 }); 
	}
	//else
	//	pOverlay->Text({ ScreenLocation.X, ScreenLocation.Y }, TextLayout::TextCenter, { 255,255,255,255 }, "%s", pEntityType->GetCleanName().c_str());
	return true;
}

std::vector<uint32_t> human_bone_indices = {
		0, 19, 20, 21, 24,	// pelvis to head
		94, 97, 99,			// left arm
		61, 63, 65,			// right arm
		9, 12, 14, 16,		// left leg
		1, 4, 6, 8			// right leg
};

std::vector<uint32_t> zombie_bone_indices = {
		0, 15, 16, 19, 21,	// pelvis to head
		56, 59, 60,			// left arm
		24, 25, 27,			// right arm
		9, 12, 13, 14,		// left leg
		1, 4, 6, 7			// right leg
};


void DrawPlayerSkeleton(Overlay* pOverlay, Enfusion::CCamera* pCamera, Enfusion::CEntity * pEntity)
{
	//int ArrSize = human_bone_indices.size() / 2;
	if (set::PlayerESP)
	{
		DrawBone(pOverlay, pCamera, pEntity, 49, 24, false);
		DrawBone(pOverlay, pCamera, pEntity, 24, 22, false);
		DrawBone(pOverlay, pCamera, pEntity, 22, 21, false);
		DrawBone(pOverlay, pCamera, pEntity, 21, 20, false);
		DrawBone(pOverlay, pCamera, pEntity, 20, 19, false);
		DrawBone(pOverlay, pCamera, pEntity, 19, 18, false);
		DrawBone(pOverlay, pCamera, pEntity, 18, 0, false);


		DrawBone(pOverlay, pCamera, pEntity, 21, 61, false);
		DrawBone(pOverlay, pCamera, pEntity, 21, 94, false);
		DrawBone(pOverlay, pCamera, pEntity, 99, 97, false);
		DrawBone(pOverlay, pCamera, pEntity, 97, 94, false);

		DrawBone(pOverlay, pCamera, pEntity, 65, 63, false);
		DrawBone(pOverlay, pCamera, pEntity, 63, 61, false);

		DrawBone(pOverlay, pCamera, pEntity, 0, 9, false);
		DrawBone(pOverlay, pCamera, pEntity, 16, 14, false);
		DrawBone(pOverlay, pCamera, pEntity, 14, 12, false);
		DrawBone(pOverlay, pCamera, pEntity, 12, 9, false);

		DrawBone(pOverlay, pCamera, pEntity, 0, 1, false);
		DrawBone(pOverlay, pCamera, pEntity, 8, 6, false);
		DrawBone(pOverlay, pCamera, pEntity, 6, 4, false);
		DrawBone(pOverlay, pCamera, pEntity, 4, 1, false);

		DrawBone(pOverlay, pCamera, pEntity, 1, 4, false);
		DrawBone(pOverlay, pCamera, pEntity, 4, 6, false);
		DrawBone(pOverlay, pCamera, pEntity, 6, 8, false);


		DrawBone(pOverlay, pCamera, pEntity, 9, 12, false);
		DrawBone(pOverlay, pCamera, pEntity, 12, 14, false);
		DrawBone(pOverlay, pCamera, pEntity, 14, 16, false);







	

	}

//	DrawBone(pOverlay, pCamera, pEntity, 113, 114, false);
//	DrawBone(pOverlay, pCamera, pEntity, 114, 115, false);


	//for (int i = 0; i < ArrSize; i++)
	{
	//	uint32_t Start = human_bone_indices[i];
	//	uint32_t End = human_bone_indices[i + 1];

	//	DrawBone(pOverlay, pCamera, pEntity, Start, End, false);
		//TRACE("[DBG] F: %d T: %d", human_bone_indices[i], human_bone_indices[i + 1]);
	}


	/*DrawBone(pOverlay, pCamera, pEntity, 54, 15, false);

	DrawBone(pOverlay, pCamera, pEntity, 15, 113, false);
	DrawBone(pOverlay, pCamera, pEntity, 113, 114, false);
	DrawBone(pOverlay, pCamera, pEntity, 114, 115, false);*/

	//DrawBone(pOverlay, pCamera, pEntity, 115, 116, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 116, 120, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 120, 121, g_Config.m_Esp.m_colPlayerSkeletons, false);

	//DrawBone(pOverlay, pCamera, pEntity, 15, 67, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 67, 68, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 68, 69, g_Config.m_Esp.m_colPlayerSkeletons, false);

	//DrawBone(pOverlay, pCamera, pEntity, 69, 70, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 70, 74, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 74, 75, g_Config.m_Esp.m_colPlayerSkeletons, false);

	//DrawBone(pOverlay, pCamera, pEntity, 15, 14, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 14, 13, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 13, 12, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 12, 10, g_Config.m_Esp.m_colPlayerSkeletons, false);

	//DrawBone(pOverlay, pCamera, pEntity, 10, 30, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 30, 31, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 31, 32, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 32, 34, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 34, 35, g_Config.m_Esp.m_colPlayerSkeletons, false);

	//DrawBone(pOverlay, pCamera, pEntity, 10, 103, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 103, 21, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 21, 22, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 22, 24, g_Config.m_Esp.m_colPlayerSkeletons, false);
	//DrawBone(pOverlay, pCamera, pEntity, 24, 25, g_Config.m_Esp.m_colPlayerSkeletons, false);
}

bool ProcessEntity(Overlay* pOverlay, Enfusion::CCamera* pCamera, Enfusion::CEntity* pEntity, Enfusion::CNetworkClient* pNetworkingClient, Enfusion::CEntity* pLocalEntity)
{
	bool DrawBox = false;
	std::string Name = {};
	Math::Vector3D DBGPos = {};

	Enfusion::CDayZPlayerType* pRendererEntityType = pEntity->GetRendererEntityType();

	if (!pRendererEntityType)
	{
		//		TRACE("EntityType is nullptr! Processing entity failure!");
		return false;
	}

	std::string szConfigName = pRendererEntityType->GetConfigName();
	std::string szCleanName = pRendererEntityType->GetCleanName(); // ANSI

	std::string ItemInHands = {};

#ifdef TRACELOG
	TRACE("Entity: 0x%p @ Type: 0x%p => CN: %s", pEntity, pRendererEntityType, szConfigName.c_str());
#endif
	Hash::hash32_t hhConfigName = Hash::FNV1a::Get(szConfigName.c_str());
	//	TRACE("[DBG] szConfigName: %s", szConfigName);
	Enfusion::CEntityVisualState* pRendererVisualState = pEntity->GetRendererVisualState();
	Enfusion::CEntityVisualState* pFutureVisualState = pEntity->GetFutureVisualState();

	if (!pRendererVisualState)
	{
		//		TRACE("[WARN] RendererVisualState is nullptr! Trying to use FutureVisualState!");
				//return false;
				//pVisualState = pEntity->GetFutureVisualState();
	}

	if (!pFutureVisualState)
	{
		//TRACE("[WARN] pFutureVisualState is nullptr!\n");
		//pVisualState = pEntity->GetFutureVisualState();
	}

	if (pFutureVisualState && !pRendererVisualState)
		pRendererVisualState = pFutureVisualState;
	else if (!pFutureVisualState && !pRendererVisualState)
	{
		//		TRACE("[FACE] [WARN] Both visual states is nullptr! Processing entity failure!\n");
		return false;
	}

	Math::Vector3D vecEntRootPos = pRendererVisualState->GetPosition();
	Math::Vector3D vecEntHeadPos = pRendererVisualState->GetHeadPosition();

	float flDistance = pLocalEntity->GetRendererVisualState()->GetPosition().Distance(pRendererVisualState->GetPosition());


	if (flDistance > set::PlayerDistance)
		return false;

	if (hhConfigName == HASH("dayzplayer"))
	{
		Math::Vector3D P = {};
		if (GetBonePosition(pEntity, 24, &vecEntHeadPos, false))
			DrawBox = true;

		DrawPlayerSkeleton(pOverlay, pCamera, pEntity);
		Name = pNetworkingClient->GetPlayerName(pEntity);

		Enfusion::CInventory* pInventory = pEntity->GetInventory();

		if (pInventory)
		{
			Enfusion::CEntity* pItemInHands = pInventory->GetItemInHands();

			if (pItemInHands)
			{
				Enfusion::CDayZPlayerType* pItemType = pItemInHands->GetRendererEntityType();
				if (pItemType)
				{
					ItemInHands = pItemType->GetCleanName();


				}
			}
		}
	}
	if (hhConfigName == HASH("dayzanimal"))
	{
		Name = szCleanName;
	}
	if (hhConfigName == HASH("dayzinfected"))
	{
		Math::Vector3D P = {};
		if (GetBonePosition(pEntity, 21, &vecEntHeadPos, true))
			DrawBox = true;

		Name = szCleanName;
	}
	if (hhConfigName == HASH("car") /*&& bShowCars etc...*/)
	{
		Name = szCleanName;
	}

	Math::Vector3D ScreenLocation = {};
	if (set::PlayerESP)
	{
	if (Core::WorldToScreen(pCamera, vecEntRootPos, ScreenLocation))
	{
			//	pOverlay->Rect({ ScreenLocation.X, ScreenLocation.Y }, { 25, 25 }, { 255,255,255,255 });
		pOverlay->Text({ ScreenLocation.X, ScreenLocation.Y + 15 }, TextLayout::TextCenter, { 255,255,255,255 }, "%s", Name.c_str());
		if (pEntity->IsDead())
			pOverlay->Text({ ScreenLocation.X, ScreenLocation.Y + 35 }, TextLayout::TextCenter, { 255,0,0,255 }, "Dead");

			pOverlay->Text({ ScreenLocation.X, ScreenLocation.Y + 25 }, TextLayout::TextCenter, { 255,0,0,255 }, "[%d] m", static_cast<int>(flDistance)); // %f 

		if (!ItemInHands.empty())
			pOverlay->Text({ ScreenLocation.X, ScreenLocation.Y + 40 }, TextLayout::TextCenter, { 255,255,0,255 }, "%s", ItemInHands.c_str());
	}

	Math::Vector3D HeadScreenLocation = {};
	if (!Core::WorldToScreen(pCamera, vecEntHeadPos, HeadScreenLocation))
		return false;

	float H = ScreenLocation.Y - HeadScreenLocation.Y;
	float W = H / 2;
	float X = ScreenLocation.X - W / 2;
	float Y = HeadScreenLocation.Y;

	/*pOverlay->Rect({ X - 1, Y - 1 }, { W + 1, H + 1 }, { 0, 0, 0, 255 });
	pOverlay->Rect({ X, Y }, { W, H }, { 255,255,255,0 });
	pOverlay->Rect({ X + 1, Y + 1 }, { W + 1, H + 1 }, { 0,255,0,255 });*/

	if (DrawBox)
	{
		int m_iWidth = W / 3;

		pOverlay->Line({ X, Y }, { X + m_iWidth, Y }, { 0,255,0,255 });
		pOverlay->Line({ X, Y }, { X, Y + m_iWidth }, { 0,255,0,255 });

		pOverlay->Line({ X + W - m_iWidth, Y }, { X + W, Y }, { 0, 255,0,255 });
		pOverlay->Line({ X + W, Y + m_iWidth }, { X + W, Y }, { 0, 255,0,255 });

		pOverlay->Line({ X, Y + H - m_iWidth }, { X, Y + H }, { 0, 255,0,255 });
		pOverlay->Line({ X, Y + H }, { X + m_iWidth, Y + H }, { 0, 255,0,255 });

		pOverlay->Line({ X + W, Y + H - m_iWidth }, { X + W, Y + H }, { 0, 255,0,255 });
		pOverlay->Line({ X + W - m_iWidth, Y + H }, { X + W, Y + H }, { 0, 255,0,255 });
	}
	}
	/*for (int i = 0; i < 150; i++)
	{
		Math::Vector3D m_vecStartW;
		Math::Vector3D m_vecStart;
		if (GetBonePosition(pEntity, (QWORD)pEntity->GetRendererVisualState(), i, &m_vecStartW, false))
		{
			if (Core::WorldToScreen(Globals.Camera, m_vecStartW, m_vecStart))
			{
				pOverlay->Text({ m_vecStart.X, m_vecStart.Y }, TextLayout::TextCenter, { 255,255,0,255 }, "%d", i);
			}
		}
	}*/

#if 0
	DrawBone(pOverlay, pEntity, 54, 15, false);

	DrawBone(pOverlay, pEntity, 15, 113, false);
	DrawBone(pOverlay, pEntity, 113, 114, false);
	DrawBone(pOverlay, pEntity, 114, 115, false);

	DrawBone(pOverlay, pEntity, 115, 116, false);
	DrawBone(pOverlay, pEntity, 116, 120, false);
	DrawBone(pOverlay, pEntity, 120, 121, false);

	DrawBone(pOverlay, pEntity, 15, 67, false);
	DrawBone(pOverlay, pEntity, 67, 68, false);
	DrawBone(pOverlay, pEntity, 68, 69, false);

	DrawBone(pOverlay, pEntity, 69, 70, false);
	DrawBone(pOverlay, pEntity, 70, 74, false);
	DrawBone(pOverlay, pEntity, 74, 75, false);

	DrawBone(pOverlay, pEntity, 15, 14, false);
	DrawBone(pOverlay, pEntity, 14, 13, false);
	DrawBone(pOverlay, pEntity, 13, 12, false);
	DrawBone(pOverlay, pEntity, 12, 10, false);

	DrawBone(pOverlay, pEntity, 10, 30, false);
	DrawBone(pOverlay, pEntity, 30, 31, false);
	DrawBone(pOverlay, pEntity, 31, 32, false);
	DrawBone(pOverlay, pEntity, 32, 34, false);
	DrawBone(pOverlay, pEntity, 34, 35, false);

	DrawBone(pOverlay, pEntity, 10, 103, false);
	DrawBone(pOverlay, pEntity, 103, 21, false);
	DrawBone(pOverlay, pEntity, 21, 22, false);
	DrawBone(pOverlay, pEntity, 22, 24, false);
	DrawBone(pOverlay, pEntity, 24, 25, false);
#endif
	//if (pszName.c_str())
	//	printf("Name: %s\n", pszName.c_str());

	//printf("HeadPosition: %f %f %f\n", HeadPosition.X, HeadPosition.Y, HeadPosition.Z);
	//Math::Vector3D HeadScreenLocation = {};
	//if (Core::WorldToScreen(Globals.Camera, HeadPosition, HeadScreenLocation))
	//{
	//	pOverlay->Rect({ HeadScreenLocation.X, HeadScreenLocation.Y }, { 10, 10 }, { 255,255,255,255 });
	////	pOverlay->Text({ ScreenLocation.X, ScreenLocation.Y + 15 }, TextLayout::TextCenter, { 255,255,255,255 }, "%s", pszTypeName);
	//}

	return true;
}

// aspect ratio
//mem->Write<float>(mem-> Driver.Read<uint64_t>(world + 0x28) + 0x6C, 1.33333 * 2);
//mem->Write<float>(mem-> Driver.Read<uint64_t>(world + 0x28) + 0x70, 0.75 * 2);
void Render(Overlay* pOverlay, void* pData)
{


	///////

	Aimbotz::m_pTarget = nullptr;
	GlobalData_t* pGlobalData = reinterpret_cast<GlobalData_t*>(pData);
	Enfusion::CWorld* pWorld = reinterpret_cast<Enfusion::CWorld*>( Driver.Read<QWORD>(pGlobalData->m_World));

	if (!pWorld)
		return;

	Enfusion::CCamera* pCamera = pWorld->GetCamera();

	if (!pCamera)
		return;

	Enfusion::CNetworkClient* pNetworkClient = reinterpret_cast<Enfusion::CNetworkClient*>( Driver.Read<QWORD>(pGlobalData->m_NetworkClient));

	float iDbgPos = 0;

	//pOverlay->Text({ 10, iDbgPos + 5.f }, TextLeft, { 0.f, 1.f, 1.f }, "DayZ Standalone MEME "); iDbgPos += 15 + 5.f; //By Blue Hair, Shizi10, M3XSCAM, Balbes2301





	//pOverlay->Text({ 10, iDbgPos + 5.f }, TextLeft, { 0.f, 1.f, 1.f }, "DayZ Standalone Lite "); iDbgPos += 15 + 5.f;
	//pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "Overlay Framerate: %f", ImGui::GetIO().Framerate); iDbgPos += 15;

	pOverlay->Text({ 10, iDbgPos + 5.f }, TextLeft, { 0.f, 1.f, 1.f }, "flow1337#2460 // https://t.me/reverse1337"); iDbgPos += 15 + 5.f;
	pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "Overlay Framerate: %f", ImGui::GetIO().Framerate); iDbgPos += 15;


	Enfusion::CSortedObject* pCameraOn = pWorld->GetCameraOn();

	if (!pCameraOn)
		return;

	Enfusion::CEntity* pLocalPlayer = pCameraOn->GetEntity()->GetBaseEntity();

	if (!pLocalPlayer)
		return;

	//pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "World: 0x%p", pWorld); iDbgPos += 15;
	//pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "Camera: 0x%p", pCamera); iDbgPos += 15;
	//pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "LocalPlayer: 0x%p", pLocalPlayer); iDbgPos += 15;
	//pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "NetworkClient: 0x%p", pNetworkClient); iDbgPos += 15;

	// GAY Sh1T
	/*for (int i = 0; i < 150; i++)
	{
		Math::Vector3D vecBonePos = {};
		Math::Vector3D vecBonePosScreen = {};
		if (GetBonePosition(pLocalPlayer, i, &vecBonePos, false))
		{
			if (Core::WorldToScreen(pCamera, vecBonePos, vecBonePosScreen))
			{
				pOverlay->Text({ vecBonePosScreen.X, vecBonePosScreen.Y }, TextLayout::TextCenter, { 255,255,0,255 }, "%d", i);
			}
		}
	}*/

	DrawPlayerSkeleton(pOverlay, pCamera, pLocalPlayer);

	//Math::Vector3D LocalHeadPos = {};
	//if (GetBonePosition(pLocalPlayer, (QWORD)pLocalPlayer->GetRendererVisualState(), 54, &LocalHeadPos, false))
	//{
	//	Math::Vector3D ScreenLHP = {};

	//	if (Core::WorldToScreen(pCamera, LocalHeadPos, ScreenLHP))
	//	{
	//		pOverlay->Text({ ScreenLHP.X, ScreenLHP.Y }, TextLayout::TextCenter, { 255, 0,0, 255 }, "Head");
	//	}
	//}

	Enfusion::CList<Enfusion::CEntity> NearAnimalList = { pWorld->GetNearAnimalTablePtr(), pWorld->GetNearAnimalTableSize() };
	Enfusion::CList<Enfusion::CEntity> FarAnimalList = { pWorld->GetFarAnimalTablePtr(), pWorld->GetFarAnimalTableSize() };
	Enfusion::CList<Enfusion::CEntity> SlowAnimalList = { pWorld->GetSlowAnimalTablePtr(), pWorld->GetSlowAnimalTableSize() };

	//	TRACE("[DBG] NearAnimalListSize: -> %d", NearAnimalList.GetSize());

	uint32_t NearAnimalListSize = NearAnimalList.GetSize();
	for (uint32_t i = 0; i < NearAnimalListSize; i++)
	{
		Enfusion::CEntity* pEntity = NearAnimalList.Get(i);

		if (!pEntity || !Core::IsValidPointer(pEntity)) continue;

		if (pEntity == pLocalPlayer)
		{
	//		printf("Local!\n");
			continue;
		}

		if (!ProcessEntity(pOverlay, pCamera, pEntity, pNetworkClient, pLocalPlayer))
			continue;
	}

	uint32_t FarAnimalListSize = FarAnimalList.GetSize();
	for (uint32_t i = 0; i < FarAnimalListSize; i++)
	{
		Enfusion::CEntity* pEntity = FarAnimalList.Get(i);

		if (!pEntity) continue;

		if (pEntity == pLocalPlayer)
		{
//			printf("Local!\n");
			continue;
		}

		if (!ProcessEntity(pOverlay, pCamera, pEntity, pNetworkClient, pLocalPlayer))
			continue;
	}

	uint32_t SlowAnimalListSize = SlowAnimalList.GetSize();
	for (uint32_t i = 0; i < SlowAnimalListSize; i++)
	{
		Enfusion::CEntity* pEntity = SlowAnimalList.GetLoot(i);

		if (!pEntity || !Core::IsValidPointer(pEntity) || !HIWORD(reinterpret_cast<DWORD_PTR>(pEntity)) || LOWORD(reinterpret_cast<DWORD_PTR>(pEntity)) == 0x1)
			continue;

//		TRACE("[DBG] Slow ent: 0x%p => 0x%p %p\n", pEntity, HIWORD((DWORD_PTR)pEntity), LOWORD((DWORD_PTR)pEntity));

		if (pEntity == pLocalPlayer)
		{
//			printf("Local!\n");
			continue;
		}

		if (!ProcessEntity(pOverlay, pCamera, pEntity, pNetworkClient, pLocalPlayer))
			continue;
	}


	// насрано!
	// 12 листов с лутом, но дейзи крейзи девы все пушат в нулевой
//	for (int i = 0; i < 12; i++)
	{
#if 0
	//	QWORD CurrentItemList =  Driver.Read<QWORD>((QWORD)pWorld + Offsets::World::ItemTable + (i * 0x20));

//		if (!CurrentItemList)
	//		continue;
//		DWORD32 CurrentItemListSize =  Driver.Read<DWORD32>((QWORD)pWorld + Offsets::World::ItemTable + (i * 0x20) + sizeof(PVOID));

//		TRACE("i:: %d ItemList: 0x%p :: ListSize:: %d", i, CurrentItemList, CurrentItemListSize);

//		QWORD ItemTable =  Driver.Read<QWORD>((QWORD)pWorld + Offsets::World::ItemTable);
//		DWORD32 ItemTableSize =  Driver.Read<DWORD32>((QWORD)pWorld + Offsets::World::ItemTable + sizeof(PVOID));

//		TRACE("ItemTable:: 0x%p => %d", ItemTable, ItemTableSize);
#endif
		Enfusion::CList<Enfusion::CEntity> ItemList = { pWorld->GetItemTablePtr(0), pWorld->GetItemTableSize(0) };

		uint32_t ItemListSize = ItemList.GetSize();
		for (uint32_t k = 0; k < ItemListSize; k++)
		{
#if 0
			/*if ( Driver.Read<QWORD>(CurrentItemList + (k * 3)) != 1)
				continue;*/

			//if (!IsAllocated(CurrentItemList, CurrentItemListSize, k))
			//	continue;
			// 
			// 
			//uint32_t ValidCheck =  Driver.Read<uint32_t>(ItemTable + (k * 0x18));

			//if (ValidCheck != 1)
			//	continue;

		//	Enfusion::CEntity* pEntity =  Driver.Read<Enfusion::CEntity*>(ItemTable + (k * 0x18) + 0x8);
			// 
#endif
			Enfusion::CEntity* pEntity = ItemList.GetLoot(k);/* Driver.Read<Enfusion::CEntity*>(CurrentItemList + (k * 0x8));*/

			if (!pEntity || !Core::IsValidPointer(pEntity) || !HIWORD(reinterpret_cast<DWORD_PTR>(pEntity)) || LOWORD(reinterpret_cast<DWORD_PTR>(pEntity)) == 0x1)
				continue;

//			TRACE("k:: %d @ pEntity: 0x%p", k, pEntity);

			if (!ProcessLoot(pOverlay, pCamera, pEntity, pLocalPlayer))
				continue;

	//		while (true)
	//		{ 


			//if (GetAsyncKeyState(VK_NUMPAD3))
			//{
			//	set::ItemDistance += 25.f;
			//}
			//else if (GetAsyncKeyState(VK_NUMPAD4))
			//{
			//	set::ItemDistance -= 25.f;
			//};
//			}

		//	printf("pEntity 0x%p\n", pEntity);
		}
	}

	 // SpeedHack();
#if 0
	/*QWORD NearEntityTable =  Driver.Read<QWORD>(pGlobalData->World + Offsets::NearEntityList);
	uint32_t nEntityCount =  Driver.Read<uint32_t>(pGlobalData->World + Offsets::NearEntityTableSize);*/

	//pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "Entity Count: %d", nEntityCount); iDbgPos += 15;
//	pOverlay->Text({ 10, iDbgPos }, TextLeft, { 1.f, 1.f, 1.f }, "LocalPlayer: 0x%p", pLocalPlayer); iDbgPos += 15;
	
	/*Enfusion::Array<Enfusion::CEntity*>* NearAnimalTable = pWorld->GetNearAnimalTable();

	printf("NearAnimalTable: 0x%p | %d\n", NearAnimalTable->GetBuffer(), NearAnimalTable->GetSize());

	for (DWORD32 EntIndex = 0; EntIndex < NearAnimalTable->GetSize(); EntIndex++)
	{
		Enfusion::CEntity* pEntity = NearAnimalTable->Get(EntIndex);

		if (!IsValidPointer(pEntity))
			continue;

		printf("Entity: 0x%p\n", pEntity);

		if (!ProcessEntity(pEntity))
		{
			continue;
		}
	 
		if (pEntity == pLocalPlayer) continue;
	}*/


	for (uint64_t i = 0; i < nEntityCount; i++)
	{
		Enfusion::CEntity* pEntity =  Driver.Read<Enfusion::CEntity*>(NearEntityTable + (i * 0x8)); // DayZInfected or DayZPlayer

		if (!pEntity) continue;

		if (pEntity == pLocalPlayer) continue;

		if (!ProcessEntity(pOverlay, pEntity))
		{
			continue;
		}
	}

#endif
	if (set::MagicBullet)
	{



		Aimbotz::Run(pCamera, pWorld, pLocalPlayer);

		Enfusion::CWinEngine* pWin = reinterpret_cast<Enfusion::CWinEngine*>(Globals.m_WinEngine);

		//pOverlay->Rect({ static_cast<float>(pWin->GetRight()) / 2.0f - AIM_FOV * 5.0f, static_cast<float>(pWin->GetBottom()) / 2.0f - AIM_FOV * 5.0f },
		//	{ AIM_FOV * 10.0f, AIM_FOV * 10.0f }, { 0, 255, 255, 255 });


		//pOverlay->GetDrawList()->AddCircle({ static_cast<float>(pWin->GetRight()) / 2.0f, static_cast<float>(pWin->GetBottom()) / 2.0f },
		//	AIM_FOV * 5.0f, IM_COL32(255, 0, 255, 255), 120);


	//	pOverlay->Rect({ static_cast<float>(pWin->GetRight()) / 2.0f - set::AimFov , static_cast<float>(pWin->GetBottom()) / 2.0f - set::AimFov },
	//		{ set::AimFov * 2, set::AimFov * 2 }, { 0, 255, 255, 255 }); //  вадрат


		pOverlay->GetDrawList()->AddCircle({ static_cast<float>(pWin->GetRight()) / 2.0f, static_cast<float>(pWin->GetBottom()) / 2.0f },
			set::AimFov , IM_COL32(255, 0, 255, 255), 120);

		if (Aimbotz::m_pTarget)
		{
			float iWidth = GetSystemMetrics(SM_CXSCREEN);  // разрешение экрана по горизонтали
			float iHeight = GetSystemMetrics(SM_CYSCREEN); // разрешение экрана по вертикали
			Math::Vector3D DBGScreen = {};
			if (Core::WorldToScreen(pCamera, Aimbotz::m_pTarget->GetRendererVisualState()->GetPosition(), DBGScreen))
			{
				pOverlay->Text({ DBGScreen.X, DBGScreen.Y }, TextLayout::TextCenter, { 255,0,0,255 }, "AIMTARGET!");
				pOverlay->Line({iWidth / 2  , iHeight / 2 }, { DBGScreen.X, DBGScreen.Y  }, { 255,0,0,255 });
			}

		}


	}
}
void HideConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);


}
#include "rawdata.h"
int RunCheatFromMemory() {
	//VMProtectBeginMutation("RunCheatFromMemory");
	void* pe = dmap;

	IMAGE_DOS_HEADER* DOSHeader;
	IMAGE_NT_HEADERS64* NtHeader;
	IMAGE_SECTION_HEADER* SectionHeader;

	PROCESS_INFORMATION PI;
	STARTUPINFOA SI;
	ZeroMemory(&PI, sizeof(PI));
	ZeroMemory(&SI, sizeof(SI));


	void* pImageBase;

	char currentFilePath[1024];

	DOSHeader = PIMAGE_DOS_HEADER(pe);
	NtHeader = PIMAGE_NT_HEADERS64(DWORD64(pe) + DOSHeader->e_lfanew);

	if (NtHeader->Signature == IMAGE_NT_SIGNATURE) {

		(GetModuleFileNameA)(NULL, currentFilePath, MAX_PATH);
		//create process
		if ((CreateProcessA)(currentFilePath, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &SI, &PI)) {

			CONTEXT* CTX;
			CTX = LPCONTEXT((VirtualAlloc)(NULL, sizeof(CTX), MEM_COMMIT, PAGE_READWRITE));
			CTX->ContextFlags = CONTEXT_FULL;


			UINT64 imageBase = 0;
			if ((GetThreadContext)(PI.hThread, LPCONTEXT(CTX)))
			{
				pImageBase = (VirtualAllocEx)(
					PI.hProcess,
					LPVOID(NtHeader->OptionalHeader.ImageBase),
					NtHeader->OptionalHeader.SizeOfImage,
					MEM_COMMIT | MEM_RESERVE,
					PAGE_EXECUTE_READWRITE
					);


				(WriteProcessMemory)(PI.hProcess, pImageBase, pe, NtHeader->OptionalHeader.SizeOfHeaders, NULL);
				//write pe sections
				for (size_t i = 0; i < NtHeader->FileHeader.NumberOfSections; i++)
				{
					SectionHeader = PIMAGE_SECTION_HEADER(DWORD64(pe) + DOSHeader->e_lfanew + 264 + (i * 40));

					(WriteProcessMemory)(
						PI.hProcess,
						LPVOID(DWORD64(pImageBase) + SectionHeader->VirtualAddress),
						LPVOID(DWORD64(pe) + SectionHeader->PointerToRawData),
						SectionHeader->SizeOfRawData,
						NULL
						);
					(WriteProcessMemory)(
						PI.hProcess,
						LPVOID(CTX->Rdx + 0x10),
						LPVOID(&NtHeader->OptionalHeader.ImageBase),
						8,
						NULL
						);

				}

				CTX->Rcx = DWORD64(pImageBase) + NtHeader->OptionalHeader.AddressOfEntryPoint;
				(SetThreadContext)(PI.hThread, LPCONTEXT(CTX));
				(ResumeThread)(PI.hThread);

				(WaitForSingleObject)(PI.hProcess, NULL);

				return 0;

			}
		}
	}
	//VMProtectEnd();
}
HANDLE iqvw64e_device_handle;

LONG WINAPI SimplestCrashHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	if (ExceptionInfo && ExceptionInfo->ExceptionRecord)
		Log(L"[!!] Crash at addr 0x" << ExceptionInfo->ExceptionRecord->ExceptionAddress << L" by 0x" << std::hex << ExceptionInfo->ExceptionRecord->ExceptionCode << std::endl);
	else
		Log(L"[!!] Crash" << std::endl);

	if (iqvw64e_device_handle)
		intel_driver::Unload(iqvw64e_device_handle);

	return EXCEPTION_EXECUTE_HANDLER;
}

int paramExists(const int argc, wchar_t** argv, const wchar_t* param) {
	size_t plen = wcslen(param);
	for (int i = 1; i < argc; i++) {
		if (wcslen(argv[i]) == plen + 1ull && _wcsicmp(&argv[i][1], param) == 0 && argv[i][0] == '/') { // with slash
			return i;
		}
		else if (wcslen(argv[i]) == plen + 2ull && _wcsicmp(&argv[i][2], param) == 0 && argv[i][0] == '-' && argv[i][1] == '-') { // with double dash
			return i;
		}
	}
	return -1;
}

void help() {
	Log(L"\r\n\r\n[!] Incorrect Usage!" << std::endl);
	Log(L"[+] Usage: kdmapper.exe [--free][--mdl][--PassAllocationPtr] driver" << std::endl);
}

bool callbackExample(ULONG64* param1, ULONG64* param2, ULONG64 allocationPtr, ULONG64 allocationSize, ULONG64 mdlptr) {
	UNREFERENCED_PARAMETER(param1);
	UNREFERENCED_PARAMETER(param2);
	UNREFERENCED_PARAMETER(allocationPtr);
	UNREFERENCED_PARAMETER(allocationSize);
	UNREFERENCED_PARAMETER(mdlptr);
	Log("[+] Callback example called" << std::endl);

	/*
	This callback occurs before call driver entry and
	can be usefull to pass more customized params in
	the last step of the mapping procedure since you
	know now the mapping address and other things
	*/
	return true;
}

uint64_t GetDriverBase(const char* Name)
{
	//get all system modules
Loop:
	ULONG Size = 0;
	NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)11, nullptr, Size, &Size);

	nt::PRTL_PROCESS_MODULES ModInfo = (nt::PRTL_PROCESS_MODULES)malloc(Size);

	if (NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)11, ModInfo, Size, &Size))
	{
		free(ModInfo); goto Loop;
	}

	//process list
	uint64_t Base = 0;
	for (ULONG i = 0; i < ModInfo->NumberOfModules; i++)
	{
		if (StrStrA(Name, ((char*)ModInfo->Modules[i].FullPathName + ModInfo->Modules[i].OffsetToFileName)))
		{
			Base = (uint64_t)ModInfo->Modules[i].ImageBase;
			break;
		}
	}

	//cleanup & ret
	free(ModInfo);
	return Base;
}

bool ParseDriverBytes(PIMAGE_NT_HEADERS& pNTH, PVOID* pOutDriverBytes)
{
	PVOID ImageLocal = (PVOID)rawData_data;

	PIMAGE_DOS_HEADER DOS_Head = (PIMAGE_DOS_HEADER)ImageLocal;
	if (DOS_Head->e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	DWORD64 _NT_Head = ((DWORD64)ImageLocal + DOS_Head->e_lfanew);

	if (_NT_Head < (DWORD64)ImageLocal + 64 || _NT_Head + 264 >(DWORD64)ImageLocal + 4096)
		return false;

	//check is 64bit driver
	if (((PIMAGE_NT_HEADERS)_NT_Head)->Signature != IMAGE_NT_SIGNATURE ||
		((PIMAGE_NT_HEADERS)_NT_Head)->OptionalHeader.Magic != 0x20B ||
		((PIMAGE_NT_HEADERS)_NT_Head)->OptionalHeader.Subsystem != 1)
	{
		printf("check 1 fail\n");
		getchar();
		return false;
	}
	//cleanup headers & ret image base
	pNTH = (PIMAGE_NT_HEADERS)_NT_Head;
	//  pNTH->OptionalHeader.MajorLinkerVersion = 0;
	//  pNTH->Signature = 0;
	//  pNTH->OptionalHeader.Magic = 0;
	  //ZeroMemory(ImageLocal, (DWORD)(_NT_Head - (DWORD64)ImageLocal));
	*pOutDriverBytes = ImageLocal;

	return true;
}
namespace mapper
{
	uint64_t map(const int argc, wchar_t** argv)
	{



		PIMAGE_NT_HEADERS ImageNt;
		PVOID Image = nullptr;

		if (!ParseDriverBytes(ImageNt, &Image))
			return 1;

		SetUnhandledExceptionFilter(SimplestCrashHandler);

		bool free = paramExists(argc, argv, L"free") > 0;
		bool mdlMode = paramExists(argc, argv, L"mdl") > 0;
		bool passAllocationPtr = paramExists(argc, argv, L"PassAllocationPtr") > 0;

		DWORD64 ntkernel = GetDriverBase("ntoskrnl.exe");
		Log(L"[->] ntoskrnl = 0x" << std::hex << ntkernel << std::endl);
		if (free) {
			Log(L"[+] Free pool memory after usage enabled" << std::endl);
		}

		if (mdlMode) {
			Log(L"[+] Mdl memory usage enabled" << std::endl);
		}

		if (passAllocationPtr) {
			Log(L"[+] Pass Allocation Ptr as first param enabled" << std::endl);
		}


		iqvw64e_device_handle = intel_driver::Load();

		if (iqvw64e_device_handle == INVALID_HANDLE_VALUE)
			return -1;



		uintptr_t ret = kdmapper::MapDriver(ImageNt, iqvw64e_device_handle, (PBYTE)Image, ntkernel, 0, free, true, mdlMode, passAllocationPtr, callbackExample);
		if (!ret)
		{
			intel_driver::Unload(iqvw64e_device_handle);
			return 1;
		}

		Sleep(100);
		intel_driver::Unload(iqvw64e_device_handle);
		Log(L"[+] success" << std::endl);


		return ret;



	}
}
int wmain(const int argc, wchar_t** argv)
{

	
	printf("\n\n\n");
	printf("initalize... \n");
	printf("\n\n\n");
	//mapper::map(argc, argv);

	if (!Driver.IsDriverLoaded())
	{
		printf("can't initalize\n");
		return -1;
	}
	else
	{
		printf("Driver loaded!\n");
	}
	
	Sleep(3500);
	printf("\t flow1337#2460 sanel sucks// https://t.me/reverse1337!\n");
	pOverlay = std::make_shared<Overlay>(L"DayZ", true);

	HWND hWnd = pOverlay->GetTargetWindow();

	while (!hWnd)
		hWnd = FindWindowA(GAME_WINDOW, nullptr);
	

	DWORD PID = 0x0;
	GetWindowThreadProcessId(hWnd, &PID);
	//printf("hwnd: %s", hWnd, "pid: %d", PID);
	std::cout << "hwnd: " << hWnd << " PID: " << PID << std::endl;
	Driver.SetPID(PID);
	// attach target process
	//char image_file_name[256];
	//std::memset(image_file_name, 0, sizeof(image_file_name));
	//std::strcpy(image_file_name, "DayZ_x64.exe");

	//for (int i = 14; image_file_name[i]; ++i)
	//	image_file_name[i] = 0;

	//for (int i = 0; image_file_name[i]; ++i)
	//	if (image_file_name[i] >= 'A' && image_file_name[i] <= 'Z')
	//		image_file_name[i] += 'a' - 'A';

	//buffer_start_t buf_start;
	//std::memset(&buf_start, 0, sizeof(buf_start));

	//buf_start.m_cmd_type = buffer_cmd_t::PRESTART;
	//buf_start.m_process_hash = HASH_RT(image_file_name);

	//run_driver_command(&buf_start, sizeof(buf_start));
	
	CDriverAPI::MODINFO GameModule = Driver.GetModInfo("DayZ_x64.exe");
	//uintptr_t ModBase = get_module_base(HASHHH("dayz_x64.exe"));
	printf("ModBase: %x", GameModule);

//	FindClass(GameModule.ModBase, 0);

	// 0x40899A0
	QWORD WorldPtr = (QWORD)Core::FindPattern("48 8B 0D ? ? ? ? 0F B6 D0", GameModule.ModBase); //STATIC_BY_BASE(0x40899A0); //      48 8B 05 69 D9 B9 03
	QWORD lWorldPtr = RVA(WorldPtr, 7);

	QWORD NetworkManagerPtr = (QWORD)Core::FindPattern("48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 1D ? ? ? ? 84 C0", GameModule.ModBase);
	QWORD lNetworkClientPtr = RVA(NetworkManagerPtr, 7) + 0x50; // FUCK THIS SHIT

	QWORD dwFrequency = (QWORD)Core::FindPattern("48 83 3D ? ? ? ? ? 75 3B", GameModule.ModBase);
	QWORD ldwFrequency =  QWORD(dwFrequency + 8 +  Driver.Read<DWORD>(dwFrequency + 3));

	QWORD dwWinEngine = (QWORD)Core::FindPattern("FF 90 ? ? ? ? 48 8B 0D ? ? ? ? 48 85 C9 74 0A", GameModule.ModBase) + 0x6;
	QWORD ldwWinEngine = RVA(dwWinEngine, 7);

	Globals.m_World = lWorldPtr; // читаем посто€нно
//	Globals.m_Camera =  Driver.Read<QWORD>(Globals.m_World + Offsets::World::Camera);
	Globals.m_NetworkClient = /* Driver.Read<QWORD>*/(lNetworkClientPtr); // читаем посто€нно
	Globals.m_WinEngine =  Driver.Read<QWORD>(ldwWinEngine);
	Globals.m_dwOffsetEntitySkeleton =  Driver.Read<DWORD>((QWORD)Core::FindPattern("48 8B 98 ? ? ? ? 48 8D 05 ? ? ? ? 4A 8B 94 F8 ? ? ? ? EB 3D", GameModule.ModBase) + 0x3);
	Globals.m_dwOffsetEntitySimpleSkeleton =  Driver.Read<DWORD>((QWORD)Core::FindPattern("48 8B 98 ? ? ? ? 48 8D 05 ? ? ? ? 4A 8B 94 F8 ? ? ? ? 48 8B CB", GameModule.ModBase) + 0x3);
	Globals.m_dwFrequencyPtr = /* Driver.Read<QWORD>*/(ldwFrequency);

	TRACE("World: 0x%p", Globals.m_World);
	//TRACE("Camera: 0x%p", Globals.m_Camera);
	TRACE("NetworkClient: 0x%p", Globals.m_NetworkClient);
	TRACE("WinEngine: 0x%p", Globals.m_WinEngine);
	TRACE("m_dwOffsetEntitySkeleton: 0x%p", Globals.m_dwOffsetEntitySkeleton);
	TRACE("m_dwOffsetEntitySimpleSkeleton: 0x%p", Globals.m_dwOffsetEntitySimpleSkeleton);
	TRACE("m_dwFrequencyPtr: 0x%p", Globals.m_dwFrequencyPtr);
	
//	printf("hwnd: %d -> %d\n", hWnd, reinterpret_cast<Enfusion::CWinEngine*>(Globals.m_WinEngine)->GetWindow());
//	printf("%dx%d", reinterpret_cast<Enfusion::CWinEngine*>(Globals.m_WinEngine)->GetRight(), reinterpret_cast<Enfusion::CWinEngine*>(Globals.m_WinEngine)->GetBottom());

	pOverlay->SetRenderProcedure(&Render);

	while (pOverlay->Render(false, reinterpret_cast<void*>(&Globals)))
	{
		Sleep(1);
	}
	
	return 0;
}