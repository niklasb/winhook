#include <iostream>
#include <string>
#include <cassert>
#include <cstdint>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

using namespace std;
#include "common.h"

const char *LISTEN_HOST = "127.0.0.1";
uint16_t LISTEN_PORT = 55555;

SOCKET client_connection;

HANDLE proc_by_name(const char *name) {
    PROCESSENTRY32 entry;
    entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (Process32First(snapshot, &entry)) {
        while (Process32Next(snapshot, &entry)) {
            if (stricmp(entry.szExeFile, name) == 0) {
                return OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
            }
        }
    }
    return NULL;
}

void* get_remote_module(HANDLE proc, const char *library) {
    HMODULE mods[1024];
    DWORD len;
    void *remote_lib = 0;
    if (EnumProcessModules(proc, mods, sizeof(mods), &len)) {
        for (int i = 0; i < (len / sizeof(HMODULE)); i++) {
            TCHAR mod_name[MAX_PATH];
            if (GetModuleFileNameEx(proc, mods[i], mod_name, sizeof(mod_name) / sizeof(TCHAR))) {
                if (strlen(mod_name) >= strlen(library) && !stricmp(mod_name + strlen(mod_name) - strlen(library), library))
                    remote_lib = mods[i];
            } else {
                return NULL;
            }
        }
    } else {
        return NULL;
    }
    if (!remote_lib)
        return NULL;
    return remote_lib;
}

const void *remote_resolve(HANDLE proc, const char * library, const char * func) {
    assert(!strchr(library, '\\'));
    HMODULE local_lib = GetModuleHandle(library);
    const void *local_func = (const void*)GetProcAddress(local_lib, func);
    if (!local_lib || !local_func)
        return NULL;
    size_t offset = (const char*)local_func - (const char*)local_lib;

    HMODULE mods[1024];
    DWORD len;
    char lib_suffix[MAX_PATH] = "\\";
    strcpy(lib_suffix + 1, library);
    const void *remote_lib = get_remote_module(proc, lib_suffix);
    if (!remote_lib) 
        return NULL;
    return (const void*)((const char*)remote_lib + offset);
}

const void* copy_to_remote(HANDLE proc, const void *local_buf, size_t len) {
    void *buf = VirtualAllocEx(proc, NULL, len, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!buf) {
        cerr << "[!] VirtualAllocEx failed." << endl;
        return NULL;
    }
    int n = WriteProcessMemory(proc, buf, local_buf, len, NULL);
    if (!n) {
        cerr << "[!] WriteProcessMemory failed." << endl;
        return NULL;
    }
    return buf;
}

bool remote_call(HANDLE proc, const void* func, const void* arg) {
    HANDLE thread = CreateRemoteThread(proc, NULL, 0, (LPTHREAD_START_ROUTINE)func, (void*)arg, 0, NULL);
    if (!thread) {
        cerr << "[!] CreateRemoteThread failed." << endl;
        return false;
    }
    WaitForSingleObject(thread, INFINITE);
    return true;
}

DWORD stdin_to_sock(void *_) {
	char buf[4096];
	while (!cin.eof()) {
		string s;
		getline(cin, s);
		send(client_connection, s.c_str(), s.size(), 0);
		send(client_connection, "\n", 1, 0);
	}
	return 0;
}

DWORD sock_to_stdout(void *_) {
	char buf[4096];
	for (;;) {
		int len = recv_blocking(client_connection, buf, sizeof buf, 0);
		if (len <= 0)
			break;
		cout.write(buf, len);
	}
	return 0;
}

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        cerr << "[!] Usage: " << argv[0] << " [-u] <proc_name> <inject_dll>" << endl;
        return 1;
    }
    bool unload = string(argv[1]) == "-u";
    const char *proc_name = argv[2 - !unload];
    const char *lib_name = argv[3 - !unload];
    
    char library[MAX_PATH];
    if (!GetFullPathName(lib_name, sizeof library, library, NULL)) {
        cerr << "[!] Could not get full path of " << lib_name << endl;
        return 1;
    }
	
	// open process and resolve remote addresses of {Load,Free}Library
    HANDLE proc = proc_by_name(proc_name);
    assert(proc);
    const void *remote_LoadLibrary = remote_resolve(proc, "kernel32.dll", "LoadLibraryA");
    assert(remote_LoadLibrary);
    const void *remote_FreeLibrary = remote_resolve(proc, "kernel32.dll", "FreeLibrary");
    assert(remote_FreeLibrary);
	
	// check if injected DLL is already loaded, if yes unload
    const void *remote_lib = get_remote_module(proc, library);
    if (remote_lib) {
        cout << "[*] Library already injected. Unloading." << endl;
        remote_call(proc, remote_FreeLibrary, remote_lib);
        remote_lib = get_remote_module(proc, library);
        if (remote_lib) {
            cerr << "[!] FreeLibrary failed." << endl;
            return 1;
        }
    } else if (unload) {
        cerr << "[!] Library not loaded." << endl;
        return 1;
    }
    if (unload)
        return 0;
	
	// listen
	SOCKET listen_sock;
	cout << "[*] Binding to " << LISTEN_HOST << ":" << LISTEN_PORT << endl;
	if (tcp_bind(&listen_sock, LISTEN_HOST, LISTEN_PORT)) {
		cout << "[!] Failed!" << endl;
		return 1;
	}
	
	// call LoadLibrary remotely
    const void *arg = copy_to_remote(proc, library, strlen(library) + 1);
    assert(arg);
    cout << "[*] Calling LoadLibrary in the remote process." << endl;
    remote_call(proc, remote_LoadLibrary, arg);
    remote_lib = get_remote_module(proc, library);
    if (!remote_lib) {
        cerr << "[!] Injection failed!" << endl;
        return 1;
    }
	
	// yay!
    cout << "[*] Done. Waiting for connection." << endl;
	if (tcp_accept(listen_sock, &client_connection)) {
		cerr << "[!] accept failed." << endl;
		return 1;
	}
	cout << "[*] Got connection, enjoy!" << endl;
	HANDLE threads[2];
    threads[0] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)stdin_to_sock, NULL, 0, NULL);
    threads[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)sock_to_stdout, NULL, 0, NULL);
	WaitForMultipleObjects(2, threads, 0, INFINITE);
	ExitProcess(0);
}
