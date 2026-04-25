#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <wrl.h>
#include <wrl/implements.h>
#include <WebView2.h>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;

static ComPtr<ICoreWebView2Controller> g_controller;
static ComPtr<ICoreWebView2> g_webview;

static const char HTML[] = R"html(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:system-ui,sans-serif;background:#0a0a0a;color:#fff;min-height:100vh;display:flex;flex-direction:column;align-items:center;justify-content:center;padding:32px 20px}
h1{font-size:32px;font-weight:700;margin-bottom:8px;letter-spacing:-0.5px}
p.subtitle{color:#888;font-size:14px;margin-bottom:32px}
#thumb{width:100%;max-width:420px;aspect-ratio:16/9;object-fit:cover;border-radius:12px;background:#111;margin-bottom:24px;opacity:0;transition:opacity .3s;border:1px solid #222}
input{width:100%;max-width:420px;padding:14px 18px;border-radius:10px;border:1px solid #333;background:#111;color:#fff;font-size:15px;outline:none;transition:border-color .2s}
input:focus{border-color:#555} input::placeholder{color:#555}
button{width:100%;max-width:420px;padding:14px;border-radius:10px;border:none;background:#fff;color:#000;font-size:15px;font-weight:600;cursor:pointer;margin-top:16px;transition:background .2s}
button:hover{background:#e0e0e0} button:disabled{opacity:.4;cursor:not-allowed}
#status{margin-top:20px;font-size:14px;min-height:22px;text-align:center}
.ok{color:#4ade80}.err{color:#f87171}.info{color:#60a5fa}
</style>
</head>
<body>
<h1>YouTube to MP3</h1>
<p class="subtitle">Descarga audio en alta calidad</p>
<img id="thumb" src="" alt="">
<input id="url" type="text" placeholder="https://www.youtube.com/watch?v=..." autocomplete="off">
<button id="btn">Descargar MP3</button>
<div id="status"></div>
<script>
const $=id=>document.getElementById(id),urlEl=$('url'),btn=$('btn'),st=$('status'),thumb=$('thumb');
const getId=u=>{const m=u.match(/(?:youtu\.be\/|v=|v\/|embed\/)([^&?\s]{11})/);return m?m[1]:null};
urlEl.addEventListener('input',()=>{const id=getId(urlEl.value);if(id){thumb.src='https://img.youtube.com/vi/'+id+'/hqdefault.jpg';thumb.style.opacity='1'}else{thumb.style.opacity='0'}});
btn.addEventListener('click',()=>{const url=urlEl.value.trim();if(!url)return;btn.disabled=true;st.className='info';st.textContent='Procesando...';if(window.chrome&&window.chrome.webview){window.chrome.webview.postMessage(JSON.stringify({action:'download',url:url}))}});
window.onOk=m=>{btn.disabled=false;st.className='ok';st.textContent='✅ '+m};
window.onErr=m=>{btn.disabled=false;st.className='err';st.textContent='❌ '+m};
</script>
</body>
</html>)html";

static std::wstring Utf8ToWstr(const char* s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
    std::wstring r(n - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s, -1, r.data(), n);
    return r;
}

static void PostScript(const wchar_t* js) {
    if (g_webview) g_webview->ExecuteScript(js, nullptr);
}

static void RunDownload(const std::wstring& url) {
    wchar_t profile[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, profile))) {
        PostScript(L"window.onErr('No se pudo obtener carpeta Downloads')");
        return;
    }
    std::wstring cmd = L"\"" + std::wstring(profile) + L"\\Downloads\\%(title)s.%(ext)s\"";
    std::wstring args = L"yt-dlp.exe -x --audio-format mp3 --audio-quality 0 --no-playlist -o " + cmd + L" \"" + url + L"\"";
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(args.c_str()), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        PostScript(L"window.onOk('Descarga iniciada en Downloads')");
    } else {
        PostScript(L"window.onErr('No se pudo iniciar yt-dlp. ¿Está instalado y en PATH?')");
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_SIZE:
        if (g_controller) { RECT r; GetClientRect(hwnd, &r); g_controller->put_Bounds(r); }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"YMP3";
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, L"YMP3", L"YMP3",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 740,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd](HRESULT, ICoreWebView2Environment* env) -> HRESULT {
                env->CreateCoreWebView2Controller(hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd](HRESULT, ICoreWebView2Controller* controller) -> HRESULT {
                            if (!controller) return E_FAIL;
                            g_controller = controller;
                            g_controller->get_CoreWebView2(&g_webview);
                            RECT r; GetClientRect(hwnd, &r); g_controller->put_Bounds(r);
                            ComPtr<ICoreWebView2Settings> s; g_webview->get_Settings(&s);
                            s->put_AreDevToolsEnabled(FALSE);
                            s->put_AreDefaultContextMenusEnabled(FALSE);
                            s->put_IsStatusBarEnabled(FALSE);
                            s->put_IsZoomControlEnabled(FALSE);
                            EventRegistrationToken tok;
                            g_webview->add_WebMessageReceived(
                                Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                                    [](ICoreWebView2*, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
                                        LPWSTR msg = nullptr;
                                        args->TryGetWebMessageAsString(&msg);
                                        if (!msg) return S_OK;
                                        std::wstring w(msg);
                                        CoTaskMemFree(msg);
                                        size_t i = w.find(L"\"url\":\"");
                                        if (i != std::wstring::npos) {
                                            i += 7;
                                            size_t j = w.find(L"\"", i);
                                            if (j != std::wstring::npos) RunDownload(w.substr(i, j - i));
                                        }
                                        return S_OK;
                                    }).Get(), &tok);
                            std::wstring html = Utf8ToWstr(HTML);
                            g_webview->NavigateToString(html.c_str());
                            return S_OK;
                        }).Get());
                return S_OK;
            }).Get());

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}
