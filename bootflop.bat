@ECHO OFF

:
: copy files to HD...
:
COPY /Y BLUE.SYS C:\reactos\system\drivers\blue.SYS
COPY /Y KEYBOARD.SYS C:\reactos\system\drivers\KEYBOARD.SYS
COPY /Y NTDLL.DLL C:\reactos\system\NTDLL.DLL
: COPY /Y CRTDLL.DLL C:\reactos\system\CRTDLL.DLL
COPY /Y SHELL.EXE C:\reactos\system\SHELL.EXE

:
: present a menu to the booter...
:
ECHO 1) Keyboard,IDE,VFatFSD
ECHO 2) IDE,VFatFSD
ECHO 3) No Drivers
CHOICE /C:123 /T:2,3 "Select kernel boot config"
IF ERRORLEVEL 3 GOTO :L3
IF ERRORLEVEL 2 GOTO :L2

:L1
CLS
LOADROS NTOSKRNL.EXE KEYBOARD.O IDE.SYS VFATFSD.SYS
GOTO :END

:L2
CLS
LOADROS NTOSKRNL.EXE IDE.SYS VFATFSD.SYS
GOTO :END

:L3
CLS
LOADROS NTOSKRNL.EXE
GOTO :END

:END
EXIT


