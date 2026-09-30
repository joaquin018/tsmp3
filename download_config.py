"""Runtime configuration shared by the desktop and console downloaders."""

from pathlib import Path
import shutil
import sys


def resource_dir():
    return Path(getattr(sys, "_MEIPASS", Path(__file__).resolve().parent))


def runtime_options():
    base = resource_dir()
    runtimes = {}
    for name in ("deno", "node"):
        executable = name + (".exe" if sys.platform == "win32" else "")
        bundled = base / "bin" / executable
        path = str(bundled) if bundled.is_file() else shutil.which(name)
        if path:
            runtimes[name] = {"path": path}
    if not runtimes:
        raise RuntimeError("Instala Deno 2.3+ o Node.js 22+ para descargar de YouTube.")

    return {"js_runtimes": runtimes, "noplaylist": True}


def ffmpeg_location():
    executable = "ffmpeg.exe" if sys.platform == "win32" else "ffmpeg"
    probe = "ffprobe.exe" if sys.platform == "win32" else "ffprobe"
    base = resource_dir()
    for folder in (base / "bin", base):
        if (folder / executable).is_file() and (folder / probe).is_file():
            return str(folder)
    ffmpeg = shutil.which("ffmpeg")
    if ffmpeg and (Path(ffmpeg).parent / probe).is_file():
        return str(Path(ffmpeg).parent)
    raise RuntimeError("No se encuentran FFmpeg y FFprobe. Colócalos en la carpeta bin de TSMP3.")
