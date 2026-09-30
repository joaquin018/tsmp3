"""Build the portable Windows executable with all download tools included."""

from pathlib import Path
import shutil
import subprocess
import sys


def main():
    if sys.platform != "win32":
        raise SystemExit("Este ejecutable se compila en Windows.")
    root = Path(__file__).resolve().parent
    node = shutil.which("node")
    if not node:
        raise SystemExit("Instala Node.js 22+ antes de compilar.")
    version = subprocess.check_output([node, "--version"], text=True).strip()
    if int(version.lstrip("v").split(".")[0]) < 22:
        raise SystemExit("Se requiere Node.js 22+.")
    subprocess.run([
        sys.executable, "-m", "PyInstaller", "--noconfirm", "--clean",
        "--onefile", "--windowed", "--name", "TSMP3", "--icon", "icon.ico",
        "--add-data", "icon.ico;.",
        "--add-binary", "bin/ffmpeg.exe;bin",
        "--add-binary", "bin/ffprobe.exe;bin",
        "--add-binary", f"{node};bin",
        "--collect-all", "customtkinter", "--collect-all", "yt_dlp_ejs",
        "gui_downloader.py",
    ], cwd=root, check=True)
    executable = root / "dist" / "TSMP3.exe"
    print(f"{executable}: {executable.stat().st_size:,} bytes")


if __name__ == "__main__":
    main()
