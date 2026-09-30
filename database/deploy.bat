@echo off
chcp 65001 > nul
rem Развёртывание базы данных «Чудо Обувь» на локальном PostgreSQL 17.
rem Пароль суперпользователя postgres берётся из %APPDATA%\postgresql\pgpass.conf,
rem а если его там нет, psql запросит пароль.
setlocal
set "PSQL=%ProgramFiles%\PostgreSQL\17\bin\psql.exe"
set "PGHOST=localhost"
set "PGPORT=5432"
set "PGUSER=postgres"
cd /d "%~dp0"

if not exist "%PSQL%" (
    echo Не найден psql: %PSQL%
    echo Установите PostgreSQL 17 или исправьте путь в переменной PSQL.
    exit /b 1
)

"%PSQL%" -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname = 'chudo_obuv'" | findstr "1" > nul
if errorlevel 1 (
    "%PSQL%" -d postgres -v ON_ERROR_STOP=1 -q -f create_database.sql || goto :error
)
"%PSQL%" -d chudo_obuv -v ON_ERROR_STOP=1 -q -o nul -f chudo_obuv.sql || goto :error
"%PSQL%" -d chudo_obuv -v ON_ERROR_STOP=1 -q -f app_role.sql || goto :error

echo База данных chudo_obuv развёрнута: структура, данные и роль приложения.
exit /b 0

:error
echo Ошибка развёртывания базы данных, подробности выше.
exit /b 1
