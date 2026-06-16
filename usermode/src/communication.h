#pragma once
#include <Windows.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned __int64 QWORD;


namespace driver
{
	namespace codes
	{
		constexpr ULONG get_cr3 = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x888, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG get_base = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x889, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG read = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x88A, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG write = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x88B, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
		constexpr ULONG mouse_move = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x88C, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);
	}

	namespace request
	{
		struct cr3
		{
			HANDLE pid;
			QWORD cr3;
		};

		struct base
		{
			HANDLE pid;
			uintptr_t base;
		};

		struct mem_op
		{
			QWORD target;
			PVOID buffer;
			QWORD cr3;
			SIZE_T size;
		};

		struct mouse_move
		{
			int x;
			int y;
			unsigned short flags;
		};
	}

	QWORD get_cr3(HANDLE driver_handle, DWORD pid)
	{
		request::cr3 req{};
		req.pid = (HANDLE)pid;

		DWORD bytes_returned = 0;
		DeviceIoControl(driver_handle, codes::get_cr3, &req, sizeof(request::cr3), &req, sizeof(request::cr3), &bytes_returned, nullptr);

		return req.cr3;
	}

	uintptr_t get_base(HANDLE driver_handle, DWORD pid)
	{
		request::base req{};
		req.pid = (HANDLE)pid;

		DWORD bytes_returned = 0;
		DeviceIoControl(driver_handle, codes::get_base, &req, sizeof(request::base), &req, sizeof(request::base), &bytes_returned, nullptr);

		return req.base;
	}

	template <class T>
	T read(HANDLE driver_handle, const uint64_t address, const QWORD cr3)
	{
		T temp{};
		
		request::mem_op req{};
		req.target = (QWORD)address;
		req.buffer = &temp;
		req.cr3 = cr3;
		req.size = sizeof(T);
		
		DWORD bytes_returned = 0;
		DeviceIoControl(driver_handle, codes::read, &req, sizeof(request::mem_op), &req, sizeof(request::mem_op), &bytes_returned, nullptr);

		return temp;
	}

	template <class T>
	void write(HANDLE driver_handle, const T& value, const uint64_t address, const QWORD cr3)
	{
		request::mem_op req{};
		req.target = (QWORD)address;
		req.buffer = const_cast<T*>(&value);
		req.cr3 = cr3;
		req.size = sizeof(T);

		DWORD bytes_returned = 0;
		DeviceIoControl(driver_handle, codes::write, &req, sizeof(request::mem_op), nullptr, 0, &bytes_returned, nullptr);
	}

	void mouse_move(HANDLE driver_handle, int x, int y, unsigned short flags)
	{
		request::mouse_move req = {};
		req.x = x;
		req.y = y;
		req.flags = flags;

		DWORD bytes_returned = 0;
		DeviceIoControl(driver_handle, codes::mouse_move, &req, sizeof(request::mouse_move), nullptr, 0, &bytes_returned, nullptr);
		
	}
}
