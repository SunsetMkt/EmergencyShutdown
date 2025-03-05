# EmergencyShutdown

Emergency Shutdown for Windows.

## Warning

Emergency Shutdown powers off the system almost immediately without noticing other applications, which may cause instability and data loss.

## Usage

```plain
Usage: EmergencyShutdown.exe [/r | /s] [/t seconds]
  /r : Reboot the system
  /s : Shutdown the system
  /t seconds : Set a countdown before executing (default 0)
```

## Background

[Reference](https://www.codeproject.com/Articles/34194/Performing-emergency-shutdowns)

Many of the Nt/Zw functions inside the Windows kernel are documented, but some are not. The `NtShutdownSystem` function is documented pretty well [here](http://undocumented.ntinternals.net/UserMode/Undocumented%20Functions/Hardware/NtShutdownSystem.html) at [NTInternals](http://undocumented.ntinternals.net/). The `NtSetSystemPowerState` function, however, is not.

At the final stages of the Windows shutdown process, `NtShutdownSystem` is called. It is responsible for shutting down all drivers, flushing Registry hives and the disc cache, clearing the page file, etc. After doing so, it calls the `NtSetSystemPowerState` function.

`NtSetSystemPowerState` then causes all plug-and-play devices to be shut down and the system to be either halted, powered off, or rebooted.

However, calling these two functions without notifying the system first is extremely dangerous, and may cause instability in the system.

## Thanks to

-   [bormaxi8080/windows-force-reboot](https://github.com/bormaxi8080/windows-force-reboot)
-   [ChatGPT](https://chatgpt.com/)
