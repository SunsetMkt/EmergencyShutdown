#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <Windows.h>

typedef enum _SHUTDOWN_ACTION
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION,
    *PSHUTDOWN_ACTION;

typedef LONG(__stdcall *PFN_NT_SHUTDOWN_SYSTEM)(DWORD SHUTDOWN_ACTION);

int DisplayError(const std::wstring &message);
std::wstring GetFormattedMessage(DWORD, LPCWSTR, DWORD, ...);

int main(int argc, char *argv[])
{
    std::wstring programName = std::wstring(argv[0], argv[0] + strlen(argv[0]));
    size_t lastSlash = programName.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos)
    {
        programName = programName.substr(lastSlash + 1);
    }
    size_t dotPos = programName.find_last_of(L".");
    if (dotPos != std::wstring::npos)
    {
        programName = programName.substr(0, dotPos);
    }

    SHUTDOWN_ACTION action;
    int delay = 0;

    if (programName == L"ShutdownImmediatelyDangerously")
    {
        action = ShutdownPowerOff;
    }
    else if (programName == L"RebootImmediatelyDangerously")
    {
        action = ShutdownReboot;
    }
    else
    {
        if (argc < 2 || strcmp(argv[1], "/?") == 0)
        {
            std::wcout << L"Usage: " << programName << L" [/r | /s] [/t seconds]\n"
                       << L"  /r : Reboot the system\n"
                       << L"  /s : Shutdown the system\n"
                       << L"  /t seconds : Set a countdown before executing (default 0)" << std::endl;
            return 0;
        }

        if (strcmp(argv[1], "/r") == 0)
        {
            action = ShutdownReboot;
        }
        else if (strcmp(argv[1], "/s") == 0)
        {
            action = ShutdownPowerOff;
        }
        else
        {
            std::wcout << L"Invalid option. Use /? for help." << std::endl;
            return 1;
        }

        if (argc == 4 && strcmp(argv[2], "/t") == 0)
        {
            delay = std::stoi(argv[3]);
        }
    }

    if (delay > 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(delay));
    }

    HINSTANCE hNtDll = GetModuleHandle(L"ntdll.dll");
    if (hNtDll == NULL)
        return DisplayError(L"GetModuleHandle() failed");

    PFN_NT_SHUTDOWN_SYSTEM pNtShutdownSystem =
        (PFN_NT_SHUTDOWN_SYSTEM)GetProcAddress(hNtDll, "NtShutdownSystem");
    if (pNtShutdownSystem == NULL)
        return DisplayError(L"GetProcAddress() failed");

    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
        return DisplayError(L"OpenProcessToken() failed");

    TOKEN_PRIVILEGES tkp;
    if (!LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid))
        return DisplayError(L"LookupPrivilegeValue() failed");

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0))
        return DisplayError(L"AdjustTokenPrivileges() failed");

    CloseHandle(hToken);

    LONG status = pNtShutdownSystem(action);
    if (status)
    {
        std::wcout << L"NtShutdownSystem() returned 0x" << std::hex << status << std::endl;
        return 1;
    }

    return 0;
}

int DisplayError(const std::wstring &message)
{
    DWORD error = GetLastError();
    std::wstring errorMsg = GetFormattedMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error);
    std::wcout << message << L": " << errorMsg << std::endl;
    return 1;
}

std::wstring GetFormattedMessage(DWORD dwFlags, LPCWSTR pMsg, DWORD dwMsgId, ...)
{
    LPWSTR pBuffer = NULL;
    va_list args;
    va_start(args, dwMsgId);

    FormatMessage(dwFlags | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                  pMsg, dwMsgId,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                  (LPWSTR)&pBuffer, 0, &args);
    va_end(args);

    std::wstring result = pBuffer ? pBuffer : L"Unknown error";
    LocalFree(pBuffer);
    return result;
}
