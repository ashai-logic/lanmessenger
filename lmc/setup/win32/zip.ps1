#
# Script.ps1 - builds the portable archive
# Copyright (c) 2010 - 2012 Qualia Digital Solutions.
# Modified by ASHAI, 2026.
#

$exePath = '..\..\release'

Compress-Archive `
	-LiteralPath $exePath\lmc.exe, `
				$exePath\libcrypto-4.dll, `
				$exePath\libssl-4.dll, `
				$exePath\lmc-ashai.asc, `
				$exePath\lmc.exe.sig, `
				$exePath\sounds, `
				$exePath\lang, `
				..\..\src\resources\text\license.txt, `
				..\..\src\resources\text\license-openssl.txt, `
				..\..\src\resources\text\notice-openssl.txt, `
				..\..\src\resources\text\readme.txt, `
				..\..\src\resources\text\portable, `
				..\..\..\History_en.txt, `
				..\..\..\History_ru.txt, `
				..\..\..\README_RU.md `
	-CompressionLevel Optimal `
	-Force `
	-DestinationPath ..\lmc-1.3.0-ASHAI-win32.zip
