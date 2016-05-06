call "%~dp0build_wx2013.bat"
@if %errorlevel% neq 0 exit /b %errorlevel%

call "%~dp0build_wx2015.bat"
@if %errorlevel% neq 0 exit /b %errorlevel%

call "%~dp0build_2013.bat"
@if %errorlevel% neq 0 exit /b %errorlevel%

call "%~dp0build_2015.bat"
@if %errorlevel% neq 0 exit /b %errorlevel%
