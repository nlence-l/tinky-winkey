# Tinky Winkey - Windows Service Keylogger

A Windows service that acts as a keylogger, developed for the 42 School curriculum. It captures keystrokes along with the context of the active process.

## Project Structure

- **svc.exe**: Service management program
- **winkey.exe**: Keylogger application

## Build

```text
nmake
```

**Requirements**: Windows OS, Visual Studio Build Tools, Windows SDK

## Usage

```text
.\bin\svc.exe install    # Install service
.\bin\svc.exe start      # Start service & keylogger
.\bin\svc.exe stop       # Stop service & keylogger
.\bin\svc.exe delete     # Remove service
```

## Features

- Runs as Windows service with SYSTEM privileges
- Captures keystrokes with foreground process information
- Single instance enforcement
- Human-readable logging with timestamps
- Locale-aware keyboard input handling

## Log Format

```
[YYYY-MM-DD HH:MM:SS] [Process: example.exe] Keystroke: A
[YYYY-MM-DD HH:MM:SS] [Process: chrome.exe] Keystroke: Enter
```

## Technical Details

- **API Functions**: OpenSCManager, CreateService, OpenService, StartService, ControlService, CloseServiceHandle, DuplicateTokenEx
- **Compiler**: CL with `/Wall` and `/WX` flags
- **Build System**: NMAKE

> **Disclaimer**: This is an educational project for 42 School. Be aware that using keyloggers may violate privacy laws in your jurisdiction.