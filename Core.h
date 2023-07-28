#pragma once
#define BYTEn(x, n)   (*((BYTE*)&(x)+n))
#define WORDn(x, n)   (*((WORD*)&(x)+n))
#define DWORDn(x, n)  (*((DWORD*)&(x)+n))

#define BYTE1(x)   BYTEn(x,  1)         // byte 1 (counting from 0)
#define BYTE2(x)   BYTEn(x,  2)
#define BYTE3(x)   BYTEn(x,  3)
#define BYTE4(x)   BYTEn(x,  4)
#define BYTE5(x)   BYTEn(x,  5)
#define BYTE6(x)   BYTEn(x,  6)
#define BYTE7(x)   BYTEn(x,  7)
#define BYTE8(x)   BYTEn(x,  8)
#define BYTE9(x)   BYTEn(x,  9)
#define BYTE10(x)  BYTEn(x, 10)
#define BYTE11(x)  BYTEn(x, 11)
#define BYTE12(x)  BYTEn(x, 12)
#define BYTE13(x)  BYTEn(x, 13)
#define BYTE14(x)  BYTEn(x, 14)
#define BYTE15(x)  BYTEn(x, 15)
#define WORD1(x)   WORDn(x,  1)
#define WORD2(x)   WORDn(x,  2)         // third word of the object, unsigned
#define WORD3(x)   WORDn(x,  3)
#define WORD4(x)   WORDn(x,  4)
#define WORD5(x)   WORDn(x,  5)
#define WORD6(x)   WORDn(x,  6)
#define WORD7(x)   WORDn(x,  7)

namespace Core
{
	bool WorldToScreen(Enfusion::CCamera * pCamera, Math::Vector3D Position, Math::Vector3D& ScreenedPosition)
	{
		ScreenedPosition = Math::Vector3D::ZeroVector;

	//	Enfusion::CCamera* pCamera = reinterpret_cast<Enfusion::CCamera*>(Camera);

		Math::Vector3D temp = Position - pCamera->GetInvertedViewTranslation();

		float x = temp.Dot(pCamera->GetInvertedViewRight());
		float y = temp.Dot(pCamera->GetInvertedViewUp());
		float z = temp.Dot(pCamera->GetInvertedViewForward());

		if (z < 0.1f)
			return false;

		Math::Vector3D Out(
			pCamera->GetViewportSize().X * (1 + (x / pCamera->GetProjectionD1().X / z)),
			pCamera->GetViewportSize().Y * (1 - (y / pCamera->GetProjectionD2().Y / z)),
			z);

		
		ScreenedPosition.X = Out.X;
		ScreenedPosition.Y = Out.Y;
		ScreenedPosition.Z = Out.Z;

		return true;

		//Math::Vector3D temp = Position - pCamera->GetInvertedViewTranslation();

		//Math::Vector3D ViewportSize = read<Math::Vector3D>(Camera + 0x58);

		//float x = temp.Dot(pCamera->GetInvertedViewRight());
		//float y = temp.Dot(pCamera->GetInvertedViewUp());
		//float z = temp.Dot(pCamera->GetInvertedViewForward());

		//if (z > 0.01f)
		//{
		//	ScreenedPosition = Math::Vector3D(ViewportSize.X * (1 + (x / pCamera->GetProjectionD1().X / z)), ViewportSize.Y * (1 - (y / pCamera->GetProjectionD2().Y / z)), z);
		//	//ScreenedPosition = Math::Vector3D(pOverlay->m_height * (1 + (x / pCamera->GetProjectionD1().X / z)), pOverlay->m_width * (1 - (y / pCamera->GetProjectionD2().Y / z)), z);
		//	return true;
		//}
		//return false;
	}

	PBYTE FindPattern_Wrapper(const char* Pattern, DWORD64 ModuleBase)
	{
		//	MUTATE

		//find pattern utils
#define InRange(x, a, b) (x >= a && x <= b) 
#define GetBits(x) (InRange(x, '0', '9') ? (x - '0') : ((x - 'A') + 0xA))
#define GetByte(x) ((BYTE)(GetBits(x[0]) << 4 | GetBits(x[1])))

//get module range
		PBYTE ModuleStart = (PBYTE)ModuleBase;
		if (!ModuleStart)
			return nullptr;
		PIMAGE_NT_HEADERS NtHeader = ((PIMAGE_NT_HEADERS)(ModuleStart + ((PIMAGE_DOS_HEADER)ModuleStart)->e_lfanew));
		PBYTE ModuleEnd = (PBYTE)(ModuleStart + NtHeader->OptionalHeader.SizeOfImage - 0x1000); ModuleStart += 0x1000;

		//scan pattern main
		PBYTE FirstMatch = nullptr;
		const char* CurPatt = Pattern;
		for (; ModuleStart < ModuleEnd; ++ModuleStart)
		{
			bool SkipByte = (*CurPatt == '\?');
			if (SkipByte || *ModuleStart == GetByte(CurPatt))
			{
				if (!FirstMatch) FirstMatch = ModuleStart;
				SkipByte ? CurPatt += 2 : CurPatt += 3;
				if (CurPatt[-1] == 0) return FirstMatch;
			}

			else if (FirstMatch)
			{
				ModuleStart = FirstMatch;
				FirstMatch = nullptr;
				CurPatt = Pattern;
			}
		}

		char szFormatter[256];
		sprintf_s<sizeof(szFormatter)>(szFormatter, "Failed to find signature: {%s}!", Pattern);
		MessageBoxA(GetConsoleWindow(), szFormatter, ERROR, MB_ICONERROR | MB_OK);
		//	MUTATE_END
		return nullptr;
	}

