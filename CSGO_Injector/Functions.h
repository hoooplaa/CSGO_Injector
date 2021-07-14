#pragma once

#include "Includes.h"

void Cleanup(const std::string& message) { // Ends program and prints out a message
	std::cout << message << std::endl;
	system("pause");
	ExitProcess(0);
}

std::string RandomString(const size_t length) { // Returns a random string
	std::string out_string{};

	static const char bet[] = { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" };
	srand((unsigned)time(NULL) * 5); // Setting seed of rand using time
	for (int i = 0; i < length; i++) {
		out_string += bet[rand() % (sizeof(bet) - 1)];
	}

	return out_string;
}

template<typename T> T ReadProcessMemory_T(HANDLE processHandle, size_t address) {
	T buffer;
	ReadProcessMemory(processHandle, (LPCVOID)address, &buffer, sizeof(T), NULL);
	return buffer;
}

template<typename T> T WriteProcessMemory_T(HANDLE processHandle, size_t address, T buffer) {
	WriteProcessMemory(processHandle, (LPCVOID)address, &buffer, sizeof(T), NULL);
}

namespace Functions
{
	bool DoesFileExist(const char* name) {
		if (FILE* file = fopen(name, "r")) {
			fclose(file);
			return true;
		}

		return false;
	}

	//===========================================================================================

	DWORD GetProcessId(const char* ProcessName)
	{
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(pe32);

		if (!Process32First(hSnap, &pe32))
			return NULL;

		do {

			if (!strcmp(_bstr_t(pe32.szExeFile), ProcessName))
			{
				CloseHandle(hSnap);
				return pe32.th32ProcessID;
			}

		} while (Process32Next(hSnap, &pe32));

		CloseHandle(hSnap);
		return NULL;
	}

	uintptr_t GetModuleBaseAddress(DWORD pid, const char* modName) {
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
		if (hSnap != INVALID_HANDLE_VALUE) {
			MODULEENTRY32 modEntry;
			modEntry.dwSize = sizeof(modEntry);
			if (Module32First(hSnap, &modEntry)) {
				do {
					if (!strcmp(_bstr_t(modEntry.szModule), modName)) {
						CloseHandle(hSnap);
						return (uintptr_t)modEntry.modBaseAddr;
					}
				} while (Module32Next(hSnap, &modEntry));
			}
		}

		std::cout << "GetModuleBaseAddress() failed" << std::endl;
		return NULL;
	}

	//===========================================================================================

	bool LoadLibraryInject(DWORD ProcessId, const char* Dll)
	{
		if (ProcessId == NULL)
			return false;

		char CustomDLL[MAX_PATH];
		GetFullPathName(Dll, MAX_PATH, CustomDLL, 0);

		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
		LPVOID allocatedMem = VirtualAllocEx(hProcess, NULL, sizeof(CustomDLL), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

		if (!WriteProcessMemory(hProcess, allocatedMem, CustomDLL, sizeof(CustomDLL), NULL))
			return FALSE;

		CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, allocatedMem, 0, 0);

		if (hProcess)
			CloseHandle(hProcess);

		return TRUE;
	}

	//===========================================================================================

	namespace Internal
	{
		LPVOID NTOpenFile = GetProcAddress(LoadLibraryW(L"ntdll"), "NtOpenFile");

		bool ExecuteBypass(HANDLE hProcess)
		{
			if (NTOpenFile) {
				char originalBytes[5];
				memcpy(originalBytes, NTOpenFile, 5);
				if (WriteProcessMemory(hProcess, NTOpenFile, originalBytes, 5, NULL)) {
					std::cout << "VAC Bypassed.." << std::endl;
					return TRUE;
				}

			}

			return FALSE;

		}

		bool Backup(HANDLE hProcess)
		{
			if (NTOpenFile) {
				//So, when I patching first 5 bytes I need to backup them to 0? (I think)
				char Orig[5];
				memcpy(Orig, NTOpenFile, 5);
				WriteProcessMemory(hProcess, NTOpenFile, Orig, 0, 0);
				return TRUE;
			}

			return FALSE;
		}

	}
}