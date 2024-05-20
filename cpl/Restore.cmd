@ECHO OFF

REM Check for Admin privileges
fltmc.exe >NUL 2>&1
IF NOT ERRORLEVEL 1 GOTO :Begin

REM Create script to run me elevated
ECHO Administrator privileges are needed to register programs in the Control Panel.
ECHO If a User Account Control prompt is shown, press Yes to continue.
(
	ECHO Set App = CreateObject^("Shell.Application"^)
	ECHO App.ShellExecute "%~nx0", "", "%~dp0", "runas", 1
) > "%TEMP%\RunAsAdmin.vbs"
cscript /nologo "%TEMP%\RunAsAdmin.vbs"
DEL "%TEMP%\RunAsAdmin.vbs"
EXIT /B 1


:Begin
TITLE TbConf.exe Control Panel unregistration

SET uuid=09833920-32A2-416E-9A46-4059E46AB0E0

ECHO Restoring the system "Taskbar and Navigation" Control Panel applet...
CALL :DelReg
IF "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
	CALL :DelReg /Reg:32
)

reg.exe DELETE "HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowCPL" /v Taskbar /f
ECHO.

ECHO Finished.
PAUSE
GOTO :EOF


:DelReg
reg.exe DELETE HKLM\SOFTWARE\Classes\CLSID\{%uuid%} /f %*
reg.exe DELETE HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{%uuid%} /f %*
GOTO :EOF

