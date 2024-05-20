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
TITLE TbConf.exe Control Panel registration

SET exepath=%%SystemDrive%%\Programs\Classic\TbConf.exe
CALL :PromptPath

SET uuid=09833920-32A2-416E-9A46-4059E46AB0E0
SET name=ClassicTbConf

ECHO Registering Control Panel item...
CALL :AddReg
IF "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
	CALL :AddReg /Reg:32
)

REM Hide system applet
SET key=HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer
reg.exe ADD "%key%" /v DisallowCPL /t REG_DWORD /d 1 /f
reg.exe ADD "%key%\DisallowCPL" /v Taskbar /d Microsoft.Taskbar /f
ECHO.

ECHO Finished.
PAUSE
GOTO :EOF


:PromptPath
SET /P exepath=Enter the "TbConf.exe" absolute file path [%exepath%]: 
REM Expand path
FOR /F "tokens=*" %%P IN ('ECHO %exepath%') DO SET realpath=%%P
IF NOT "%realpath:~1,1%"==":" (
	ECHO The path must be absolute.
	ECHO.
	GOTO :PromptPath
)
IF NOT "%realpath:~-4%"==".exe" (
	SET exepath=%exepath%.exe
	SET realpath=%realpath%.exe
)
IF NOT EXIST "%realpath%" (
	ECHO The file %realpath% does not exist.
	ECHO.
	GOTO :PromptPath
)
ECHO.
GOTO :EOF


:AddReg
SET key=HKLM\SOFTWARE\Classes\CLSID\{%uuid%}
reg.exe ADD %key% /ve /t REG_SZ /d %name% /f %*
reg.exe ADD %key% /v LocalizedString /t REG_EXPAND_SZ /d "@%%SystemRoot%%\system32\shell32.dll,-32517" /f %*
reg.exe ADD %key% /v InfoTip         /t REG_EXPAND_SZ /d "@%%SystemRoot%%\system32\shell32.dll,-30348" /f %*
reg.exe ADD %key% /v System.ApplicationName /t REG_SZ /d "%name%" /f %*
reg.exe ADD %key% /v System.ControlPanel.Category /t REG_SZ /d "1" /f %*
reg.exe ADD %key% /v System.ControlPanel.EnableInSafeMode /t REG_DWORD /d 3 /f %*

reg.exe ADD %key%\DefaultIcon /ve /t REG_EXPAND_SZ /d "%%SystemRoot%%\system32\imageres.dll,-80" /f %*
reg.exe ADD %key%\Shell\Open\Command /ve /t REG_EXPAND_SZ /d "%exepath%" /f %*
reg.exe ADD %key%\ShellFolder /v Attributes /t REG_DWORD /d 0 /f %*

SET key=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{%uuid%}
reg.exe ADD %key% /ve /t REG_SZ /d %name% /f %*

GOTO :EOF