	PBYTE FindPattern(const char* Pattern, DWORD64 ModuleBase, bool AddModuleBase = true)
	{
		IMAGE_DOS_HEADER DosHead = Driver.Read<IMAGE_DOS_HEADER>(ModuleBase);
		IMAGE_NT_HEADERS64 PeHead = Driver.Read<IMAGE_NT_HEADERS64>(ModuleBase + DosHead.e_lfanew);

		SIZE_T SizeOfMod = PeHead.OptionalHeader.SizeOfImage;

		LPVOID Buff = VirtualAlloc(0, SizeOfMod, MEM_COMMIT, PAGE_READWRITE);
		Driver.ReadArr(ModuleBase, Buff, SizeOfMod);
		

		PBYTE Temp = FindPattern_Wrapper(Pattern, (DWORD64)Buff);

		if (!Temp)
		{
			VirtualFree(Buff, 0, MEM_RELEASE);
			return 0;
		}

		TRACE("[DBG] %s -> (%s) %p", __FUNCTION__, Pattern, Temp);

		VirtualFree(Buff, 0, MEM_RELEASE);

		return (PBYTE)((DWORD64)(AddModuleBase ? ModuleBase : 0) + ((DWORD64)Temp - (DWORD64)Buff));
	}

	bool IsValidPointer(void* m_pPointer)
	{
		if (reinterpret_cast<DWORD64>(m_pPointer) <= 0x400000 || reinterpret_cast<DWORD64>(m_pPointer) >= 0x7fffffffffff)
			return false;

		return true;
	}

	__int64 __fastcall strcmp_imp(unsigned __int8* a1, unsigned __int64 a2)
	{
		__int64 v2; // r9
		unsigned __int8 v3; // al
		unsigned __int8 v4; // dl
		__int64 v5; // rax
		unsigned __int64 v6; // rdx
		__int64 result; // rax
		unsigned __int64 v8; // rdx
		unsigned __int64 v9; // rdx
		unsigned int v10; // edx

		v2 = a2 - (DWORD64)a1;
		if (((unsigned __int8)a1 & 7) != 0)
		{
			while (1)
			{
			_loc_180090B3B:
				v3 = *a1;
				v4 = a1[v2];
				if (*a1 != v4)
					return -(__int64)(*a1 < v4) - ((*a1 < v4) - 1i64);
				++a1;
				if (!v3)
					break;
				if (((unsigned __int8)a1 & 7) == 0)
					goto _loc_180090B60;
			}
			result = 0i64;
		}
		else
		{
			while (1)
			{
			_loc_180090B60:
				if ((((WORD)v2 + (WORD)a1) & 0xFFFu) > 0xFF8)
					goto _loc_180090B3B;
				v5 = *(DWORD64*)a1;
				v6 = *(DWORD64*)&a1[v2];
				if (*(DWORD64*)a1 != v6)
					goto _loc_180090B3B;
				a1 += 8;
				if ((((v6 + 0x7EFEFEFEFEFEFEFFi64) ^ ~v5) & 0x8101010101010100ui64) != 0)
				{
					if (!(BYTE)v6)
						break;
					if (!BYTE1(v6))
						break;
					v8 = v6 >> 16;
					if (!(BYTE)v8)
						break;
					if (!BYTE1(v8))
						break;
					v9 = v8 >> 16;
					if (!(BYTE)v9)
						break;
					if (!BYTE1(v9))
						break;
					v10 = WORD1(v9);
					if (!(BYTE)v10 || !BYTE1(v10))
						break;
				}
			}
			result = 0i64;
		}
		return result;
	}


	int strcmp(const char* m_szStr1, const char* m_szStr2)
	{
		return strcmp_imp((unsigned char*)(m_szStr1), (unsigned __int64)(m_szStr2));
	}

	std::string WideToMultibyte(const std::wstring& str)
	{
		if (str.empty())
			return {};

		int str_len = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);

		std::string out;
		out.resize(str_len);

		WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &out[0], str_len, nullptr, nullptr);

		return out;
	}
}