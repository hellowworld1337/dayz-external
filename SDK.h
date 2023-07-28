#pragma once

namespace M
{
	__forceinline const char* ReadASCII(QWORD Address)
	{
		char Data[64];
		Driver.ReadArr(Address, Data, 64);

		return Data;
	}

	/*__forceinline const char**/std::string ReadArmaString(QWORD Address)
	{
		//std::string String = {};
		int Len = Driver.Read<int>(Address + Offsets::ArmaString::Length);

	//	auto buffer = std::make_unique<char[]>(Len);
	//	ReadArr(Address + 0x10, buffer.get(), Len);
	//	char Data[64];

		if (Len > 120)
		{
			TRACE("[WARN] Too long str len > 64 (%d)\n", Len);
			return {};
		}

		//char* pBuffer = new char[Len + 1];
		char Buffer[120] = {};
		//RtlZeroMemory(pBuffer, Len);
		Driver.ReadArr(Address + Offsets::ArmaString::Data, Buffer, Len);
		//String.reserve(Len);

		//Driver.ReadArr(Address + Offsets::ArmaString::Data, (PVOID)String.data(), Len);

		return std::string(Buffer, Len);
	}

	template <typename T>
	__forceinline T ReadChain(std::initializer_list<uint64_t> Offsets, bool ReadFirstOffset = true)
	{
		auto Start = Offsets.begin();
		size_t ReadSize = Offsets.size();
		uint64_t LastPtr = read<uint64_t>((ReadFirstOffset ? read<uint64_t>(Start[0]) : Start[0]) + Start[1]);
		for (size_t i = 2; i < ReadSize - 1; i++)
			if (!(LastPtr = read<uint64_t>(LastPtr + Start[i])))
				return T{};
		return read<T>(LastPtr + Start[ReadSize - 1]);
	}

	template <typename T>
	__forceinline bool WriteChain(std::initializer_list<uint64_t> Offsets, T Value, bool ReadFirstOffset = true)
	{
		auto Start = Offsets.begin();
		size_t ReadSize = Offsets.size();
		uint64_t LastPtr = read<uint64_t>((ReadFirstOffset ? read<uint64_t>(Start[0]) : Start[0]) + Start[1]);
		for (size_t i = 2; i < ReadSize - 1; i++)
			if (!(LastPtr = read<uint64_t>(LastPtr + Start[i])))
				return false;
		Driver.Write(LastPtr + Start[ReadSize - 1], Value);
	}
}

namespace Enfusion
{
	class CCamera;
	class CEntity;
	class CSortedObject;

	//NOTE: custom wrapper
	template <typename T>
	class CList
	{
	public:
		CList(QWORD dwListBase, QWORD dwSizeOffset = sizeof(PVOID)) :
			m_dwListBase(dwListBase), m_dwSizeOffset(dwSizeOffset)
		{}

		QWORD GetBase()
		{
			return m_dwListBase;
		}

		uint32_t GetSize()
		{
//			printf("[DBG] [%s] 0x%p\n", __FUNCSIG__, this->m_dwListBase + this->m_dwSizeOffset);
			return Driver.Read<uint32_t>(this->m_dwSizeOffset);
		}

		T* Get(DWORD32 Position)
		{
			/*
				ѕриведение типов без проверки. 
				reinterpret_cast Ч непосредственное указание компил€тору. 
				ѕримен€етс€ только в случае полной уверенности программиста в собственных действи€х. 
				Ќе снимает константность и volatile. примен€етс€ дл€ приведени€ указател€ к указателю, указател€ к целому и наоборот.
			*/

			QWORD Element = Driver.Read<QWORD>(this->m_dwListBase + (Position * 0x8));
			return reinterpret_cast<T*>(Element);
		}

		T* GetLoot(DWORD32 Position)
		{
			DWORD32 ValidCheck = Driver.Read<DWORD32>(this->m_dwListBase + (Position * 0x18));

			if (ValidCheck == 1)
				return Driver.Read<T*>(this->m_dwListBase + (Position * 0x18) + 0x8);

			return nullptr;
		//	return reinterpret_cast<T*>(Element);
		}

	private:
		QWORD m_dwListBase = 0x0ull;
		QWORD m_dwSizeOffset = 0x0ull;
	};

	class CPlayerIdentity
	{
	public:
		std::uint32_t GetNetworkID()
		{
			return Driver.Read<std::uint32_t>((QWORD)this + Offsets::PlayerIdentity::NetworkID);
		}

		std::string GetPlayerName()
		{
			QWORD ArmaSring = Driver.Read<QWORD>((QWORD)this + Offsets::PlayerIdentity::PlayerName);
			return M::ReadArmaString(ArmaSring);
		}
	};

	class CWorld
	{
	public:
		CCamera* GetCamera()
		{
			return Driver.Read<CCamera*>((QWORD)this + Offsets::World::Camera);
		}

