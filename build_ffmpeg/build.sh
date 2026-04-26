#!/bin/bash
set -e

# Work in the script's directory
cd "$(dirname "$0")"

# 1. Clone FFmpeg (shallow)
if [ ! -d "ffmpeg" ]; then
    echo "Cloning FFmpeg..."
    git clone --depth 1 https://git.ffmpeg.org/ffmpeg.git ffmpeg
fi
cd ffmpeg

# 2. Configure for extreme small size
echo "Configuring FFmpeg..."
./configure \
    --disable-everything \
    --enable-small \
    --enable-ffmpeg \
    --disable-ffplay \
    --enable-ffprobe \
    --disable-doc \
    --disable-debug \
    --disable-network \
    --disable-autodetect \
    --enable-libmp3lame \
    --enable-decoder=aac,opus,vorbis,mp3float \
    --enable-demuxer=mov,matroska,mp3 \
    --enable-parser=aac,opus,mpegaudio \
    --enable-encoder=libmp3lame \
    --enable-muxer=mp3 \
    --enable-protocol=file \
    --enable-filter=aresample \
    --pkg-config-flags="--static" \
    --extra-cflags="-static" \
    --extra-ldflags="-static"

# 3. Build
echo "Building FFmpeg..."
make -j$(nproc)

# 4. Strip
echo "Stripping binary..."
strip ffmpeg.exe

# 5. Output size
echo "Final size:"
ls -lh ffmpeg.exe
ls -lh ffprobe.exe

# 6. Copy to desktop folder
cp ffmpeg.exe ../../desktop/ffmpeg-min.exe
cp ffprobe.exe ../../desktop/ffprobe.exe
echo "Done! Binaries copied to desktop/"
