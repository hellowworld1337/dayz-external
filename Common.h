#pragma once
#pragma warning(disable: 4244)
#pragma warning(disable: 4477)
#pragma warning(disable: 6066)

#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <string>
#include <memory>
#include <vector>

#include <Windows.h>
#include <winternl.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <Shlobj.h>

#include "Driver/Driver.h"

#pragma comment(lib, "ntdll.lib")
#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dwmapi.lib" )

#define GAME_WINDOW "DayZ"
#define RVA(Instr, InstrSize) ((DWORD64)Instr + InstrSize + Driver.Read<long>((DWORD64)Instr + (InstrSize - sizeof(LONG))))
#define TRACE(String, ...) { printf("[CRACK CRACK CRACK] "); printf(String, __VA_ARGS__); putchar(10); }

#define WAIT_FOR_PTR(Ptr) while(!Ptr) /*Ptr = read<QWORD>(Ptr)*/Sleep(100);

#define NOTRACE 0
#define TRACEERRORS 1
#define TRACEWARNS 2
#define TRACEINFO 3
#define TRACEALL 4

using QWORD = DWORD64;

#define TRACELVL 1