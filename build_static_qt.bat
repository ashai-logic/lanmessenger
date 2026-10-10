@echo off
setlocal

REM ============================================================================
REM  LAN Messenger - static Qt 5.12.12 build script
REM  Copyright (c) 2010 - 2012 Qualia Digital Solutions.
REM  Modified by ASHAI, 2026.
REM
REM  Builds a static (release) Qt 5.12.12 for MinGW 7.3.0 32-bit.
REM  The resulting prefix is used to build the portable LAN Messenger
REM  (see README.md) without any Qt DLLs.
REM
REM  Qt is built and used under the GNU General Public License v3 (GPLv3).
REM  Qt source code:
REM  https://download.qt.io/archive/qt/5.12/5.12.12/single/qt-everywhere-src-5.12.12.tar.xz
REM ============================================================================

REM === Paths ===
set QTSRC=C:\Qt\Qt5.12.12\5.12.12\Src\qtbase
set GNUWIN32=C:\Qt\Qt5.12.12\5.12.12\Src\gnuwin32\bin
set MINGW=C:\Qt\Qt5.12.12\Tools\mingw730_32\bin
set PERL=C:\Strawberry\perl\bin
set PREFIX=C:\Qt-static\5.12.12\mingw73_32
set BUILDDIR=C:\Qt-static\build

REM === PATH (MinGW + Perl + gnuwin32) ===
set PATH=%MINGW%;%PERL%;%GNUWIN32%;%PATH%

if not exist "%BUILDDIR%" mkdir "%BUILDDIR%"
if not exist "%PREFIX%" mkdir "%PREFIX%"
cd /d "%BUILDDIR%"

REM === Configuration ===
call "%QTSRC%\configure.bat" ^
  -static -release ^
  -opensource -confirm-license ^
  -platform win32-g++ ^
  -prefix "%PREFIX%" ^
  -nomake tests -nomake examples ^
  -no-openssl ^
  -opengl desktop

if errorlevel 1 goto :err

REM === Build and install (parallel build) ===
mingw32-make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 goto :err
mingw32-make install

echo.
echo Build completed. Static Qt prefix: %PREFIX%
pause
exit /b 0

:err
echo Build ERROR.
pause
exit /b 1
