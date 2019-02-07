#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <Windows.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(linker, "/subsystem:windows")

__declspec(noreturn) void panic(const wchar_t* msg, bool from_win32) {
	const wchar_t* source = msg;
	if (from_win32) {
		wchar_t os_error[512];
		if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, GetLastError(), 0, os_error, ARRAYSIZE(os_error), NULL)) {
			wcscpy(os_error, L"Unknown error");
		}
		wchar_t error[1024];
		if (wsprintf(error, L"%s: \"%s\"", msg, os_error) > 0) {
			source = error;
		}
	}
	MessageBoxW(0, source, L"Fatal error", MB_ICONERROR);
	exit(1);
}

#define VERIFY_IMPL(cond, msg, from_win32) do { if (!(cond)) { panic(msg, from_win32); } } while (0)
#define VERIFY(cond, msg) VERIFY_IMPL(cond, msg, false)
#define VERIFY_WIN32(cond, msg) VERIFY_IMPL(cond, msg, true) 

void* read_file(const wchar_t* filename, uint32_t* size) {
	HANDLE handle = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	VERIFY_WIN32(handle != INVALID_HANDLE_VALUE, L"Cannot open file specified");

	LARGE_INTEGER file_size;
	VERIFY_WIN32(GetFileSizeEx(handle, &file_size), L"Cannot get file size");
	VERIFY(file_size.HighPart == 0, L"File is too big");

	void* buffer = malloc(file_size.LowPart);
	VERIFY(buffer, L"Out of memory");

	DWORD nread;
	VERIFY_WIN32(ReadFile(handle, buffer, file_size.LowPart, &nread, NULL), L"Cannot read file");
	VERIFY(nread == file_size.LowPart, L"Cannot read file");

	CloseHandle(handle);
	*size = nread;
	return buffer;
}

void write_file(const wchar_t* filename, void* data, uint32_t size) {
	HANDLE handle = CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	VERIFY_WIN32(handle != INVALID_HANDLE_VALUE, L"Cannot open file specified");

	DWORD nwritten;
	VERIFY_WIN32(WriteFile(handle, data, size, &nwritten, NULL), L"Cannot write file");
	VERIFY(nwritten == size, L"Cannot write file");

	CloseHandle(handle);
}

#define CP_SHIFTJIS 932

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow) {
	int argc = 0;
	const wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	VERIFY_WIN32(argv, L"Unable to get command line arguments");
	VERIFY(argc > 1, L"No arguments were provided.");

	uint32_t source_size;
	void* source = read_file(argv[1], &source_size);
	
	uint32_t in_buffer_size = source_size * sizeof(int);
	void* in_buffer = malloc(in_buffer_size);
	VERIFY(in_buffer, L"Out of memory");
	int nwideconverted = MultiByteToWideChar(CP_SHIFTJIS, 0, (char*)source, source_size, (wchar_t*)in_buffer, in_buffer_size);
	VERIFY_WIN32(nwideconverted > 0, L"Cannot convert characters");

	uint32_t out_buffer_size = nwideconverted * sizeof(int);
	void* out_buffer = malloc(out_buffer_size);
	VERIFY(out_buffer, L"Out of memory");
	int out_written = WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)in_buffer, nwideconverted, (char*)out_buffer, out_buffer_size, NULL, NULL);
	VERIFY_WIN32(out_written > 0, L"Cannot convert characters");

	write_file(argv[1], out_buffer, out_written);
	MessageBoxW(0, L"Finished.", L"Finished", MB_ICONINFORMATION);

	return 0;
}