import os
import sys
import threading
import requests
from io import BytesIO
import customtkinter as ctk
from PIL import Image
import yt_dlp

# Configuración estética
ctk.set_appearance_mode("dark")

def resource_path(relative_path):
    """ Get absolute path to resource, works for dev and for PyInstaller """
    try:
        # PyInstaller creates a temp folder and stores path in _MEIPASS
        base_path = sys._MEIPASS
    except Exception:
        base_path = os.path.abspath(".")
    return os.path.join(base_path, relative_path)

class App(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("TSMP3")
        # Set window icon
        try:
            self.iconbitmap(resource_path("icon.ico"))
        except:
            pass # Fallback if icon is missing
        self.geometry("450x680")
        self.resizable(False, False)
        self.configure(fg_color="#121212")

        # Contenedor Principal (Tarjeta)
        self.main_card = ctk.CTkFrame(self, fg_color="#1A1A1A", corner_radius=40)
        self.main_card.pack(pady=30, padx=25, fill="both", expand=True)

        # Título
        self.label_title = ctk.CTkLabel(self.main_card, text="YouTube a MP3", font=("Segoe UI Semibold", 34), text_color="white")
        self.label_title.pack(pady=(40, 5))

        self.label_subtitle = ctk.CTkLabel(self.main_card, text="Descarga audio", font=("Segoe UI", 16), text_color="#808080")
        self.label_subtitle.pack(pady=(0, 20))

        # Contenedor de Miniatura (Thumbnail)
        self.thumb_frame = ctk.CTkFrame(self.main_card, fg_color="#262626", corner_radius=20, width=320, height=240)
        self.thumb_frame.pack(pady=10)
        self.thumb_frame.pack_propagate(False)

        self.thumb_label = ctk.CTkLabel(self.thumb_frame, text="", width=320, height=240)
        self.thumb_label.pack()

        # Input de URL
        self.url_entry = ctk.CTkEntry(
            self.main_card, 
            placeholder_text="URL del video...", 
            width=320, 
            height=50, 
            fg_color="#121212", 
            border_color="#2D2D2D",
            border_width=2,
            corner_radius=12,
            font=("Segoe UI", 14)
        )
        self.url_entry.pack(pady=(30, 15))
        self.url_entry.bind("<KeyRelease>", self.update_thumbnail_event)

        # Botón Descargar
        self.download_btn = ctk.CTkButton(
            self.main_card, 
            text="Descargar MP3", 
            command=self.start_download,
            width=320, 
            height=55, 
            fg_color="#FFFFFF", 
            text_color="#000000", 
            hover_color="#E0E0E0",
            font=("Segoe UI Bold", 18),
            corner_radius=12
        )
        self.download_btn.pack(pady=10)

        # Estado e Info
        self.status_label = ctk.CTkLabel(self.main_card, text="", font=("Segoe UI", 12), text_color="gray")
        self.status_label.pack(pady=10)

    def update_thumbnail_event(self, event):
        url = self.url_entry.get().strip()
        if len(url) > 20: # Evitar peticiones constantes para URLs cortas incompletas
            threading.Thread(target=self.load_thumbnail, args=(url,), daemon=True).start()

    def load_thumbnail(self, url):
        try:
            ydl_opts = {'quiet': True, 'no_warnings': True}
            with yt_dlp.YoutubeDL(ydl_opts) as ydl:
                info = ydl.extract_info(url, download=False)
                thumb_url = info.get('thumbnail')
                
                if thumb_url:
                    response = requests.get(thumb_url, timeout=5)
                    img_data = BytesIO(response.content)
                    img = Image.open(img_data)
                    
                    # Crop/Resize para llenar el cuadro
                    w, h = img.size
                    target_ratio = 320 / 240
                    current_ratio = w / h
                    
                    if current_ratio > target_ratio:
                        new_w = int(h * target_ratio)
                        left = (w - new_w) / 2
                        img = img.crop((left, 0, left + new_w, h))
                    else:
                        new_h = int(w / target_ratio)
                        top = (h - new_h) / 2
                        img = img.crop((0, top, w, top + new_h))
                        
                    img_ctk = ctk.CTkImage(light_image=img, dark_image=img, size=(320, 240))
                    self.thumb_label.configure(image=img_ctk)
        except:
            pass

    def start_download(self):
        url = self.url_entry.get().strip()
        if not url:
            self.status_label.configure(text="❌ Introduce una URL", text_color="#FF5555")
            return
            
        self.download_btn.configure(state="disabled", text="Descargando...")
        self.status_label.configure(text="⏳ Analizando y descargando...", text_color="gray")
        threading.Thread(target=self.download_logic, args=(url,), daemon=True).start()

    def download_logic(self, url):
        # Determinar ubicación de ffmpeg
        if getattr(sys, 'frozen', False):
            ffmpeg_path = sys._MEIPASS
        else:
            ffmpeg_path = os.path.join(os.getcwd(), 'bin')

        # Ruta a la carpeta de descargas del usuario de Windows
        downloads_dir = os.path.join(os.path.expanduser("~"), "Downloads")
        
        # Si por alguna razón no existe (poco probable en Windows), crear una local
        if not os.path.exists(downloads_dir):
            downloads_dir = os.path.join(os.getcwd(), 'downloads')
            if not os.path.exists(downloads_dir):
                os.makedirs(downloads_dir)

        ydl_opts = {
            'format': 'bestaudio/best',
            'outtmpl': os.path.join(downloads_dir, '%(title)s.%(ext)s'),
            'ffmpeg_location': ffmpeg_path,
            'postprocessors': [
                {
                    'key': 'FFmpegExtractAudio',
                    'preferredcodec': 'mp3',
                    'preferredquality': '320',
                },
                {
                    'key': 'FFmpegMetadata',
                    'add_metadata': True,
                }
            ],
            'quiet': True,
            'no_warnings': True
        }

        try:
            with yt_dlp.YoutubeDL(ydl_opts) as ydl:
                ydl.download([url])
            self.status_label.configure(text="✅ Descarga completada", text_color="#55FF55")
        except Exception as e:
            self.status_label.configure(text=f"❌ Error en descarga", text_color="#FF5555")
        
        self.download_btn.configure(state="normal", text="Descargar MP3")

if __name__ == "__main__":
    app = App()
    app.mainloop()
