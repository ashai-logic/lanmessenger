LAN Messenger (ASHAI fork)
==========================

LAN Messenger is a free peer-to-peer messaging application for intra-network
communication that does not require a server. This is an unofficial fork based
on LAN Messenger 1.2.39 of 18 December 2018.

Project repository: https://github.com/ashai-logic/lanmessenger

The application is distributed under the GNU General Public License v3.
It includes OpenSSL, distributed under the Apache License 2.0.
The application uses Qt 5.12.12 (statically linked) under the GPLv3 license.


Supported build environment
===========================
The reference (and supported) build environment is Windows 32-bit:

  - Qt 5.12.12, built statically with MinGW 7.3.0 32-bit, installed in
    C:\Qt-static\5.12.12\mingw73_32 (see "Building static Qt" below).
  - MinGW 7.3.0 32-bit (C:\Qt\Qt5.12.12\Tools\mingw730_32).
  - Strawberry Perl 5.22.1.3 32-bit (C:\Strawberry) - needed to build static Qt.
  - NSIS 3.12 (C:\Program Files (x86)\NSIS) - needed to build the installer.
  - OpenSSL 4.0.1 (Win32). Only the required files (headers, MinGW import
    libraries, runtime DLLs and the license) are vendored in
    lmc\src\third_party\openssl, so a separate OpenSSL installation is NOT
    required to build LAN Messenger.

The source tree has the following top level layout. "lmc" and "lmcapp" must
stay at the same level.

  lanmessenger
   |- lmc
   |   |- build      (shadow build directory for the lmc project)
   |   |- release    (build output: lmc.exe, lmc.rcc, lang, sounds, themes)
   |   |- setup      (installer/packaging scripts; win32, x11, mac)
   |   \- src
   |        |- third_party\openssl   (vendored OpenSSL headers/libs/license)
   |        \- scripts               (buildwin32.bat, ...)
   |- lmcapp
   |- build_static_qt.bat
   |- build_windows.bat
   |- History_en.txt
   \- History_ru.txt


Building static Qt
==================
Static Qt is used so that the portable archive contains no Qt DLLs.
Run build_static_qt.bat (run it from a normal command prompt, it does not need
elevation) and wait for "Build completed". The script configures and builds
qtbase only (-no-openssl -opengl desktop -static -release) and installs it to
C:\Qt-static\5.12.12\mingw73_32. Adjust the paths at the top of the script if
your Qt sources, MinGW or Perl are installed elsewhere.

Qt is built and used under the GNU General Public License v3 (GPLv3). Qt source
code:
https://download.qt.io/archive/qt/5.12/5.12.12/single/qt-everywhere-src-5.12.12.tar.xz


Building OpenSSL
================
The required OpenSSL files are already vendored in
lmc\src\third_party\openssl. If you need to refresh them, install the 32-bit
"Win32 OpenSSL" package (https://slproweb.com/products/Win32OpenSSL.html) and
copy only:

  include\openssl\*         -> lmc\src\third_party\openssl\include\openssl\
  lib\MinGW\x86\*.dll.a     -> lmc\src\third_party\openssl\lib\MinGW\x86\
  bin\libcrypto-4.dll       -> lmc\src\third_party\openssl\bin\
  bin\libssl-4.dll          -> lmc\src\third_party\openssl\bin\
  license.txt               -> lmc\src\third_party\openssl\license.txt


Building LAN Messenger
======================
Set QTDIR to the static Qt prefix, e.g.:

  set QTDIR=C:\Qt-static\5.12.12\mingw73_32
  set PATH=C:\Qt\Qt5.12.12\Tools\mingw730_32\bin;%QTDIR%\bin;%PATH%

1. Build the "lmcapp" support library (static library):

     cd lmcapp\src
     qmake lmcapp.pro -spec win32-g++ CONFIG-=debug CONFIG+=release
     mingw32-make
     copy /Y ..\lib\liblmcapp2.a ..\lib\liblmcapp.a

2. Build the main "lmc" project (shadow build in lmc\build):

     cd lmc\build
     qmake ..\src\lmc.pro -spec win32-g++ CONFIG-=debug CONFIG+=release
     mingw32-make -j4

   The executable is written to lmc\release\lmc.exe.

3. Run the post-build script (compiles the translations, builds lmc.rcc and
   copies the OpenSSL DLLs). QTDIR must point to a Qt that contains lrelease
   and rcc; the static prefix normally does not, so point QTDIR to the regular
   Qt 5.12.12 installation when running this step:

     cd lmc\src
     set QTDIR=C:\Qt\Qt5.12.12\5.12.12\mingw73_32
     scripts\buildwin32.bat ..\release release

   Alternatively use build_windows.bat %COMPILER% which performs the steps
   above for the mingw32 tool chain.

An easier way is to use Qt Creator with the static kit, add the custom build
step described in PLATFORM_SPECIFIC.md.


Digital signatures
==================
The authorship and integrity of the released binaries is protected with a
detached GnuPG signature (Ed25519). Next to each signed file a copy of the
public key (lmc-ashai.asc) and a detached signature named after the full name
of the signed file (e.g. lmc.exe.sig) are shipped:

  lmc-1.3.0-ASHAI-win32.zip      + lmc-ashai.asc + lmc-1.3.0-ASHAI-win32.zip.sig
  lmc-1.3.0-ASHAI-win32.exe      + lmc-ashai.asc + lmc-1.3.0-ASHAI-win32.exe.sig
  lmc.exe (portable/installed)   + lmc-ashai.asc + lmc.exe.sig

Verify a file externally with:

  gpg --verify lmc-1.3.0-ASHAI-win32.zip.sig lmc-1.3.0-ASHAI-win32.zip

The application performs the same check when the Help - About dialog is opened,
and displays the result (OK in green, FAILED in red).


Building the Windows packages
=============================
Portable archive (lmc-1.3.0-ASHAI-win32.zip):

  cd lmc\setup\win32
  zip.bat

Installer (lmc-1.3.0-ASHAI-win32.exe):

  cd lmc\setup\win32
  setup.bat

Both outputs are written to lmc\setup. Sign the resulting files as described
above before publishing.


Platform specific notes
=======================
See PLATFORM_SPECIFIC.md for details about the build directories, the custom
build steps and the packaging scripts on each platform.
