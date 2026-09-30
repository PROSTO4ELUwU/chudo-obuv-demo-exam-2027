@echo off
chcp 65001 > nul
rem Сборка C++-версии, запуск тестов и подготовка к запуску на другом
rem компьютере: всё нужное для работы копируется в bin\cpp.
rem Пути по умолчанию соответствуют стандартным установщикам Qt и PostgreSQL;
rem если у вас они другие, задайте переменные QT_DIR, MINGW_DIR и PG_BIN.
setlocal
if not defined QT_DIR set "QT_DIR=C:\Qt\6.11.2\mingw_64"
if not defined MINGW_DIR set "MINGW_DIR=C:\Qt\Tools\mingw1310_64"
if not defined PG_BIN set "PG_BIN=%ProgramFiles%\PostgreSQL\17\bin"
set "PATH=%MINGW_DIR%\bin;%QT_DIR%\bin;%PG_BIN%;%PATH%"
cd /d "%~dp0"
set "DEPLOY_DIR=%~dp0..\bin\cpp"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_CXX_COMPILER="%MINGW_DIR%\bin\g++.exe" || goto :error
cmake --build build || goto :error
ctest --test-dir build --output-on-failure || goto :error

rem Папку открытую, например, в проводнике удалить нельзя — тогда она просто очищается
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
if not exist "%DEPLOY_DIR%" mkdir "%DEPLOY_DIR%"
copy /y build\ChudoObuv.exe "%DEPLOY_DIR%" > nul || goto :error

rem Библиотеки Qt и среда выполнения MinGW. Из драйверов баз данных нужен
rem только qsqlpsql, из форматов изображений — только ico; сетевые плагины,
rem сенсорный ввод и стиль Windows не нужны (приложение использует Fusion)
windeployqt --release --no-translations --no-opengl-sw --no-system-d3d-compiler ^
    --compiler-runtime --exclude-plugins qsqlibase,qsqlite,qsqlmimer,qsqloci,qsqlodbc,^
qgif,qjpeg,qcertonlybackend,qschannelbackend,qnetworklistmanager,qtuiotouchplugin,^
qmodernwindowsstyle ^
    "%DEPLOY_DIR%\ChudoObuv.exe" || goto :error

rem Русский перевод стандартных кнопок и диалогов Qt
mkdir "%DEPLOY_DIR%\translations"
copy /y "%QT_DIR%\translations\qtbase_ru.qm" "%DEPLOY_DIR%\translations" > nul || goto :error

rem Клиентская библиотека PostgreSQL для драйвера qsqlpsql и её зависимости
for %%F in (libpq.dll libintl-9.dll libiconv-2.dll libssl-3-x64.dll libcrypto-3-x64.dll) do (
    copy /y "%PG_BIN%\%%F" "%DEPLOY_DIR%" > nul || goto :error
)

echo Готово: bin\cpp\ChudoObuv.exe (config.ini берётся из корня репозитория)
exit /b 0

:error
echo Ошибка сборки, подробности выше.
exit /b 1
