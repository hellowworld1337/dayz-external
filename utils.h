#pragma once
#include "antiimports.h"
#include <string>
#include "xor.h"
#include "driver.h"

extern "C" NTSYSAPI PIMAGE_NT_HEADERS NTAPI RtlImageNtHeader(PVOID Base);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////425g45g
PVOID get_dll_by_file(LPCWSTR file_path)//
{
	HANDLE h_dll = LI_FIND(CreateFileW)(file_path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h_dll == INVALID_HANDLE_VALUE)
		return NULL;

	DWORD dll_file_sz = LI_FIND(GetFileSize)(h_dll, NULL);
	PVOID dll_buffer = LI_FIND(VirtualAlloc)(NULL, dll_file_sz, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!ReadFile(h_dll, dll_buffer, dll_file_sz, NULL, FALSE) || *(PDWORD)dll_buffer != 9460301)
	{
		LI_FIND(VirtualFree)(dll_buffer, 0, MEM_RELEASE);
		goto exit;
	}

exit:
	CloseHandle(h_dll);
	return dll_buffer;
}

std::wstring to_fast_convert_wchar(PCCH a) // 4f3g345g2
{
	std::wstring out_str;

	for (int i = 0; i < strlen(a) + 1; i++)
		out_str.push_back((const wchar_t)a[i]);

	return out_str;
}

DWORD get_process_id_and_thread_id_by_window_class(LPCSTR window_class_name, PDWORD p_thread_id)
{
	DWORD process_id = 0;
	while (!process_id)
	{
		*p_thread_id = LI_FIND(GetWindowThreadProcessId)(FindWindow(window_class_name, NULL), &process_id); LI_FIND(Sleep)(20);
	} return process_id;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
DWORD get_process_id_by_name(PCSTR name)
{
	DWORD pid = 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 process;
	ZeroMemory(&process, sizeof(process));
	process.dwSize = sizeof(process);

	if (Process32First(snapshot, &process))
	{
		do
		{
			if (std::string(process.szExeFile) == std::string(name))
			{
				pid = process.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &process));
	}

	CloseHandle(snapshot);
	return pid;
}

std::string get_process_name_by_pid(DWORD process_id)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);
	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	for (BOOL bok = Process32First(processesSnapshot, &processInfo); bok; bok = Process32Next(processesSnapshot, &processInfo))
	{
		if (process_id == processInfo.th32ProcessID)
		{
			return processInfo.szExeFile;
		}
	}

	CloseHandle(processesSnapshot);
	return std::string();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
