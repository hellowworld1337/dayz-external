#pragma once
#include "../Common.h"
#include "../hash.h"

class CDriverAPI
{
	enum IO_CMD : ULONG
	{
		CMD_NONE = 0x000000,
		CMD_READ_VM = 0x200000,
		CMD_WRITE_VM = 0x300000,
		CMD_WRITE_PROT = 0x400000,
		CMD_WRITE_RO_VM = 0x500000,
		CMD_GET_MOD_INFO = 0x600000,
		CMD_ALLOC_VM = 0x700000,
		CMD_FREE_VM = 0x800000,
		CMD_CHECKSTATUS = 0x900000
	};

	/*struct MODINFO
	{
		ULONG64 ModBase;
		ULONG ModSize;
	};*/

	struct IOCOMMAND
	{
		ULONG NameHash;
		ULONG ProcessID;
		ULONG CommandID;
		ULONG64 Src, Dst;
		ULONG CopySize;
	};
public:
	struct MODINFO
	{
		ULONG64 ModBase;
		ULONG ModSize;
	};

	CDriverAPI() = default;
	CDriverAPI(ULONG PID);

	
	template <typename T>
	T Read(uintptr_t Addr)
	{
		T ReadData{}; IOCOMMAND IO_Data;
		IO_Data.NameHash = 'LAST';
		IO_Data.CommandID = IO_CMD::CMD_READ_VM;
		IO_Data.ProcessID = this->m_PID;
		IO_Data.Src = (ULONG64)Addr;
		IO_Data.Dst = (ULONG64)&ReadData;
		IO_Data.CopySize = sizeof(T);

		NtQuerySystemInformation((_SYSTEM_INFORMATION_CLASS)76, &IO_Data, sizeof(IO_Data), &IO_Data.ProcessID);
		//MemoryAPI::SysCall(0x0036, (_SYSTEM_INFORMATION_CLASS)76, &IO_Data, sizeof(IO_Data), &IO_Data.ProcessID);

		return ReadData;
	}

	void ReadArr(uintptr_t Addr, PVOID Data, ULONG Size);
	void WriteArr(uintptr_t Addr, PVOID Data, ULONG Size);

	template <typename T>
	__declspec(noinline) void Write(uintptr_t Addr, T Data)
	{
		IOCOMMAND IO_Data;
		IO_Data.NameHash = 'LAST';
		IO_Data.CommandID = IO_CMD::CMD_WRITE_VM;
		IO_Data.ProcessID = this->m_PID;
		IO_Data.Src = (ULONG64)&Data;
		IO_Data.Dst = (ULONG64)Addr;
		IO_Data.CopySize = sizeof(T);

		NtQuerySystemInformation((_SYSTEM_INFORMATION_CLASS)76, &IO_Data, sizeof(IO_Data), &IO_Data.ProcessID);
		//MemoryAPI::SysCall(0x0036, (_SYSTEM_INFORMATION_CLASS)76, &IO_Data, sizeof(IO_Data), &IO_Data.ProcessID);
	}

	template <typename T>
	void WriteProtected(uintptr_t Addr, T Data)
	{
		IOCOMMAND IO_Data;
		IO_Data.NameHash = 'LAST';
		IO_Data.CommandID = IO_CMD::CMD_WRITE_PROT;
		IO_Data.ProcessID = this->m_PID;
		IO_Data.Src = (ULONG64)&Data;
		IO_Data.Dst = (ULONG64)Addr;
		IO_Data.CopySize = sizeof(T);

		NtQuerySystemInformation((_SYSTEM_INFORMATION_CLASS)76, &IO_Data, sizeof(IO_Data), &IO_Data.ProcessID);
		MemoryAPI::SysCall(0x0036, (_SYSTEM_INFORMATION_CLASS)76, &IO_Data, sizeof(IO_Data), &IO_Data.ProcessID);
	}

	template <typename T>
	void WriteProtectedArray(uintptr_t Addr, PVOID Data, ULONG Size);

	MODINFO GetModInfo(const char* ModName);
	uintptr_t Allocate(uintptr_t Addr, DWORD Size);
	void Free(uintptr_t Addr, DWORD Size);
	bool IsDriverLoaded();

	inline void SetPID(ULONG PID)
	{
		this->m_PID = PID;
	}

private:
	ULONG m_PID = 0x0;
};

