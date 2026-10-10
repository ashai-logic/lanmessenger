LAN Messenger (форк ASHAI)
==========================

LAN Messenger — это свободная одноранговая программа обмена сообщениями для
связи внутри локальной сети, не требующая сервера. Это неофициальный форк,
основанный на LAN Messenger 1.2.39 от 18 декабря 2018 года.

Репозиторий проекта: https://github.com/ashai-logic/lanmessenger

Программа распространяется под лицензией GNU General Public License v3.
В состав входит OpenSSL, распространяемый под лицензией Apache License 2.0.
Программа использует Qt 5.12.12 (статически слинкованный) по лицензии GPLv3.

Поддерживаемое окружение сборки
===============================
Эталонное (и поддерживаемое) окружение сборки — Windows 32-бит:

  - Qt 5.12.12, собранный статически с MinGW 7.3.0 32-бит, установленный в
    C:\Qt-static\5.12.12\mingw73_32 (см. «Сборка статического Qt» ниже).
  - MinGW 7.3.0 32-бит (C:\Qt\Qt5.12.12\Tools\mingw730_32).
  - Strawberry Perl 5.22.1.3 32-бит (C:\Strawberry) — требуется для сборки
    статического Qt.
  - NSIS 3.12 (C:\Program Files (x86)\NSIS) — требуется для сборки установщика.
  - OpenSSL 4.0.1 (Win32). В дерево включены только необходимые файлы
    (заголовки, import-библиотеки MinGW, runtime-DLL и лицензия) в каталоге
    lmc\src\third_party\openssl, поэтому отдельная установка OpenSSL для сборки
    LAN Messenger НЕ требуется.

Дерево исходного кода имеет следующую структуру верхнего уровня. Каталоги "lmc"
и "lmcapp" должны находиться на одном уровне.

  lanmessenger
   |- lmc
   |   |- build      (каталог теневой сборки проекта lmc)
   |   |- release    (результат сборки: lmc.exe, lmc.rcc, lang, sounds, themes)
   |   |- setup      (скрипты установщика/упаковки; win32, x11, mac)
   |   \- src
   |        |- third_party\openssl   (включённые OpenSSL заголовки/библиотеки/лицензия)
   |        \- scripts               (buildwin32.bat, ...)
   |- lmcapp
   |- build_static_qt.bat
   |- build_windows.bat
   |- History_en.txt
   \- History_ru.txt


Сборка статического Qt
======================
Статический Qt используется для того, чтобы в портативном архиве не было
Qt-DLL. Запустите build_static_qt.bat (из обычной командной строки, повышение
прав не требуется) и дождитесь сообщения "Build completed". Скрипт настраивает
и собирает только qtbase (-no-openssl -opengl desktop -static -release) и
устанавливает его в C:\Qt-static\5.12.12\mingw73_32. При необходимости
скорректируйте пути в начале скрипта, если ваши исходники Qt, MinGW или Perl
установлены в других местах.

Qt собирается и используется по лицензии GPLv3. Исходный код Qt:
https://download.qt.io/archive/qt/5.12/5.12.12/single/qt-everywhere-src-5.12.12.tar.xz


Сборка OpenSSL
==============
Необходимые файлы OpenSSL уже включены в
lmc\src\third_party\openssl. Если их нужно обновить, установите 32-битный
пакет "Win32 OpenSSL" (https://slproweb.com/products/Win32OpenSSL.html) и
скопируйте только:

  include\openssl\*         -> lmc\src\third_party\openssl\include\openssl\
  lib\MinGW\x86\*.dll.a     -> lmc\src\third_party\openssl\lib\MinGW\x86\
  bin\libcrypto-4.dll       -> lmc\src\third_party\openssl\bin\
  bin\libssl-4.dll          -> lmc\src\third_party\openssl\bin\
  license.txt               -> lmc\src\third_party\openssl\license.txt


Сборка LAN Messenger
====================
Установите QTDIR на префикс статического Qt, например:

  set QTDIR=C:\Qt-static\5.12.12\mingw73_32
  set PATH=C:\Qt\Qt5.12.12\Tools\mingw730_32\bin;%QTDIR%\bin;%PATH%

1. Соберите вспомогательную библиотеку "lmcapp" (статическая библиотека):

     cd lmcapp\src
     qmake lmcapp.pro -spec win32-g++ CONFIG-=debug CONFIG+=release
     mingw32-make
     copy /Y ..\lib\liblmcapp2.a ..\lib\liblmcapp.a

2. Соберите основной проект "lmc" (теневая сборка в lmc\build):

     cd lmc\build
     qmake ..\src\lmc.pro -spec win32-g++ CONFIG-=debug CONFIG+=release
     mingw32-make -j4

   Исполняемый файл записывается в lmc\release\lmc.exe.

3. Запустите пост-сборочный скрипт (компилирует переводы, собирает lmc.rcc и
   копирует OpenSSL-DLL). QTDIR должен указывать на Qt, содержащий lrelease
   и rcc; в статическом префиксе их обычно нет, поэтому на этом шаге укажите
   QTDIR на обычную установку Qt 5.12.12:

     cd lmc\src
     set QTDIR=C:\Qt\Qt5.12.12\5.12.12\mingw73_32
     scripts\buildwin32.bat ..\release release

   Либо используйте build_windows.bat %COMPILER%, который выполняет шаги выше
   для цепочки инструментов mingw32.

Более простой способ — использовать Qt Creator со статическим набором и
добавить пользовательский шаг сборки, описанный в PLATFORM_SPECIFIC.md.


Цифровые подписи
================
Авторство и целостность выпускаемых бинарных файлов защищены отсоединённой
подписью GnuPG (Ed25519). Рядом с каждым подписанным файлом поставляются
копия публичного ключа (lmc-ashai.asc) и отсоединённая подпись, названная по
полному имени подписываемого файла (например, lmc.exe.sig):

  lmc-1.3.0-ASHAI-win32.zip      + lmc-ashai.asc + lmc-1.3.0-ASHAI-win32.zip.sig
  lmc-1.3.0-ASHAI-win32.exe      + lmc-ashai.asc + lmc-1.3.0-ASHAI-win32.exe.sig
  lmc.exe (портативная/установка) + lmc-ashai.asc + lmc.exe.sig

Проверьте файл извне командой:

  gpg --verify lmc-1.3.0-ASHAI-win32.zip.sig lmc-1.3.0-ASHAI-win32.zip

Приложение выполняет ту же проверку при открытии окна «Справка — О программе»
и показывает результат (OK зелёным, FAILED красным).


Сборка пакетов для Windows
==========================
Портативный архив (lmc-1.3.0-ASHAI-win32.zip):

  cd lmc\setup\win32
  zip.bat

Установщик (lmc-1.3.0-ASHAI-win32.exe):

  cd lmc\setup\win32
  setup.bat

Оба результата записываются в lmc\setup. Перед публикацией подпишите
полученные файлы, как описано выше.


Замечания по платформам
=======================
Подробности о каталогах сборки, пользовательских шагах сборки и скриптах
упаковки для каждой платформы см. в PLATFORM_SPECIFIC.md.
