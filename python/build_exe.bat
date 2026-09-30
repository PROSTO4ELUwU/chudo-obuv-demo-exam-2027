@echo off
chcp 65001 > nul
rem Сборка Python-версии в один exe-файл: bin\python\ChudoObuv.exe.
rem Нужно окружение из requirements-dev.txt (PyInstaller); запуск из любой папки.
rem Что входит в сборку, описано в ChudoObuv.spec.
setlocal
cd /d "%~dp0"

python -m PyInstaller --noconfirm --clean --distpath "%~dp0..\bin\python" ^
    --workpath build ChudoObuv.spec || goto :error

echo Готово: bin\python\ChudoObuv.exe (config.ini берётся из корня репозитория)
exit /b 0

:error
echo Ошибка сборки, подробности выше.
exit /b 1
