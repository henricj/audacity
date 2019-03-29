@setlocal

@if exist "%VS150COMNTOOLS%VsDevCmd.bat" goto skipwhere

@if defined VSWHERE if not exist %VSWHERE% set "VSWHERE="

@if not defined VSWHWERE set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

@if not exist %VSWHERE% set VSWHERE="%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

@if not exist %VSWHERE% goto weberrorexit

@set pre=Microsoft.VisualStudio.Product.
@set ids=%pre%Community %pre%Professional %pre%Enterprise %pre%BuildTools

@for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products %ids% -requires Microsoft.Component.MSBuild -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath`) do @(
  @set VS150COMNTOOLS=%%i\Common7\Tools\
)

@if not exist "%VS150COMNTOOLS%VsDevCmd.bat" goto weberrorexit

:skipwhere
md "%~dp0build\"

@setlocal

call "%VS150COMNTOOLS%VsDevCmd.bat"
@if %errorlevel% neq 0 goto weberrorexit

call :dobuild Win32
@if %errorlevel% neq 0 goto errorexit

@endlocal
@exit /b 0

:dobuild

cd "%~dp0"
@if %errorlevel% neq 0 goto errorexit

call :run_msbuild "Debug" %1 %2
@if %errorlevel% neq 0 goto errorexit

call :run_msbuild "Release" %1 %2
@if %errorlevel% neq 0 goto errorexit

@endlocal
@exit /b 0

:run_msbuild

@title Cleaning %~1^|%~2
@echo +++++++++++++++++++++++++ %~1^|%~2
msbuild /m "/p:Configuration=%~1;Platform=%~2" /t:Clean /verbosity:minimal /fileLogger "/fileLoggerParameters:Append;Verbosity=normal;LogFile=%~dp0build\audacity_%~1_%~2.log" top.proj
@if %errorlevel% neq 0 goto errorexit
del "%~dp0build\audacity_%~1_%~2.log"
@echo ------------------------- %~1^|%~2
@title Command Prompt

@exit /b 0

:weberrorexit
@echo ***** Unable to find Visual Studio 2017 *****
start https://www.visualstudio.com/downloads/#build-tools-for-visual-studio-2017

:errorexit
@endlocal
@title Command Prompt
@echo ***** BUILD FAILED ***** %0 %*
@exit /b 1
