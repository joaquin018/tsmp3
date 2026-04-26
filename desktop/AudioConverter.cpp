#include "AudioConverter.h"
#include <wmcontainer.h>
#include <mferror.h>
#include <propvarutil.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "propsys.lib")

bool AudioConverter::ConvertToMp3(const std::wstring& inputPath, const std::wstring& outputPath) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return false;

    hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    IMFSourceReader* pReader = NULL;
    IMFSinkWriter* pWriter = NULL;
    DWORD streamIndex = 0;
    BOOL success = FALSE;

    do {
        // 1. Crear el Source Reader para el archivo de entrada
        hr = MFCreateSourceReaderFromURL(inputPath.c_str(), NULL, &pReader);
        if (FAILED(hr)) break;

        // 2. Seleccionar el primer flujo de audio y desactivar los demás
        hr = pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
        if (FAILED(hr)) break;
        hr = pReader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
        if (FAILED(hr)) break;

        // 3. Configurar el formato de salida para el encoder MP3
        IMFMediaType* pMediaTypeOut = NULL;
        hr = MFCreateMediaType(&pMediaTypeOut);
        if (FAILED(hr)) break;

        hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio); // Audio
        hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_MP3);
        hr = pMediaTypeOut->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
        hr = pMediaTypeOut->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
        hr = pMediaTypeOut->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 192000 / 8); // 192 kbps
        hr = pMediaTypeOut->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 1);

        // 4. Crear el Sink Writer para el archivo de salida
        hr = MFCreateSinkWriterFromURL(outputPath.c_str(), NULL, NULL, &pWriter);
        if (FAILED(hr)) {
            pMediaTypeOut->Release();
            break;
        }

        DWORD writerStreamIndex = 0;
        hr = pWriter->AddStream(pMediaTypeOut, &writerStreamIndex);
        pMediaTypeOut->Release();
        if (FAILED(hr)) break;

        // 5. Configurar el formato que el encoder espera recibir (PCM)
        IMFMediaType* pMediaTypeIn = NULL;
        hr = MFCreateMediaType(&pMediaTypeIn);
        if (FAILED(hr)) break;

        hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        hr = pMediaTypeIn->SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, 2);
        hr = pMediaTypeIn->SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, 44100);
        hr = pMediaTypeIn->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        hr = pMediaTypeIn->SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, 4);
        hr = pMediaTypeIn->SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 44100 * 4);

        hr = pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pMediaTypeIn);
        if (FAILED(hr)) {
            pMediaTypeIn->Release();
            break;
        }
        
        hr = pWriter->SetInputMediaType(writerStreamIndex, pMediaTypeIn, NULL);
        pMediaTypeIn->Release();
        if (FAILED(hr)) break;

        // 6. Iniciar el proceso
        hr = pWriter->BeginWriting();
        if (FAILED(hr)) break;

        // Loop de lectura y escritura
        while (true) {
            DWORD dwFlags = 0;
            IMFSample* pSample = NULL;
            hr = pReader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, NULL, &dwFlags, NULL, &pSample);
            if (FAILED(hr) || (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM)) break;

            if (pSample) {
                hr = pWriter->WriteSample(writerStreamIndex, pSample);
                pSample->Release();
                if (FAILED(hr)) break;
            }
        }

        hr = pWriter->Finalize();
        if (SUCCEEDED(hr)) success = TRUE;

    } while (false);

    if (pReader) pReader->Release();
    if (pWriter) pWriter->Release();

    MFShutdown();
    CoUninitialize();

    return success;
}
