@echo off
setlocal enabledelayedexpansion

set ROOT=%~dp0
set OUT_DIR=%ROOT%build
set DEPS_DIR=%ROOT%deps
set RAYLIB_DIR=%DEPS_DIR%\raylib-win64\raylib-6.0_win64_mingw-w64

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"
if not exist "%DEPS_DIR%" mkdir "%DEPS_DIR%"

if not exist "%RAYLIB_DIR%\lib\libraylib.a" (
    echo Baixando Raylib para Windows MinGW...
    set "ZIP_URL=https://github.com/raysan5/raylib/releases/download/6.0/raylib-6.0_win64_mingw-w64.zip"
    set "ZIP_PATH=%DEPS_DIR%\raylib-6.0_win64_mingw-w64.zip"
    curl.exe -L -o "%ZIP_PATH%" "%ZIP_URL%"
    if errorlevel 1 (
        echo Falha ao baixar o Raylib.
        exit /b 1
    )
    if not exist "%DEPS_DIR%\raylib-win64" mkdir "%DEPS_DIR%\raylib-win64"
    tar -xf "%ZIP_PATH%" -C "%DEPS_DIR%\raylib-win64"
    if errorlevel 1 (
        echo Falha ao extrair o Raylib.
        exit /b 1
    )
)

if not exist "%RAYLIB_DIR%\lib\libraylib.a" (
    echo Raylib nao encontrado em %RAYLIB_DIR%.
    exit /b 1
)

gcc "%ROOT%src\main.c" -I "%RAYLIB_DIR%\include" -L "%RAYLIB_DIR%\lib" -o "%OUT_DIR%\fronteira-arida.exe" -lraylib -lopengl32 -lgdi32 -lwinmm
if errorlevel 1 (
    echo Erro ao compilar o jogo.
    exit /b 1
)

copy /Y "%RAYLIB_DIR%\lib\raylib.dll" "%OUT_DIR%\raylib.dll"

echo Build concluido: %OUT_DIR%\fronteira-arida.exe
