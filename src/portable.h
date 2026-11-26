#pragma once

#include <shellapi.h>
#include <shlwapi.h>
#include <string>
#include <vector>

// Quote one Windows command-line argument using CommandLineToArgvW-compatible rules.
std::wstring QuoteSpaceIfNeeded(const std::wstring &arg)
{
    if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos)
        return arg;

    std::wstring result = L"\"";
    size_t backslashes = 0;
    for (wchar_t ch : arg)
    {
        if (ch == L'\\')
        {
            ++backslashes;
        }
        else if (ch == L'"')
        {
            result.append(backslashes * 2 + 1, L'\\');
            result += L'"';
            backslashes = 0;
        }
        else
        {
            result.append(backslashes, L'\\');
            result += ch;
            backslashes = 0;
        }
    }
    result.append(backslashes * 2, L'\\');
    result += L'"';
    return result;
}

std::wstring JoinArgsString(const std::vector<std::wstring> &args, const std::wstring &delimiter)
{
    std::wstring text;
    for (size_t i = 0; i < args.size(); ++i)
    {
        if (i)
            text += delimiter;
        text += QuoteSpaceIfNeeded(args[i]);
    }
    return text;
}

bool IsExistsPortable()
{
    std::wstring path = GetAppDir() + L"\\portable";
    return PathFileExistsW(path.c_str()) != FALSE;
}

bool IsNeedPortable()
{
    return true;
    static bool need_portable = IsExistsPortable();
    return need_portable;
}

std::wstring GetUserDataDir()
{
    std::wstring path = GetAppDir() + L"\\..\\Data";
    wchar_t temp[MAX_PATH] = {0};
    if (!PathCanonicalizeW(temp, path.c_str()))
        return path;
    return temp;
}

std::wstring GetDiskCacheDir()
{
    std::wstring path = GetAppDir() + L"\\..\\Cache";
    wchar_t temp[MAX_PATH] = {0};
    if (!PathCanonicalizeW(temp, path.c_str()))
        return path;
    return temp;
}

static bool HasSwitch(const std::vector<std::wstring> &args, const wchar_t *name)
{
    const size_t length = wcslen(name);
    for (const auto &arg : args)
    {
        if (_wcsnicmp(arg.c_str(), name, length) == 0 &&
            (arg.length() == length || arg[length] == L'='))
            return true;
    }
    return false;
}

// Build the relaunch command line. argv[0] is passed separately as lpFile.
std::wstring GetCommand(LPWSTR param)
{
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(param, &argc);
    if (!argv || argc <= 0)
    {
        if (argv)
            LocalFree(argv);
        return L"";
    }

    std::vector<std::wstring> args;
    std::vector<std::wstring> trailing;
    bool after_separator = false;
    for (int i = 1; i < argc; ++i)
    {
        if (!after_separator && wcscmp(argv[i], L"--") == 0)
        {
            after_separator = true;
            continue;
        }
        (after_separator ? trailing : args).push_back(argv[i]);
    }
    LocalFree(argv);

    if (!HasSwitch(args, L"--shuax"))
        args.push_back(L"--shuax");
    if (!HasSwitch(args, L"--disable-features"))
        args.push_back(L"--disable-features=RendererCodeIntegrity,FlashDeprecationWarning");
    if (!HasSwitch(args, L"--disk-cache-dir"))
        args.push_back(L"--disk-cache-dir=" + GetDiskCacheDir());
    if (!HasSwitch(args, L"--user-data-dir"))
        args.push_back(L"--user-data-dir=" + GetUserDataDir());

    if (!trailing.empty())
    {
        args.push_back(L"--");
        args.insert(args.end(), trailing.begin(), trailing.end());
    }
    return JoinArgsString(args, L" ");
}

void Portable(LPWSTR param)
{
    wchar_t path[MAX_PATH] = {0};
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    if (!length || length >= MAX_PATH)
    {
        DebugLog(L"Portable: GetModuleFileName failed, error=%lu", GetLastError());
        return;
    }

    std::wstring args = GetCommand(param);
    if (args.empty())
    {
        DebugLog(L"Portable: failed to construct command line");
        return;
    }

    wchar_t app_dir[MAX_PATH] = {0};
    lstrcpynW(app_dir, path, MAX_PATH);
    if (!PathRemoveFileSpecW(app_dir))
    {
        DebugLog(L"Portable: PathRemoveFileSpec failed");
        return;
    }

    SHELLEXECUTEINFOW sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    sei.lpVerb = L"open";
    sei.lpFile = path;
    sei.lpParameters = args.c_str();
    sei.lpDirectory = app_dir;
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExW(&sei))
    {
        DebugLog(L"Portable: relaunched with command line: %s", args.c_str());
        if (sei.hProcess)
            CloseHandle(sei.hProcess);
        ExitProcess(0);
    }

    DebugLog(L"Portable: ShellExecuteEx failed, error=%lu, command line: %s",
             GetLastError(), args.c_str());
}
