@echo off

rem %1: Path to the extension DLL
rem %2: PHP extension directory
rem %3: Apache binary directory

set DLL_PATH=%1
set PHPEXT_DIR=%2
set APACHE_BIN=%3

"%APACHE_BIN%\httpd.exe" -k stop
copy "%DLL_PATH%" "%PHPEXT_DIR%"
"%APACHE_BIN%\httpd.exe" -k start
