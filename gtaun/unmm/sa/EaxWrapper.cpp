/**
 * Copyright (C) 2013 MK124
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma warning(disable: 4217)

#include <windows.h>

#include <string>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>

#include <subhook.h>
#include <gtaun/unmm/CoveringMapManager.hpp>
#include <gtaun/unmm/archive/imgv2/ImgV2.hpp>

using namespace std;


#define ORIGINAL_LIB_FILENAME "original_eax.dll"
#define KERNEL32_LIB_FILENAME "Kernel32.dll"

#define UNMM_LOG_FILENAME "UNMM-SA.log"

#define SCOPE_REMOVE_HOOKES \
	SubHook::ScopedRemove createFileHookRemove(&createFileHook); \
	SubHook::ScopedRemove closeHandleHookRemove(&closeHandleHook); \
	SubHook::ScopedRemove readFileHookRemove(&readFileHook); \
	SubHook::ScopedRemove readFileExHookRemove(&readFileExHook); \
	SubHook::ScopedRemove setFilePointerHookRemove(&setFilePointerHook); \
	SubHook::ScopedRemove createFileMappingHookRemove(&createFileMappingHook);


LPCSTR IMPORT_NAMES[] = {"DllCanUnloadNow", "DllGetClassObject", "DllRegisterServer", "DllUnregisterServer", "EAXDirectSoundCreate", "EAXDirectSoundCreate8", "GetCurrentVersion"};

HANDLE WINAPI HookedCreateFileA (LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
BOOL WINAPI HookedCloseHandle (HANDLE hObject);
BOOL WINAPI HookedReadFile (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
BOOL WINAPI HookedReadFileEx (HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);
DWORD WINAPI HookedSetFilePointer (HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
HANDLE WINAPI HookedCreateFileMappingA (HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName);


HINSTANCE dllInstance = nullptr, originalDllInstance = nullptr;
UINT_PTR procAddresses[7] = {0};

SubHook createFileHook, closeHandleHook, readFileHook, readFileExHook, setFilePointerHook, createFileMappingHook;

unordered_map<HANDLE, string> openedFilenames;


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	dllInstance = hinstDLL;
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		originalDllInstance = LoadLibrary(ORIGINAL_LIB_FILENAME);
		if (originalDllInstance == nullptr)
		{
			MessageBox(nullptr, "Cannot load library " ORIGINAL_LIB_FILENAME, "UNMM Error", 0);
			return FALSE;
		}

		for (int i=0; i<sizeof(procAddresses)/sizeof(procAddresses[0]); i++)
		{
			procAddresses[i] = (UINT_PTR) GetProcAddress(originalDllInstance, IMPORT_NAMES[i]);
		}

		HINSTANCE kernel32 = LoadLibrary(KERNEL32_LIB_FILENAME);
		if (originalDllInstance == nullptr)
		{
			MessageBox(nullptr, "Cannot load library " KERNEL32_LIB_FILENAME, "UNMM Error", 0);
			return FALSE;
		}

		void* originalCreateFileA = GetProcAddress(kernel32, "CreateFileA");
		void* originalCloseHandle = GetProcAddress(kernel32, "CloseHandle");
		void* originalReadFile = GetProcAddress(kernel32, "ReadFile");
		void* originalReadFileEx = GetProcAddress(kernel32, "ReadFileEx");
		void* originalSetFilePointer = GetProcAddress(kernel32, "SetFilePointer");
		void* originalCreateFileMapping = GetProcAddress(kernel32, "CreateFileMappingA");

		CloseHandle(kernel32);

		createFileHook.Install((void*) originalCreateFileA, (void*) HookedCreateFileA);
		closeHandleHook.Install((void*) originalCloseHandle, (void*) HookedCloseHandle);
		readFileHook.Install((void*) originalReadFile, (void*) HookedReadFile);
		readFileExHook.Install((void*) originalReadFileEx, (void*) HookedReadFileEx);
		setFilePointerHook.Install((void*) originalSetFilePointer, (void*) HookedSetFilePointer);
		// createFileMappingHook.Install((void*) originalCreateFileMapping, (void*) HookedCreateFileMappingA);

		DeleteFile(UNMM_LOG_FILENAME);
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		createFileHook.Remove();
		closeHandleHook.Remove();
		readFileHook.Remove();
		readFileExHook.Remove();
		setFilePointerHook.Remove();
		// createFileMappingHook.Remove();

		FreeLibrary(originalDllInstance);
	}

	return TRUE;
}

HANDLE WINAPI HookedCreateFileA
(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	SCOPE_REMOVE_HOOKES;

	HANDLE ret = CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (ret != INVALID_HANDLE_VALUE)
	{
		openedFilenames[ret] = lpFileName;
	}

	ofstream logFile(UNMM_LOG_FILENAME, ios::out | ios::app | ios::ate);
	if (logFile)
	{
		logFile << "CreateFileA(" << lpFileName << ", " << dwDesiredAccess << ", " << dwShareMode << ", "
			<< lpSecurityAttributes << ", " << dwCreationDisposition << ", " << dwFlagsAndAttributes << ", "
			<< hTemplateFile << ")=" << ret << ";" << endl;
		logFile.close();
	}

	return ret;
}

BOOL WINAPI HookedCloseHandle(HANDLE hObject)
{
	SCOPE_REMOVE_HOOKES;

	BOOL ret = CloseHandle(hObject);

	auto it = openedFilenames.find(hObject);
	if (it != openedFilenames.end())
	{
		ofstream logFile(UNMM_LOG_FILENAME, ios::out | ios::app | ios::ate);
		if (logFile)
		{
			logFile << "CloseHandle(" << hObject << " " << it->second << ")=" << ret << ";" << endl;
			logFile.close();
		}

		openedFilenames.erase(it);
	}

	return ret;
}

BOOL WINAPI HookedReadFile
(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
	SCOPE_REMOVE_HOOKES;

	ofstream logFile(UNMM_LOG_FILENAME, ios::out | ios::app | ios::ate);
	if (logFile)
	{
		auto it = openedFilenames.find(hFile);
		if (it != openedFilenames.end())
		{
			logFile << "ReadFile(" << hFile << " " << it->second << ", " << lpBuffer << ", " << nNumberOfBytesToRead << ", "
				<< lpNumberOfBytesRead << ", " << lpOverlapped << ");" << endl;
		}
		else
		{
			logFile << "ReadFile(" << hFile << ", " << lpBuffer << ", " << nNumberOfBytesToRead << ", "
				<< lpNumberOfBytesRead << ", " << lpOverlapped << ");" << endl;
		}

		logFile.close();
	}

	return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

BOOL WINAPI HookedReadFileEx
(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	SCOPE_REMOVE_HOOKES;

	ofstream logFile(UNMM_LOG_FILENAME, ios::out | ios::app | ios::ate);
	if (logFile)
	{
		auto it = openedFilenames.find(hFile);
		if (it != openedFilenames.end())
		{
			logFile << "ReadFileEx(" << hFile << " " << it->second << ", " << lpBuffer << ", " << nNumberOfBytesToRead << ", "
				<< ", " << lpOverlapped << ", " << lpCompletionRoutine << ");" << endl;
		}
		else
		{
			logFile << "ReadFileEx(" << hFile << ", " << lpBuffer << ", " << nNumberOfBytesToRead << ", "
				<< ", " << lpOverlapped << ", " << lpCompletionRoutine << ");" << endl;
		}

		logFile.close();
	}

	return ReadFileEx(hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, lpCompletionRoutine);
}

DWORD WINAPI HookedSetFilePointer
(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	SCOPE_REMOVE_HOOKES;

	ofstream logFile(UNMM_LOG_FILENAME, ios::out | ios::app | ios::ate);
	if (logFile)
	{
		auto it = openedFilenames.find(hFile);
		if (it != openedFilenames.end())
		{
			logFile << "SetFilePointer(" << hFile << " " << it->second << ", " << lDistanceToMove << ", "
				<< lpDistanceToMoveHigh << ", " << dwMoveMethod << ");" << endl;
		}
		else
		{
			logFile << "SetFilePointer(" << hFile << ", " << lDistanceToMove << ", "
				<< lpDistanceToMoveHigh << ", " << dwMoveMethod << ");" << endl;
		}

		logFile.close();
	}

	return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

HANDLE WINAPI HookedCreateFileMappingA
(HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName)
{
	SCOPE_REMOVE_HOOKES;

	ofstream logFile(UNMM_LOG_FILENAME, ios::out | ios::app | ios::ate);
	if (logFile)
	{
		auto it = openedFilenames.find(hFile);
		if (it != openedFilenames.end())
		{
			logFile << "CreateFileMappingA(" << hFile << " " << it->second << ", "
				<< lpFileMappingAttributes << ", " << flProtect << ", "
				<< dwMaximumSizeHigh << ", " << dwMaximumSizeLow << ");" << endl;
		}
		else
		{
			logFile << "CreateFileMappingA(" << hFile << " " << it->second << ", "
				<< lpFileMappingAttributes << ", " << flProtect << ", "
				<< dwMaximumSizeHigh << ", " << dwMaximumSizeLow << ");" << endl;
		}

		logFile.close();
	}

	return CreateFileMappingA(hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
}

extern "C" __declspec(naked) void __stdcall DllCanUnloadNow_wrapper() {__asm{jmp procAddresses[0*4]}}
extern "C" __declspec(naked) void __stdcall DllGetClassObject_wrapper() {__asm{jmp procAddresses[1*4]}}
extern "C" __declspec(naked) void __stdcall DllRegisterServer_wrapper() {__asm{jmp procAddresses[2*4]}}
extern "C" __declspec(naked) void __stdcall DllUnregisterServer_wrapper() {__asm{jmp procAddresses[3*4]}}
extern "C" __declspec(naked) void __stdcall EAXDirectSoundCreate_wrapper() {__asm{jmp procAddresses[4*4]}}
extern "C" __declspec(naked) void __stdcall EAXDirectSoundCreate8_wrapper() {__asm{jmp procAddresses[5*4]}}
extern "C" __declspec(naked) void __stdcall GetCurrentVersion_wrapper() {__asm{jmp procAddresses[6*4]}}
