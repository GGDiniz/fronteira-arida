@echo off
setlocal

set ROOT=%~dp0
set RAYLIB_DIR=C:\Users\52800817879\source\raylib-win64\raylib-6.0_win64_mingw-w64
set OUT_DIR=%ROOT%build

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

gcc "%ROOT%src\main.c" -I "%RAYLIB_DIR%\include" -L "%RAYLIB_DIR%\lib" -o "%OUT_DIR%\fronteira-arida.exe" -lraylib -lopengl32 -lgdi32 -lwinmm
if errorlevel 1 (
    echo Erro ao compilar o jogo.
    exit /b 1
)

copy /Y "%RAYLIB_DIR%\lib\raylib.dll" "%OUT_DIR%\raylib.dll"

echo Build concluido: %OUT_DIR%\fronteira-arida.exe
