@echo off
setlocal
cd /d "%~dp0"
if not exist "venv\Scripts\pythonw.exe" (
    echo Primero ejecuta: python -m venv venv
    echo Luego: venv\Scripts\python.exe -m pip install -U -r requirements.txt
    pause
    exit /b 1
)
start "" "venv\Scripts\pythonw.exe" "gui_downloader.py"
