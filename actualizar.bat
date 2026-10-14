@echo off
chcp 65001 >nul
title Ceitus - Actualizar Offsets
cd /d "C:\Users\holad\chetox"

echo ============================================
echo   Ceitus AutoDump + Build
echo ============================================
echo.

echo [1/3] Descargando offsets nuevos...
node autodump.js
if errorlevel 2 (
    echo.
    echo ERROR: Un pointer critico no se actualizo — recompilar seria inutil.
    echo        Revisa el resumen de arriba.
    pause
    exit /b 2
)
if errorlevel 1 (
    echo ERROR: No se pudieron descargar los offsets
    pause
    exit /b 1
)

echo.
echo [2/3] Compilando...
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" "RbxESP\RbxESP\RbxESP.vcxproj" /p:Configuration=Release /p:Platform=x64 /v:minimal
if errorlevel 1 (
    echo ERROR: No compilo
    pause
    exit /b 1
)

echo.
echo [3/3] Copiando exe...
copy /y "RbxESP\RbxESP\x64\Release\RbxESP.exe" "RbxESP\x64\Release\RbxESP.exe"

echo.
echo ============================================
echo   LISTO - Ceitus actualizado
echo ============================================
pause
