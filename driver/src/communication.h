#pragma once
#include <ntifs.h>

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
}