		CSortedObject* GetCameraOn()
		{
			return Driver.Read<CSortedObject*>((QWORD)this + Offsets::World::CameraOn);
		}

		// Near:
		QWORD GetNearAnimalTablePtr()
		{
			return Driver.Read<QWORD>((QWORD)this + Offsets::World::NearAnimalTable);
		}

		QWORD GetNearAnimalTableSize()
		{
			return /*Driver.Read<DWORD32>*/((QWORD)this + Offsets::World::NearAnimalTable + sizeof(PVOID));
		}
		// Far:
		QWORD GetFarAnimalTablePtr()
		{
			return Driver.Read<QWORD>((QWORD)this + Offsets::World::FarAnimalTable);
		}

		QWORD GetFarAnimalTableSize()
		{
			return /*Driver.Read<DWORD32>*/((QWORD)this + Offsets::World::FarAnimalTable + sizeof(PVOID));
		}

		// Slow:
		QWORD GetSlowAnimalTablePtr()
		{
			return Driver.Read<QWORD>((QWORD)this + Offsets::World::SlowAnimalTable);
		}

		QWORD GetSlowAnimalTableSize()
		{
			return /*Driver.Read<QWORD>*/((QWORD)this + Offsets::World::SlowAnimalTable + sizeof(PVOID));
		}

		QWORD GetItemTablePtr(DWORD64 Index)
		{
			return Driver.Read<QWORD>((QWORD)this + Offsets::World::ItemTable + (Index * 0x20));
		}

		QWORD GetItemTableSize(DWORD64 Index)
		{
			return /*Driver.Read<QWORD>*/((QWORD)this + Offsets::World::ItemTable + (Index * 0x20) + sizeof(PVOID));
		}

		QWORD GetBulletTablePtr()
		{
			return Driver.Read<QWORD>((QWORD)this + Offsets::World::BulletTable);
		}

		QWORD GetBulletTableSize()
		{
			return /*Driver.Read<QWORD>*/((QWORD)this + Offsets::World::BulletTable + sizeof(PVOID));
		}
	};

	class CSortedObject
	{
	public:
		CEntity* GetEntity()
		{
			return Driver.Read<CEntity*>((QWORD)this + 0x8);
			//return reinterpret_cast<CEntity*>((DWORD64)this - 0xA8);
		}

		CEntity* GetLocal()
		{
			// NOTE: Do not read, just cast
			return reinterpret_cast<CEntity*>((QWORD)this->GetEntity() - 0xA8);
		}
	};


	/*
	* std::vector<uint32_t> human_bone_indices = {
		0, 19, 20, 21, 24,	// pelvis to head
		94, 97, 99,			// left arm
		61, 63, 65,			// right arm
		9, 12, 14, 16,		// left leg
		1, 4, 6, 8			// right leg
	};
 
	std::vector<uint32_t> zombie_bone_indices = 
	{
		0, 15, 16, 19, 21,	// pelvis to head
		56, 59, 60,			// left arm
		24, 25, 27,			// right arm
		9, 12, 13, 14,		// left leg
		1, 4, 6, 7			// right leg
	};
	*/

	class CDayZPlayerType
	{
	public:
		std::string GetTypeName()
		{
			QWORD Pointer = Driver.Read<QWORD>((QWORD)this + Offsets::PlayerType::TypeName);
			return M::ReadArmaString(Pointer);
		}

		std::string GetModelName()
		{
			QWORD Pointer = Driver.Read<QWORD>((QWORD)this + Offsets::PlayerType::ModelName);
			return M::ReadArmaString(Pointer);
		}

		std::string GetConfigName()
		{
			QWORD Pointer = Driver.Read<QWORD>((QWORD)this + Offsets::PlayerType::ConfigName);
			return M::ReadArmaString(Pointer);
		}

		std::string GetCleanName()
		{
			QWORD Pointer = Driver.Read<QWORD>((QWORD)this + Offsets::PlayerType::CleanName);
			return M::ReadArmaString(Pointer);
		}
	};

