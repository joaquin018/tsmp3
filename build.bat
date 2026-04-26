@echo off
cd /d "%~dp0"
setlocal enabledelayedexpansion

echo ============================================
echo   TSMP3 - Build Script
echo ============================================
echo.

:: Find vcvars64.bat (supports both VS IDE and BuildTools)
set "VCVARS="
for /d %%d in ("C:\Program Files\Microsoft Visual Studio\2022\*") do if exist "%%d\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%d\VC\Auxiliary\Build\vcvars64.bat"
for /d %%d in ("C:\Program Files (x86)\Microsoft Visual Studio\2022\*") do if exist "%%d\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%d\VC\Auxiliary\Build\vcvars64.bat"

if not defined VCVARS (
    echo [ERROR] Visual Studio 2022 no encontrado.
    echo         Instala Visual Studio 2022 ^(Community, Professional, Enterprise^)
    echo         o BuildTools con el workload "Desktop development with C++".
    pause
    exit /b 1
)

echo [INFO] vcvars64.bat: !VCVARS!
echo.

:: Setup MSVC environment
call "!VCVARS!" >nul 2>&1
if errorlevel 1 (
    echo [ERROR] No se pudo configurar el entorno de MSVC.
    pause
    exit /b 1
)
echo [OK] Entorno MSVC configurado.
echo.

:: Ensure cmake is available
where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] cmake no encontrado en el PATH.
    pause
    exit /b 1
)

:: Clean old build if generator doesn't match (e.g. was VS IDE, now NMake)
set "RECONFIGURE="
if exist "desktop\build\CMakeCache.txt" (
    findstr /c:"CMAKE_MAKE_PROGRAM" "desktop\build\CMakeCache.txt" | findstr /c:"nmake" >nul 2>&1
    if errorlevel 1 set "RECONFIGURE=1"
) else (
    set "RECONFIGURE=1"
)

if defined RECONFIGURE (
    if exist "desktop\build\CMakeCache.txt" (
        echo [INFO] Generador anterior no compatible. Limpiando build anterior...
        rmdir /s /q "desktop\build" 2>nul
    )
    if not exist "desktop\build\" mkdir "desktop\build"

    echo [STEP 1/3] Configurando CMake...
    cmake -S desktop -B desktop/build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 (
        echo [ERROR] Fallo la configuracion de CMake.
        pause
        exit /b 1
    )
    echo [OK] CMake configurado.
) else (
    echo [STEP 1/3] CMake ya configurado. Saltando...
)
echo.

:: Build Release
echo [STEP 2/3] Compilando tsmp3.exe (Release)...
cmake --build desktop/build
if errorlevel 1 (
    echo [ERROR] Fallo la compilacion.
    pause
    exit /b 1
)
echo [OK] Compilacion exitosa.
echo.

:: Copy .exe to root
echo [STEP 3/3] Copiando tsmp3.exe a la raiz...
if exist "desktop\build\tsmp3.exe" (
    copy /y "desktop\build\tsmp3.exe" "tsmp3.exe" >nul
    echo [OK] tsmp3.exe actualizado en raiz.
    for %%A in ("tsmp3.exe") do echo       Tamano: %%~zA bytes
) else (
    echo [WARN] No se encontro desktop\build\tsmp3.exe
)

echo.
echo ============================================
echo   Build completado!
echo ============================================
endlocal