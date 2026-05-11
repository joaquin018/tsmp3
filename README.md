# TSMP3 v2.0 (Python Edition)

Descarga audio de YouTube en MP3 con la máxima calidad y una interfaz moderna. Esta versión ha sido reconstruida completamente en Python para mayor facilidad de mantenimiento y portabilidad.

## ✨ Características

*   **Pega y descarga**: Solo pega el link de YouTube o SoundCloud y obtén tu MP3 en segundos.
*   **Vista previa**: Carga automáticamente la miniatura del video al detectar la URL.
*   **Calidad máxima**: Extrae audio a 320kbps usando `yt-dlp` y `ffmpeg`.
*   **Metadata inteligente**: Incrusta automáticamente artista, título y carátula en el archivo.
*   **Totalmente Portable**: Un único archivo `.exe` que contiene todo lo necesario.

## 🚀 Instalación y Uso

### Para usuarios finales
1.  Ve a la sección de [Releases](https://github.com/joaquin018/tsmp3/releases).
2.  Descarga `TSMP3.exe`.
3.  Ejecútalo y ¡listo! No requiere instalación.

### Para desarrolladores (Entorno de desarrollo)
Si quieres ejecutar el código fuente o modificarlo:

1.  Clona el repositorio.
2.  Crea un entorno virtual: `python -m venv venv`.
3.  Instala las dependencias: `pip install -r requirements.txt`.
4.  Ejecuta el programa: `python gui_downloader.py`.

*Nota: Requiere tener `ffmpeg` en una carpeta `bin/` o instalado en el sistema.*

## 🛠 Tecnologías

*   **Lenguaje**: Python 3.12+
*   **Interfaz**: CustomTkinter
*   **Motor de descarga**: yt-dlp
*   **Procesamiento de audio**: FFmpeg
*   **Empaquetado**: PyInstaller

## 📄 Licencia

MIT - Joaquín 2025
