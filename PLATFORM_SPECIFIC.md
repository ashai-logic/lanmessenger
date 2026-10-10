LAN Messenger - platform specific build notes
=============================================

Windows (supported)
-------------------
The reference build uses the statically linked Qt 5.12.12 / MinGW 7.3.0
32-bit tool chain.

Build directory layout (shadow build):
  lmc
   |-build
   |  \- ...           (qmake generated makefiles and object files)
   |-release           (output: lmc.exe, lmc.rcc, lang, sounds, themes)
   \-src

OpenSSL is not installed system-wide; it is vendored in
lmc\src\third_party\openssl and referenced directly by lmc.pro, so no extra
configuration is required.

Setting up the environment:
  set QTDIR=C:\Qt-static\5.12.12\mingw73_32
  set PATH=C:\Qt\Qt5.12.12\Tools\mingw730_32\bin;%QTDIR%\bin;%PATH%

The custom build step needs the following parameters (Qt Creator, release kit):
  Command: scripts\buildwin32.bat
  Working directory: %{sourceDir}
  Command arguments: %{buildDir}\..\release release

Note: the static Qt prefix usually does not ship lrelease/rcc. Before running
buildwin32.bat set QTDIR to the regular Qt installation that provides them, for
example:
  set QTDIR=C:\Qt\Qt5.12.12\5.12.12\mingw73_32

Alternatively build everything from the command line with:
  build_windows.bat mingw32       (lmcapp + lmc)
  cd lmc\setup\win32
  zip.bat                         (portable archive)
  setup.bat                       (NSIS installer)

For building the installer, NSIS 3.12 must be installed (the default path is
C:\Program Files (x86)\NSIS). setup.bat executes the NSIS script setup.nsi,
which contains the variables ProductVersion and InstallerVersion that must be
kept in sync with IDA_VERSION. The generated installer is named
lmc-<version>-win32.exe and is written to lmc\setup. zip.bat runs zip.ps1 to
build the portable archive lmc-<version>-win32.zip.

Signing: each released file is accompanied by the public key lmc-ashai.asc and
a detached signature named after the full name of the signed file, e.g.
lmc.exe -> lmc.exe.sig. See README.md.


Linux/X11 (not part of the reference build)
-------------------------------------------
Build directory for debug is lmc/debug, for release lmc/release.
The custom build script needs executable permission:
  chmod 755 ./scripts/buildx11

Custom build step (Qt Creator):
  Command: ./scripts/buildx11
  Working directory: %{sourceDir}
  Command arguments: %{buildDir}

For packaging, run the bash script "setup" in lmc/setup/x11 (and "rpm_setup"
for an RPM package built from the deb with alien). The scripts named "postinst",
"postrm" and "prerm" in package/DEBIAN must have executable permission, and the
file "control" in the same folder should be edited to reflect the proper
version and installed size.


Mac OS X (not part of the reference build)
------------------------------------------
Build directory for debug is lmc/debug, for release lmc/release.
The custom build script needs executable permission:
  chmod 755 ./scripts/buildmacos

Custom build step (Qt Creator):
  Command: ./scripts/buildmacos
  Working directory: %{sourceDir}
  Command arguments: %{buildDir}/LAN-Messenger.app/Contents/MacOS

For packaging, run the bash script "createdisk" in lmc/setup/mac, then set up
the disk image and run "addlicense". The dmg file is named
lmc_<version>_intel.dmg and is written to lmc/setup.

---

Modified by ASHAI, 2026.
