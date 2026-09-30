# TSMP3 v2.0.2

Descarga audio de YouTube y conviértelo a MP3 con yt-dlp y FFmpeg.

## Descargar para Windows

[Descargar TSMP3 v2.0.2](https://github.com/joaquin018/tsmp3/releases/download/v2.0.2/TSMP3.exe).
El ejecutable incluye Python, yt-dlp/EJS, Node y FFmpeg. No requiere instalarlos
por separado. Compatible con Windows 10/11 de 64 bits.

## Ejecutar en Windows

Requiere Python 3.12+ y Node.js 22+ o Deno 2.3+ disponibles en PATH.

Desde la raíz del repositorio:

```powershell
python -m venv venv
.\venv\Scripts\python.exe -m pip install --upgrade -r requirements.txt
.\venv\Scripts\python.exe gui_downloader.py
```

Después de instalar las dependencias, puedes abrir `run-tsmp3.cmd`.
Los audios se guardan en Descargas; si esa carpeta no existe, se usa `downloads/`.

Para usar la interfaz de consola:

```powershell
.\venv\Scripts\python.exe downloader.py
```

La consola guarda los audios en `downloads/`.

## Cambios de v2.0.2

- Actualiza el requisito de yt-dlp a 2026.08.19 o posterior e incluye EJS.
- Habilita los motores JavaScript Node y Deno cuando están disponibles.
- Encuentra FFmpeg y FFprobe independientemente del directorio de ejecución.
- Muestra el error concreto cuando una descarga falla.

La descarga y conversión a MP3 de 320 kbps se verificaron con yt-dlp 2026.08.19.
El bitrate de salida no mejora la calidad del audio original.

## Estructura

- `gui_downloader.py`: aplicación de escritorio con CustomTkinter.
- `downloader.py`: interfaz de consola.
- `download_config.py`: configuración compartida de JavaScript y FFmpeg.
- `bin/`: FFmpeg y FFprobe para Windows.
- `web/`: página de presentación estática; no realiza las descargas.
- `run-tsmp3.cmd`: acceso a la aplicación desde el entorno Python local.

## Actualizar el motor de descargas

```powershell
.\venv\Scripts\python.exe -m pip install --upgrade -r requirements.txt
```

El extra `yt-dlp[default]` instala la versión compatible de `yt-dlp-ejs`.
La aplicación habilita Node o Deno cuando están disponibles y busca FFmpeg
en `bin/`, independientemente del directorio desde el que se abra.

## Compilar el ejecutable

Con las dependencias de desarrollo instaladas, ejecuta:

```powershell
.\venv\Scripts\python.exe build_windows.py
```

Genera `dist/TSMP3.exe` con Node, FFmpeg y FFprobe incluidos. Los ejecutables
conservan las dependencias con las que se compilaron: para actualizarlos,
descarga una nueva release o vuelve a compilar.

Referencias oficiales: [versiones de yt-dlp](https://github.com/yt-dlp/yt-dlp/releases/latest)
y [requisitos JavaScript/EJS](https://github.com/yt-dlp/yt-dlp/wiki/EJS).
