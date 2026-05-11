import os
import yt_dlp
from rich.console import Console
from rich.panel import Panel
from rich.progress import Progress, SpinnerColumn, TextColumn, BarColumn, DownloadColumn, TaskProgressColumn
from rich.theme import Theme
from rich.prompt import Prompt

# Custom theme for a premium feel
custom_theme = Theme({
    "info": "cyan",
    "warning": "yellow",
    "error": "bold red",
    "success": "bold green",
    "highlight": "bold magenta"
})

console = Console(theme=custom_theme)

class MP3Downloader:
    def __init__(self):
        self.download_path = os.path.join(os.getcwd(), "downloads")
        if not os.path.exists(self.download_path):
            os.makedirs(self.download_path)

    def progress_hook(self, d, progress, task_id):
        if d['status'] == 'downloading':
            p = d.get('_percent_str', '0%').replace('%', '')
            try:
                progress.update(task_id, completed=float(p))
            except ValueError:
                pass
        elif d['status'] == 'finished':
            progress.update(task_id, completed=100)

    def download(self, url):
        # yt-dlp configuration for high quality MP3 and metadata
        ydl_opts = {
            'format': 'bestaudio/best',
            'outtmpl': os.path.join(self.download_path, '%(title)s.%(ext)s'),
            'postprocessors': [
                {
                    'key': 'FFmpegExtractAudio',
                    'preferredcodec': 'mp3',
                    'preferredquality': '320',
                },
                {
                    'key': 'FFmpegMetadata',
                    'add_metadata': True,
                },
                {
                    'key': 'EmbedThumbnail',
                }
            ],
            'ffmpeg_location': os.path.join(os.getcwd(), 'bin'),
            'writethumbnail': True,
            'quiet': True,
            'no_warnings': True,
            # Logger to handle status messages
            'logger': MyLogger(),
        }

        try:
            with Progress(
                SpinnerColumn(),
                TextColumn("[progress.description]{task.description}"),
                BarColumn(bar_width=None, pulse_style="bright_blue"),
                TaskProgressColumn(),
                DownloadColumn(),
                console=console
            ) as progress:
                
                task_id = progress.add_task("[cyan]Analizando enlace...", total=100)
                
                # We use a custom hook to update the progress bar
                ydl_opts['progress_hooks'] = [lambda d: self.progress_hook(d, progress, task_id)]

                with yt_dlp.YoutubeDL(ydl_opts) as ydl:
                    info = ydl.extract_info(url, download=True)
                    title = info.get('title', 'Audio desconocido')
                    
                progress.update(task_id, description=f"[success]¡Finalizado! {title}")

            console.print(Panel(
                f"[success]Descarga completada exitosamente.[/]\n\n"
                f"[info]Título:[/] {title}\n"
                f"[info]Ubicación:[/] [highlight]{self.download_path}[/]",
                title="[bold white]Resultado[/]",
                border_style="bright_blue",
                expand=False
            ))

        except Exception as e:
            console.print(Panel(f"[error]Ocurrió un error inesperado:[/]\n{str(e)}", border_style="red", title="Error"))

class MyLogger:
    def debug(self, msg): pass
    def warning(self, msg): pass
    def error(self, msg):
        console.print(f"[error]{msg}[/]")

def main():
    console.clear()
    console.print(Panel.fit(
        "  [bold highlight]TS MP3 DOWNLOADER v2.0[/]  \n"
        "[italic white]Potenciado por yt-dlp y Rich[/]",
        border_style="bright_magenta",
        padding=(1, 4)
    ))

    while True:
        url = Prompt.ask("\n[bold yellow]Introduce la URL (YouTube, SoundCloud, etc.) o 'q' para salir[/]")
        
        if url.lower() == 'q':
            console.print("[italic gray]¡Hasta luego![/]")
            break
            
        if not url.strip():
            continue

        downloader = MP3Downloader()
        downloader.download(url)

if __name__ == "__main__":
    main()
