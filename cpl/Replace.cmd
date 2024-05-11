@ECHO OFF

SET uuid=09833920-32A2-416E-9A46-4059E46AB0E0
SET name=ClassicTbConf
SET path=%%SystemDrive%%\Programs\Classic\TbConf.exe

ECHO Adding %name% to the Control Panel...
ECHO.

CALL :AddReg
IF "%PROCESSOR_ARCHITECTURE%"=="AMD64" (
	CALL :AddReg /Reg:32
)

REM Hide system applet
SET key=HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer
REG ADD "%key%" /v DisallowCPL /t REG_DWORD /d 1 /f
REG ADD "%key%\DisallowCPL" /v Taskbar /d Microsoft.Taskbar /f

ECHO.
ECHO Finished.
PAUSE
GOTO :EOF

:AddReg
SET key=HKLM\SOFTWARE\Classes\CLSID\{%uuid%}
REG ADD %key% /ve /t REG_SZ /d %name% /f %*
REG ADD %key% /v LocalizedString /t REG_EXPAND_SZ /d "@%%SystemRoot%%\system32\shell32.dll,-32517" /f %*
REG ADD %key% /v InfoTip         /t REG_EXPAND_SZ /d "@%%SystemRoot%%\system32\shell32.dll,-30348" /f %*
REG ADD %key% /v System.ApplicationName /t REG_SZ /d "%name%" /f %*
REG ADD %key% /v System.ControlPanel.Category /t REG_SZ /d "1" /f %*
REG ADD %key% /v System.ControlPanel.EnableInSafeMode /t REG_DWORD /d 3 /f %*

REG ADD %key%\DefaultIcon /ve /t REG_EXPAND_SZ /d "%%SystemRoot%%\system32\imageres.dll,-80" /f %*
REG ADD %key%\Shell\Open\Command /ve /t REG_EXPAND_SZ /d "%path%" /f %*
REG ADD %key%\ShellFolder /v Attributes /t REG_DWORD /d 0 /f %*

SET key=HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\ControlPanel\NameSpace\{%uuid%}
REG ADD %key% /ve /t REG_SZ /d %name% /f %*

GOTO :EOF
