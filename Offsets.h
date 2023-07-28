#pragma once
#define OFFSET constexpr DWORD

namespace Offsets
{
	// World
	// TODO: namespace World
	namespace World
	{
		OFFSET Camera = 0x1B8;
		OFFSET CameraOn = 0x28B8; // [actual address in first opcode] 4C 89 A7 ? ? ? ? 48 8B 8F ? ? ? ? 48 85 C9 74 0C
		OFFSET NearAnimalTable = 0xEB8;
		OFFSET FarAnimalTable = 0x1000; // 48 8B 92 ? ? ? ? EB 22 
		OFFSET SlowAnimalTable = 0x1F68; // 48 8B 8F ? ? ? ? 48 85 C9 74 4A 
		OFFSET BulletTable = 0xD70; // 48 8D 8F ? ? ? ? E8 ? ? ? ? 48 8D 8F ? ? ? ? E8 ? ? ? ? 48 8D 8F ? ? ? ? E8 ? ? ? ? 48 8D 8F ? ? ? ? E8 ? ? ? ? 48 8D 8F ? ? ? ? E8 ? ? ? ? 48 8D 9F ? ? ? ? 
		OFFSET ItemTable = 0x1FB8;
	}
#if 1
	constexpr DWORD NearEntityList = 0xEB8;
	constexpr DWORD NearEntityTableSize = NearEntityList + 0x08;

	constexpr DWORD FarEntityTable = 0xFF0;
	constexpr DWORD FarEntityTableSize = FarEntityTable + 0x08;
#endif
	namespace Entity
	{
		OFFSET IsDamagedOrDestroyed = 0x15D;
		OFFSET FutureVisualState = 0xF0; // 48 8D 90 ? ? ? ? 49 0F 44 D0 48 8B 02 
		OFFSET VisualState = 0x198; // 48 8B 99 ? ? ? ? 33 C9 
		OFFSET PlayerSkeleton = 0x760;
		OFFSET ZombieSkeleton = 0x5D0;
		OFFSET RendererEntityType = 0x148;
		OFFSET NetworkID = 0x634;
		OFFSET Inventory = 0x5B0;
	}

	namespace Inventory
	{
		OFFSET ItemInHands = 0x1B0;
	}

	namespace PlayerType
	{
		OFFSET ConfigName = 0xA0;
		OFFSET TypeName = 0x68;
		OFFSET ModelName = 0x80;
		OFFSET CleanName = 0x4E0;
	}

	namespace VisualState
	{
		OFFSET Location = 0x2C; // Coordinates
		OFFSET HeadPosition = 0xF8;
	}

	namespace Camera
	{
		OFFSET GetInvertedViewTranslation = 0x2C;
		OFFSET GetInvertedViewRight = 0x8;
		OFFSET GetInvertedViewUp = 0x14;
		OFFSET GetInvertedViewForward = 0x20;
		OFFSET GetViewportSize = 0x58;
		OFFSET GetProjectionD1 = 0xD0;
		OFFSET GetProjectionD2 = 0xDC;
	}

	namespace NetworkClient
	{
		OFFSET ScoreBoard = 0x10;
		OFFSET PlayersCount = 0x18;
		OFFSET ServerName = 0x340; // 330?
	}

	namespace PlayerIdentity
	{
		OFFSET NetworkID = 0x30;
		OFFSET PlayerName = 0xF8;
	}

	namespace ArmaString
	{
		OFFSET Length = 0x8;
		OFFSET Data = 0x10;
	}

	namespace WinEngine
	{
		OFFSET Right = 0x30;
		OFFSET Bottom = 0x34;
		OFFSET Left = 0x38;
		OFFSET Top = 0x3C;
		OFFSET InputDevice = 0x128;
		OFFSET Renderer = 0x130;
		OFFSET hWnd = 0x2E8;
	}
}