	class CEntityVisualState
	{
	public:
		Math::Vector3D GetPosition()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::VisualState::Location);
		}

		Math::Vector3D GetHeadPosition()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::VisualState::HeadPosition);
		}

		void SetPosition(Math::Vector3D Pos)
		{
			Driver.Write<Math::Vector3D>((QWORD)this + Offsets::VisualState::Location, Pos);
		}

	};

	class CInventory
	{
	public:
		CEntity* GetItemInHands()
		{
			return Driver.Read<CEntity*>((QWORD)this + Offsets::Inventory::ItemInHands);
		}
	};


	class CEntity
	{
	public:
		CEntity* GetBaseEntity()
		{
			return /*read*/reinterpret_cast<CEntity*>((QWORD)this - 0xA8);
		}

		CDayZPlayerType* GetRendererEntityType()
		{
			return Driver.Read<CDayZPlayerType*>((QWORD)this + Offsets::Entity::RendererEntityType);
		}

#if 0 // Siggied
		QWORD GetSkeleton()
		{
			QWORD PlayerSkeleton = read<QWORD>((QWORD)this + Offsets::PlayerSkeleton);

			if (!PlayerSkeleton) // Zombie?
			{
				QWORD ZombieSkeleton = read<QWORD>((QWORD)this + Offsets::ZombieSkeleton);
				if (ZombieSkeleton) return ZombieSkeleton;
			}
			else
				return PlayerSkeleton;

			return NULL;
		}
#endif

		CEntityVisualState* GetRendererVisualState()
		{
			return Driver.Read<CEntityVisualState*>((QWORD)this + Offsets::Entity::VisualState);
		}

		CEntityVisualState* GetFutureVisualState()
		{
			return Driver.Read<CEntityVisualState*>((QWORD)this + Offsets::Entity::FutureVisualState);
		}

		bool IsDead()
		{
			return Driver.Read<bool>((QWORD)this + Offsets::Entity::IsDamagedOrDestroyed);
		}

		DWORD32 GetNetworkID()
		{
			return Driver.Read<DWORD32>((QWORD)this + Offsets::Entity::NetworkID);
		}

		CInventory* GetInventory()
		{
			return Driver.Read<CInventory*>((QWORD)this + Offsets::Entity::Inventory);
		}
	};

	class CNetworkClient
	{
	public:
		QWORD GetScoreBoard()
		{
			return Driver.Read<QWORD>((QWORD)this + Offsets::NetworkClient::ScoreBoard);
		}

		CPlayerIdentity* GetPlayerIdentity(std::uint32_t dwNetworkID)
		{
			void* pScoreBoard = reinterpret_cast<void*>(this->GetScoreBoard());
			if (!pScoreBoard)
				return nullptr;

			for (std::uint64_t i = 0; i < this->GetPlayersCount(); i++)
			{
				CPlayerIdentity* pIdentity = /*read*/reinterpret_cast<CPlayerIdentity*>((DWORD64)pScoreBoard + (i * 0x160));
				if (!pIdentity)
					continue;

			//	TRACE("[FACE] [DBG] pIdentity: 0x%p [%d]\n", pIdentity, this->GetPlayersCount());

				if (pIdentity->GetNetworkID() == dwNetworkID)
					return pIdentity;
			}

			return nullptr;
		}

		std::string GetServerName()
		{
			QWORD ArmaString = Driver.Read<QWORD>((QWORD)this + Offsets::NetworkClient::ServerName);
			return M::ReadArmaString(ArmaString);
		}

		std::uint32_t GetPlayersCount()
		{
			return Driver.Read<std::uint32_t>((QWORD)this + Offsets::NetworkClient::PlayersCount);
		}

		std::string GetPlayerName(CEntity* pEntity)
		{
			CPlayerIdentity* pIdentity = GetPlayerIdentity(pEntity->GetNetworkID());

			if (!pIdentity)
				return {};

			return pIdentity->GetPlayerName();
		}

	};

	class CCamera
	{
	public:
		Math::Vector3D GetInvertedViewTranslation()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::Camera::GetInvertedViewTranslation);
		}

		Math::Vector3D GetInvertedViewRight()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::Camera::GetInvertedViewRight);
		}

		Math::Vector3D GetInvertedViewUp()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::Camera::GetInvertedViewUp);
		}

		Math::Vector3D GetInvertedViewForward()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::Camera::GetInvertedViewForward);
		}

		Math::Vector3D GetViewportSize()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::Camera::GetViewportSize);
		}

		Math::Vector3D GetProjectionD1()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::Camera::GetProjectionD1);
		}

		Math::Vector3D GetProjectionD2()
		{
			return Driver.Read<Math::Vector3D>((QWORD)this + Offsets::Camera::GetProjectionD2);
		}

	};

	class CWinEngine
	{
	public:
		int GetRight()
		{
			return Driver.Read<int>((QWORD)this + Offsets::WinEngine::Right);
		}

		int GetBottom()
		{
			return Driver.Read<int>((QWORD)this + Offsets::WinEngine::Bottom);
		}

		int GetLeft()
		{
			return Driver.Read<int>((QWORD)this + Offsets::WinEngine::Left);
		}

		int GetTop()
		{
			return Driver.Read<int>((QWORD)this + Offsets::WinEngine::Top);
		}

		void* GetInputDevice()
		{
			return Driver.Read<void*>((QWORD)this + Offsets::WinEngine::InputDevice);
		}

		void* GetRenderer()
		{
			return Driver.Read<void*>((QWORD)this + Offsets::WinEngine::Renderer);
		}

		HWND GetWindow()
		{
			return Driver.Read<HWND>((QWORD)this + Offsets::WinEngine::hWnd);
		}
	};
}