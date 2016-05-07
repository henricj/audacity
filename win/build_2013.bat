@setlocal

md "%~dp0build\"

call "%VS120COMNTOOLS%vsvars32.bat"
@if %errorlevel% neq 0 goto errorexit

cd "%~dp0"
@if %errorlevel% neq 0 goto errorexit

msbuild /m /p:Configuration="Debug",Platform=x64 /t:Build /consoleLoggerParameters:Summary /verbosity:minimal /fileLogger /fileLoggerParameters:Summary;Append;Verbosity=normal;LogFile=%~dp0build\audacity_d_x64_2013.log audacity.sln
@if %errorlevel% neq 0 goto errorexit

msbuild /m /p:Configuration="Debug",Platform=Win32 /t:Build /consoleLoggerParameters:Summary /verbosity:minimal /fileLogger /fileLoggerParameters:Summary;Append;Verbosity=normal;LogFile=%~dp0build\audacity_d_win32_2013.log audacity.sln
@if %errorlevel% neq 0 goto errorexit

msbuild /m /p:Configuration=Release,Platform=x64 /t:Build /consoleLoggerParameters:Summary /verbosity:minimal /fileLogger /fileLoggerParameters:Summary;Append;Verbosity=normal;LogFile=%~dp0build\audacity_x64_2013.log audacity.sln
rem @if %errorlevel% neq 0 goto errorexit

msbuild /m /p:Configuration=Release,Platform=Win32 /t:Build /consoleLoggerParameters:Summary /verbosity:minimal /fileLogger /fileLoggerParameters:Summary;Append;Verbosity=normal;LogFile=%~dp0build\audacity_win32_2013.log audacity.sln
rem @if %errorlevel% neq 0 goto errorexit

msbuild /m /p:Configuration="Static Release",Platform=x64 /t:Build /consoleLoggerParameters:Summary /verbosity:minimal /fileLogger /fileLoggerParameters:Summary;Append;Verbosity=normal;LogFile=%~dp0build\audacity_s_x64_2013.log audacity.sln
rem @if %errorlevel% neq 0 goto errorexit

msbuild /m /p:Configuration="Static Release",Platform=Win32 /t:Build /consoleLoggerParameters:Summary /verbosity:minimal /fileLogger /fileLoggerParameters:Summary;Append;Verbosity=normal;LogFile=%~dp0build\audacity_s_win32_2013.log audacity.sln
rem @if %errorlevel% neq 0 goto errorexit

@endlocal
@exit /b 0

:errorexit
@endlocal
@exit /b 1
