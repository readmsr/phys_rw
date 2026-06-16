#pragma once
#include <ntifs.h>

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned __int64 QWORD;

namespace mem
{
	PEPROCESS get_eprocess_by_pid(HANDLE pid);
	NTSTATUS read_phys(QWORD phys_addr, PVOID buffer, SIZE_T size);
	NTSTATUS write_phys(QWORD phys_addr, PVOID buffer, SIZE_T size);
	QWORD virt_to_phys(QWORD cr3, QWORD virt_addr);
	NTSTATUS read_virtual(QWORD cr3, QWORD target_va, PVOID buffer, SIZE_T size);
	NTSTATUS write_virtual(QWORD cr3, QWORD target_va, PVOID buffer, SIZE_T size);
}