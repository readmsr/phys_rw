#include <Windows.h>
#include <iostream>
#include "communication.h"

int main()
{
	std::cout << "[*] Attaching to driver...\n";
	HANDLE driver_handle = CreateFileA("\\\\.\\reak", GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (driver_handle == INVALID_HANDLE_VALUE)
	{
		std::cout << "[-] Failed to attach to driver.\n";
		Sleep(3000);
		return 1;
	}

	std::cout << "[+] Attached to driver.\n";

	DWORD pid;
	std::cout << "[*] Enter PID: ";
	std::cin >> pid;

	QWORD cr3 = driver::get_cr3(driver_handle, pid);
	std::cout << "[*] Process CR3: " << std::hex << cr3 << "\n";
	uintptr_t base = driver::get_base(driver_handle, pid);
	std::cout << "[*] Process Base: " << std::hex << base << "\n";

	int test = driver::read<int>(driver_handle, 0x2EB7FF33A2C, cr3);
	std::cout << "[*] Test Value: " << std::dec << test << "\n";

	driver::write<int>(driver_handle, 60, 0x2EB7FF33A2C, cr3);

	for (size_t i = 0; i < 20; ++i)
	{
		driver::mouse_move(driver_handle, 25, 25, 0x0);
		Sleep(100);
	}

	std::cin.get();
	std::cin.get();
	return 0;
}