#include "mem.h"

PEPROCESS mem::get_eprocess_by_pid(HANDLE pid)
{
	DWORD pid_offset = *(DWORD*)((BYTE*)PsGetProcessId + 3);
	DWORD apl_offset = pid_offset + 8;

	PEPROCESS system_proc = PsInitialSystemProcess;
	PEPROCESS current_entry = system_proc;

	do
	{
		HANDLE current_pid = *(HANDLE*)((BYTE*)current_entry + pid_offset);
		if (current_pid == pid)
			return current_entry;

		PLIST_ENTRY next = (PLIST_ENTRY)((BYTE*)current_entry + apl_offset);
		current_entry = (PEPROCESS)((BYTE*)next->Flink - apl_offset);
	} while (current_entry != system_proc);

	return nullptr;
}

NTSTATUS mem::read_phys(QWORD phys_addr, PVOID buffer, SIZE_T size)
{
	if (!phys_addr || !buffer)
		return STATUS_UNSUCCESSFUL;

	MM_COPY_ADDRESS addr;
	addr.PhysicalAddress.QuadPart = phys_addr;
	SIZE_T bytes_read = 0;

	return MmCopyMemory(buffer, addr, size, MM_COPY_MEMORY_PHYSICAL, &bytes_read);
}

NTSTATUS mem::write_phys(QWORD phys_addr, PVOID buffer, SIZE_T size)
{
	if (!phys_addr || !buffer)
		return STATUS_UNSUCCESSFUL;

	PHYSICAL_ADDRESS addr;
	addr.QuadPart = phys_addr;

	PVOID mapped = MmMapIoSpace(addr, size, MmNonCached);
	if (!mapped)
		return STATUS_UNSUCCESSFUL;

	memcpy(mapped, buffer, size);
	MmUnmapIoSpace(mapped, size);

	return STATUS_SUCCESS;
}

QWORD mem::virt_to_phys(QWORD cr3, QWORD virt_addr)
{
	QWORD pml4_idx = (virt_addr >> 39) & 0x1FF;
	QWORD pdpt_idx = (virt_addr >> 30) & 0x1FF;
	QWORD pd_idx   = (virt_addr >> 21) & 0x1FF;
	QWORD pt_idx   = (virt_addr >> 12) & 0x1FF;
	QWORD offset   = virt_addr & 0xFFF;

	QWORD pml4e = 0;
	if (!NT_SUCCESS(read_phys(cr3 + (pml4_idx * 8), &pml4e, sizeof(QWORD))) || !(pml4e & 1))
		return 0;

	QWORD pdpte = 0;
	if (!NT_SUCCESS(read_phys((pml4e & 0xFFFFFFFFFF000) + (pdpt_idx * 8), &pdpte, sizeof(QWORD))) || !(pdpte & 1))
		return 0;

	QWORD pde = 0;
	if (!NT_SUCCESS(read_phys((pdpte & 0xFFFFFFFFFF000) + (pd_idx * 8), &pde, sizeof(QWORD))) || !(pde & 1))
		return 0;

	if (pde & 0x80)
		return (pde & 0xFFFFFFFE00000) + (virt_addr & 0x1FFFFF);

	QWORD pte = 0;
	if (!NT_SUCCESS(read_phys((pde & 0xFFFFFFFFFF000) + (pt_idx * 8), &pte, sizeof(QWORD))) || !(pte & 1))
		return 0;

	return (pte & 0xFFFFFFFFFF000) + offset;
}

NTSTATUS mem::read_virtual(QWORD cr3, QWORD target_va, PVOID buffer, SIZE_T size)
{
	SIZE_T total_read = 0;
	SIZE_T to_read = size;
	QWORD current_va = target_va;
	BYTE* current_buffer = (BYTE*)buffer;

	while (to_read > 0)
	{
		QWORD phys = virt_to_phys(cr3, current_va);
		if (!phys)
			break;

		SIZE_T page_remaining = 0x1000 - (phys & 0xFFF);
		SIZE_T chunk = (to_read > page_remaining) ? page_remaining : to_read;

		if (!NT_SUCCESS(read_phys(phys, current_buffer, chunk)))
			break;

		current_va += chunk;
		current_buffer += chunk;
		to_read -= chunk;
		total_read += chunk;
	}

	return (total_read == size) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS mem::write_virtual(QWORD cr3, QWORD target_va, PVOID buffer, SIZE_T size)
{
	SIZE_T total_written = 0;
	SIZE_T to_write = size;
	QWORD current_va = target_va;
	BYTE* current_buffer = (BYTE*)buffer;

	while (to_write > 0)
	{
		QWORD phys = virt_to_phys(cr3, current_va);
		if (!phys)
			break;

		SIZE_T page_remaining = 0x1000 - (phys & 0xFFF);
		SIZE_T chunk = (to_write > page_remaining) ? page_remaining : to_write;

		if (!NT_SUCCESS(write_phys(phys, current_buffer, chunk)))
			break;

		current_va += chunk;
		current_buffer += chunk;
		to_write -= chunk;
		total_written += chunk;
	}

	return (total_written == size) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


