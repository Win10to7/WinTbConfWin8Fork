@ECHO OFF

SET uuid=09833920-32A2-416E-9A46-4059E46AB0E0

ECHO Restoring the system "Taskbar and Navigation" Control Panel applet...
ECHO.

CALL :DelReg
IF "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
	CALL :DelReg /Reg:32
)

REG DELETE "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowCPL" /v Taskbar /f

ECHO.
ECHO Finished.
PAUSE
GOTO :EOF

:DelReg
REG DELETE HKLM\SOFTWARE\Classes\CLSID\{%uuid%} /f %*
REG DELETE HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{%uuid%} /f %*
GOTO :EOF