extern CDriverAPI Driver;
//
//enum class buffer_cmd_t : int
//{
//	ALLOCATE = 0,
//	FREE,
//	WRITE,
//	READ,
//	GET_IMPORT,
//	GET_MODULE,
//	PRESTART,
//	END
//};
//
//enum class buffer_status_t : int
//{
//	SUCCESS = 1,
//	FAILED_TO_ALLOCATE,
//	FAILED_TO_WRITE,
//	FAILED_TO_READ,
//	FAILED_TO_GET_MODULE_BASE,
//	FAILED_TO_GET_PROC_ADDRESS
//};
//
//__declspec(align(8)) struct buffer_start_t
//{
//	buffer_cmd_t m_cmd_type;
//	buffer_status_t m_status;
//	uint64_t m_process_hash;
//};
//
//struct buffer_allocate_t
//{
//	buffer_start_t m_begin;
//	uintptr_t m_out_address;
//	size_t m_alloc_size;
//};
//
//struct buffer_free_t
//{
//	buffer_start_t m_begin;
//	uintptr_t m_free_address;
//};
//
//struct buffer_read_t
//{
//	buffer_start_t m_begin;
//	uintptr_t m_address;
//	uintptr_t m_buffer;
//	size_t m_size;
//};
//
//struct buffer_write_t
//{
//	buffer_start_t m_begin;
//	uintptr_t m_address;
//	uintptr_t m_buffer;
//	size_t m_size;
//};
//
//struct buffer_get_import_address_t
//{
//	buffer_start_t m_begin;
//	uint64_t m_module_hash;
//	uint64_t m_import_hash;
//	uintptr_t m_out_address;
//};
//
//struct buffer_get_module_t
//{
//	buffer_start_t m_begin;
//	uint64_t m_module_hash;
//	uintptr_t m_out_address;
//};
//
//uintptr_t g_shared;
//
//void run_driver_command(void* buffer, size_t size)
//{
//	std::memcpy(reinterpret_cast<void*>(g_shared + 8), buffer, size);
//
//	_InterlockedExchange64(reinterpret_cast<volatile LONG64*>(g_shared), HASHHH("pending"));
//
//	while (_InterlockedOr64(reinterpret_cast<volatile LONG64*>(g_shared), 0) != HASHHH("done"))
//		Sleep(100);
//
//	std::memcpy(buffer, reinterpret_cast<void*>(g_shared + 8), size);
//}
//
//template< typename _ty >
//_ty read(uintptr_t address)
//{
//	buffer_read_t buf_read;
//	_ty ret;
//	std::memset(&buf_read, 0, sizeof(buf_read));
//	std::memset(&ret, 0, sizeof(ret));
//
//	buf_read.m_begin.m_cmd_type = buffer_cmd_t::READ;
//	buf_read.m_address = address;
//	buf_read.m_buffer = (uintptr_t)(&ret);
//	buf_read.m_size = sizeof(_ty);
//
//	run_driver_command(&buf_read, sizeof(buf_read));
//
//	return ret;
//}
//void ReadArr(PVOID address, PVOID buff, uintptr_t size)
//{
//	buffer_read_t buf_read;
//
//	std::memset(&buf_read, 0, sizeof(buf_read));
//
//	buf_read.m_begin.m_cmd_type = buffer_cmd_t::READ;
//	buf_read.m_address = (uintptr_t)address;
//	buf_read.m_buffer = (ULONG64)buff;
//	buf_read.m_size = size;
//
//	run_driver_command(&buf_read, sizeof(buf_read));
//}
//template< typename _ty >
//void write(uintptr_t address, _ty buf)
//{
//	buffer_write_t buf_write;
//	std::memset(&buf_write, 0, sizeof(buf_write));
//
//	buf_write.m_begin.m_cmd_type = buffer_cmd_t::WRITE;
//	buf_write.m_address = address;
//	buf_write.m_buffer = (uintptr_t)(&buf);
//	buf_write.m_size = sizeof(_ty);
//
//	run_driver_command(&buf_write, sizeof(buf_write));
//}
//void WriteArr(PVOID address, PVOID buff, uintptr_t size)
//{
//	buffer_write_t buf_write;
//	std::memset(&buf_write, 0, sizeof(buf_write));
//
//	buf_write.m_begin.m_cmd_type = buffer_cmd_t::WRITE;
//	buf_write.m_address = (uintptr_t)address;
//	buf_write.m_buffer = (ULONG64)buff;
//	buf_write.m_size = size;
//
//	run_driver_command(&buf_write, sizeof(buf_write));
//}
//
//uintptr_t Allocate(size_t size)
//{
//	buffer_allocate_t buf_alloc;
//	std::memset(&buf_alloc, 0, sizeof(buf_alloc));
//
//	buf_alloc.m_begin.m_cmd_type = buffer_cmd_t::ALLOCATE;
//	buf_alloc.m_alloc_size = size;
//
//	run_driver_command(&buf_alloc, sizeof(buf_alloc));
//
//	return buf_alloc.m_out_address;
//}
//
//void Free(uintptr_t ptr)
//{
//	buffer_free_t buf_free;
//	std::memset(&buf_free, 0, sizeof(buf_free));
//
//	buf_free.m_begin.m_cmd_type = buffer_cmd_t::FREE;
//	buf_free.m_free_address = ptr;
//
//	run_driver_command(&buf_free, sizeof(buf_free));
//}
//
//uintptr_t get_module_base(uint64_t hash)
//{
//	buffer_get_module_t buf_mod;
//	std::memset(&buf_mod, 0, sizeof(buf_mod));
//
//	buf_mod.m_begin.m_cmd_type = buffer_cmd_t::GET_MODULE;
//	buf_mod.m_module_hash = hash;
//
//	run_driver_command(&buf_mod, sizeof(buf_mod));
//
//	return buf_mod.m_out_address;
//}