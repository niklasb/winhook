#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdint>

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

using namespace std;
#include "common.h"
#include "eval_mid.h"

const char *CONNECT_HOST = "127.0.0.1";
const uint16_t CONNECT_PORT = 55555;

HANDLE mainloop_thread;
HINSTANCE self;
SOCKET sock;

void send_message(const char* s) {
    send(sock, s, strlen(s), 0);
    send(sock, "\n", 1, 0);
}
void send_message_f(const char *fmt, ...) {
    char buf[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof buf - 1, fmt, args);
    send_message(buf);
}

void* get_module(const char *library) {
    HMODULE mods[1024];
    DWORD len;
    void *remote_lib = 0;
    HANDLE proc = GetCurrentProcess();
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

struct Hook {
    const char* mod_name_suffix;
    size_t offset;
    void *func_ptr;
    unsigned char saved_entry[5];
    unsigned char *target;
    Hook(const char *mod_name_suffix, size_t offset, void* func_ptr)
    : mod_name_suffix(mod_name_suffix), offset(offset), func_ptr(func_ptr)
    , target(0)
    {}
    bool hook() {
        void *mod = get_module(mod_name_suffix);
        if (!mod)
            return false;
        target = (unsigned char*)mod + offset;
        DWORD old;
        if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old)) {
            send_message_f("[!] cannot set target to writable (VirtualProtect failed): 0x%p", target);
            target = 0;
            return 0;
        }
        uint32_t diff = (unsigned char*)func_ptr - target - 5;
        memcpy(saved_entry, target, 5);
        //send_message_f("[*] overwriting 5 bytes at 0x%p with jmp to 0x%p", target, func_ptr);
        target[0] = 0xe9;
        memcpy(target + 1, &diff, 4);
        return true;
    }
    bool unhook() {
        if (!target)
            return false;
        //send_message_f("[*] restoring 5 bytes at 0x%p: %02x %02x %02x %02x %02x", target, 
        //    saved_entry[0], saved_entry[1], saved_entry[2], saved_entry[3], saved_entry[4]);
        memcpy(target, saved_entry, 5);
        return true;
    }
};

// wow, such hack
#define CALL_ORIG_(f, ...) ((f ## _ptr)((f ## _hook).target))(__VA_ARGS__)
#define CALL_ORIG_VOID(f, ...) do { f ## _hook.unhook(); \
                                    CALL_ORIG_(f, __VA_ARGS__); \
                                    f ## _hook.hook(); } while(0)
#define CALL_ORIG(f, ...) GET_MID(f ## _hook.unhook(), CALL_ORIG_(f, __VA_ARGS__), f ## _hook.hook())
#define DECLARE_HOOK(modname, offset, ret, name, ...) \
    typedef ret (*(name ## _ptr))(__VA_ARGS__); \
    ret name(__VA_ARGS__); \
    Hook name ## _hook(modname, offset, name); \
    ret name(__VA_ARGS__) {
#define DECLARE_HOOK_THISCALL(modname, offset, ret, name, this_t, ...) \
    typedef ret (__thiscall *(name ## _ptr))(this_t this_, __VA_ARGS__); \
    ret __stdcall name(__VA_ARGS__); \
    Hook name ## _hook(modname, offset, name); \
    ret __stdcall name(__VA_ARGS__) { \
        this_t this_; \
        __asm mov this_, ecx;
#define END_HOOK }

template<typename T>
T convert_string(string s) {
    T val; 
    stringstream ss(s); 
    ss >> val;
    return val;
}

#include "cheat.cpp"

void hook_functions() {
    send_message("[*] hooking functions");
    for (int i = 0; i < sizeof hooks / sizeof *hooks; ++i) {
        Hook& h = *hooks[i];
        send_message_f("[*] hooking %s + 0x%08x --> 0x%p", h.mod_name_suffix, h.offset, h.func_ptr);
        if (!h.hook())
            send_message("[!] hook failed!");
        else {
            send_message_f("[*] hook worked, overwrite at address 0x%p", h.target);
        }
    }
}

void unhook_functions() {
    send_message("[*] unhooking functions");
    for (int i = 0; i < sizeof hooks / sizeof *hooks; ++i) {
        Hook& h = *hooks[i];
        send_message_f("[*] unhooking %s + 0x%08x --> 0x%p", h.mod_name_suffix, h.offset, h.func_ptr);
        if (!h.unhook())
            send_message("[!] unhook failed!");
    }
}

bool handle_command(char *cmdline) {
    if (!strcmp(cmdline, "exit"))
        return 0;
    char *pch = strtok(cmdline, " ");
    if (!pch)
        return 1;
    string cmd(pch);
    vector<string> args;
    while (pch) {
        pch = strtok(NULL, " ");
        if (pch && strlen(pch))
            args.emplace_back(pch);
    }
    handle_user_command(cmd, args);
    return 1;
}

DWORD WINAPI suicide(void *_) {
    Sleep(100);
    FreeLibraryAndExitThread(self, 0);
    return 0;
}

void cleanup() {
    unhook_functions();
    closesocket(sock);
}

DWORD WINAPI mainloop(void *_) {
    char buf[2048];
    for(;;) {
        memset(buf, 0, sizeof buf);
        bool finished = 0;
        for (int i = 0; i < sizeof buf - 1; ++i) {
            int len = recv_blocking(sock, buf+i, 1, 0);
            if (len <= 0) {
                finished = 1;
                break;
            }
            if (buf[i] == '\n')
                break;
        }
        if (finished)
            break;
        while (*buf && strchr("\n\r", buf[strlen(buf) - 1]))
            buf[strlen(buf) - 1] = 0;
        if (!handle_command(buf))
            break;
    }
    // for some weird reason, if we call FreeLibraryAndExitThread directly here,
    // the DLL is unloaded but there is still some dangling handle on the DLL
    // file. Workaround seems to be just to do this, I don't know why.
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)suicide, NULL, 0, NULL);
    return 0;
}

BOOL APIENTRY DllMain (HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
    switch (reason) {
      case DLL_PROCESS_ATTACH:
        self = hInst;
        if (tcp_connect(&sock, CONNECT_HOST, CONNECT_PORT)) {
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)suicide, NULL, 0, NULL);
            break;
        }
        hook_functions();
        mainloop_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainloop, NULL, 0, NULL);
        break;

      case DLL_PROCESS_DETACH:
        TerminateThread(mainloop_thread, 0);
        cleanup();
        break;

      case DLL_THREAD_ATTACH:
        break;
      case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}