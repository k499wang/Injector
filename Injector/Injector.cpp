#include <windows.h>
#include <stdio.h>
#include <string>
#include <iostream>

void InjectDLL(const char* processPath, const char* dllPath) {

    // We create a suspended process, and then inject into it ... 
    STARTUPINFOA si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;

    if (!CreateProcessA(processPath, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        printf("CreateProcess failed with error %lu\n", GetLastError());
		std::cout << processPath << std::endl;
        return;
    }

    // We will run loadlibrary A with the paramter of our dll name
    LPVOID loadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

    if (!loadLibraryAddr) {
        printf("Failed to find LoadLibraryA\n");
        return;
    }

    
    LPVOID remoteString = VirtualAllocEx(pi.hProcess, NULL, strlen(dllPath) + 1, MEM_COMMIT, PAGE_READWRITE);

    if (!remoteString) {
        printf("VirtualAllocEx failed with error %lu\n", GetLastError());
        return;
    }

    // We then write the dll path in the remote process
    if (!WriteProcessMemory(pi.hProcess, remoteString, dllPath, strlen(dllPath) + 1, NULL)) {
        printf("WriteProcessMemory failed with error %lu\n", GetLastError());
        return;
    }

    // Create a remote thread running it ... 
    HANDLE hThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, remoteString, 0, NULL);

    if (!hThread) {
        printf("CreateRemoteThread failed with error %lu\n", GetLastError());
        return;
    }


    // Cleaning up everything
    WaitForSingleObject(hThread, INFINITE);

    ResumeThread(pi.hThread);

    CloseHandle(hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <processPath> <dllPath>\n", argv[0]);
        return 1;
    }

    char currentDir[MAX_PATH];
    if (GetModuleFileNameA(NULL, currentDir, MAX_PATH) == 0) {
        printf("Failed to get current directory: %lu\n", GetLastError());
        return 0;
    }

    char* lastBackslash = strrchr(currentDir, '\\'); // last occurance of a chracter of a string, this returns a pointer
    if (lastBackslash == NULL) {
        printf("Invalid directory path\n");
        return 0;
    }

    // Null-terminate the string to get the directory part only


    *lastBackslash = '\0'; // we replace the \\ with a null terminated char, don't we love c style strings ... 

    // Create the full path to the DLL
    std::string path = std::string(currentDir) + "\\" + argv[2];
    std::string proc = std::string(currentDir) + "\\" + argv[1];



	const char* processPath = proc.c_str();
	const char* dllPath = path.c_str();

    InjectDLL(processPath, dllPath);

    return 0;
}
