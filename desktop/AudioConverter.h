#pragma once
#include <string>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

class AudioConverter {
public:
    // Convierte un archivo de audio (M4A, etc) a MP3 usando Windows Media Foundation
    // Devuelve true si tuvo éxito.
    static bool ConvertToMp3(const std::wstring& inputPath, const std::wstring& outputPath);

private:
    static HRESULT CreateMediaSource(const std::wstring& url, IMFMediaSource** ppSource);
};
