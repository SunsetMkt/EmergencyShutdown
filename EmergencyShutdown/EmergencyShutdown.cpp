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
    // std::wcout << L"Retrieving program name..." << std::endl;
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
    // std::wcout << L"Program name detected: " << programName << std::endl;

    SHUTDOWN_ACTION action;
    int delay = 0;

    if (programName == L"ShutdownImmediatelyDangerously")
    {
        std::wcout << L"Matched special shutdown executable name, initiating immediate shutdown..." << std::endl;
        action = ShutdownPowerOff;
    }
    else if (programName == L"RebootImmediatelyDangerously")
    {
        std::wcout << L"Matched special reboot executable name, initiating immediate reboot..." << std::endl;
        action = ShutdownReboot;
    }
    else
    {
        if (argc < 2 || strcmp(argv[1], "/?") == 0)
        {
            std::wcout << L"Usage: " << programName << L" [/r | /s] [/t seconds]\n"
                       << L"  /r : Reboot the system\n"
                       << L"  /s : Shutdown the system\n"
                       << L"  /t seconds : Set a countdown before executing (default 0)\n"
                       << L"This program will shutdown or reboot the system immediately by\n"
                       << L"calling the NtShutdownSystem function without notifying other\n"
                       << L"applications, which may cause instability and data loss." << std::endl;
            return 0;
        }

        std::wcout << L"Processing command: " << argv[1] << std::endl;
        if (strcmp(argv[1], "/r") == 0)
        {
            action = ShutdownReboot;
            std::wcout << L"Action: Reboot" << std::endl;
        }
        else if (strcmp(argv[1], "/s") == 0)
        {
            action = ShutdownPowerOff;
            std::wcout << L"Action: Shutdown" << std::endl;
        }
        else
        {
            std::wcout << L"Invalid option. Use /? for help." << std::endl;
            return 1;
        }

        if (argc == 4 && strcmp(argv[2], "/t") == 0)
        {
            delay = std::stoi(argv[3]);
            std::wcout << L"Shutdown delay: " << delay << L" seconds" << std::endl;
        }
    }

    if (delay > 0)
    {
        std::wcout << L"Shutdown will execute in " << delay << L" seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(delay));
    }

    std::wcout << L"Loading ntdll.dll..." << std::endl;
    HINSTANCE hNtDll = GetModuleHandle(L"ntdll.dll");
    if (hNtDll == NULL)
        return DisplayError(L"GetModuleHandle() failed");

    std::wcout << L"Retrieving NtShutdownSystem function..." << std::endl;
    PFN_NT_SHUTDOWN_SYSTEM pNtShutdownSystem =
        (PFN_NT_SHUTDOWN_SYSTEM)GetProcAddress(hNtDll, "NtShutdownSystem");
    if (pNtShutdownSystem == NULL)
        return DisplayError(L"GetProcAddress() failed");

    std::wcout << L"Adjusting privileges..." << std::endl;
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

    std::wcout << L"Calling NtShutdownSystem..." << std::endl;
    LONG status = pNtShutdownSystem(action);
    if (status)
    {
        std::wcout << L"NtShutdownSystem() returned 0x" << std::hex << status << std::endl;
        return 1;
    }

    std::wcout << L"System " << (action == ShutdownReboot ? L"reboot" : L"shutdown") << L" initiated successfully." << std::endl;
    return 0;
}
