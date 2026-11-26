#include <windows.h>
#include <intrin.h>
#include <stdint.h>
#include "MinHook.h"

namespace hijack
{

#define NOP_FUNC            \
    {                       \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        __nop();            \
        return __COUNTER__; \
    }
// 用 __COUNTER__ 来生成一点不一样的代码，避免被 VS 自动合并相同函数

#define EXPORT(api) int __cdecl api() NOP_FUNC

#pragma region 声明导出函数
// 声明导出函数
#pragma comment(linker, "/export:GetFileVersionInfoA=?GetFileVersionInfoA@hijack@@YAHXZ,@1")
#pragma comment(linker, "/export:GetFileVersionInfoByHandle=?GetFileVersionInfoByHandle@hijack@@YAHXZ,@2")
#pragma comment(linker, "/export:GetFileVersionInfoExA=?GetFileVersionInfoExA@hijack@@YAHXZ,@3")
#pragma comment(linker, "/export:GetFileVersionInfoExW=?GetFileVersionInfoExW@hijack@@YAHXZ,@4")
#pragma comment(linker, "/export:GetFileVersionInfoSizeA=?GetFileVersionInfoSizeA@hijack@@YAHXZ,@5")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExA=?GetFileVersionInfoSizeExA@hijack@@YAHXZ,@6")
#pragma comment(linker, "/export:GetFileVersionInfoSizeExW=?GetFileVersionInfoSizeExW@hijack@@YAHXZ,@7")
#pragma comment(linker, "/export:GetFileVersionInfoSizeW=?GetFileVersionInfoSizeW@hijack@@YAHXZ,@8")
#pragma comment(linker, "/export:GetFileVersionInfoW=?GetFileVersionInfoW@hijack@@YAHXZ,@9")
#pragma comment(linker, "/export:VerFindFileA=?VerFindFileA@hijack@@YAHXZ,@10")
#pragma comment(linker, "/export:VerFindFileW=?VerFindFileW@hijack@@YAHXZ,@11")
#pragma comment(linker, "/export:VerInstallFileA=?VerInstallFileA@hijack@@YAHXZ,@12")
#pragma comment(linker, "/export:VerInstallFileW=?VerInstallFileW@hijack@@YAHXZ,@13")
#pragma comment(linker, "/export:VerLanguageNameA=?VerLanguageNameA@hijack@@YAHXZ,@14")
#pragma comment(linker, "/export:VerLanguageNameW=?VerLanguageNameW@hijack@@YAHXZ,@15")
#pragma comment(linker, "/export:VerQueryValueA=?VerQueryValueA@hijack@@YAHXZ,@16")
#pragma comment(linker, "/export:VerQueryValueW=?VerQueryValueW@hijack@@YAHXZ,@17")

EXPORT(GetFileVersionInfoA)
EXPORT(GetFileVersionInfoByHandle)
EXPORT(GetFileVersionInfoExA)
EXPORT(GetFileVersionInfoExW)
EXPORT(GetFileVersionInfoSizeA)
EXPORT(GetFileVersionInfoSizeExA)
EXPORT(GetFileVersionInfoSizeExW)
EXPORT(GetFileVersionInfoSizeW)
EXPORT(GetFileVersionInfoW)
EXPORT(VerFindFileA)
EXPORT(VerFindFileW)
EXPORT(VerInstallFileA)
EXPORT(VerInstallFileW)
EXPORT(VerLanguageNameA)
EXPORT(VerLanguageNameW)
EXPORT(VerQueryValueA)
EXPORT(VerQueryValueW)
} // namespace hijack
#pragma endregion

// 使用 MinHook 将本地代理导出函数转接到系统 version.dll。
// 目标函数是本 DLL 中的 NOP 占位导出；跳转目标是系统 DLL 的真实实现。
void LoadVersion(HINSTANCE hModule)
{
    if (!hModule)
        return;

    PBYTE imageBase = (PBYTE)hModule;
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)imageBase;
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
        return;

    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(imageBase + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE)
        return;

    IMAGE_DATA_DIRECTORY exportData =
        ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!exportData.VirtualAddress || !exportData.Size)
        return;

    PIMAGE_EXPORT_DIRECTORY exports =
        (PIMAGE_EXPORT_DIRECTORY)(imageBase + exportData.VirtualAddress);
    DWORD* names = (DWORD*)(imageBase + exports->AddressOfNames);
    DWORD* functions = (DWORD*)(imageBase + exports->AddressOfFunctions);
    WORD* ordinals = (WORD*)(imageBase + exports->AddressOfNameOrdinals);

    wchar_t systemDirectory[MAX_PATH + 1] = { 0 };
    UINT systemLength = GetSystemDirectoryW(systemDirectory, MAX_PATH);
    if (!systemLength || systemLength >= MAX_PATH)
        return;

    wchar_t systemDllPath[MAX_PATH + 16] = { 0 };
    lstrcpyW(systemDllPath, systemDirectory);
    lstrcatW(systemDllPath, L"\\version.dll");

    // 使用绝对系统路径，避免再次加载当前目录中的代理 DLL。
    HMODULE systemDll = LoadLibraryW(systemDllPath);
    if (!systemDll)
        return;

    for (DWORD i = 0; i < exports->NumberOfNames; ++i)
    {
        WORD ordinalIndex = ordinals[i];
        if (ordinalIndex >= exports->NumberOfFunctions)
            continue;

        PBYTE localFunction = imageBase + functions[ordinalIndex];
        FARPROC systemFunction = GetProcAddress(
            systemDll, (LPCSTR)(imageBase + names[i]));
        if (!localFunction || !systemFunction)
            continue;

        LPVOID originalFunction = NULL;
        MH_STATUS status = MH_CreateHook(
            localFunction, (LPVOID)systemFunction, &originalFunction);
        if (status == MH_OK)
            status = MH_EnableHook(localFunction);

        if (status != MH_OK && status != MH_ERROR_ENABLED)
        {
            char message[160] = { 0 };
            wsprintfA(message, "[chrome++] version.dll export hook failed: %d\r\n", status);
            OutputDebugStringA(message);
        }
    }
}

void LoadSysDll(HINSTANCE hModule)
{
    LoadVersion(hModule);
}
