#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <cctype>
#include <wrl.h>
#include <wrl/implements.h>
#include <WebView2.h>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;

static ComPtr<ICoreWebView2Controller> g_controller;
static ComPtr<ICoreWebView2> g_webview;
static HWND g_hwnd = nullptr;
static const UINT WM_RUN_JS = WM_APP + 1;

// Resource-based embedding


static const char HTML[] = R"html(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
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
.progress-container{width:100%;max-width:420px;display:none;margin-top:16px}
.progress-label{font-size:13px;color:#888;margin-bottom:4px;display:flex;justify-content:space-between}
.progress-label span:last-child{color:#666}
.progress-bar{width:100%;height:8px;background:#1a1a1a;border-radius:4px;overflow:hidden;}
.progress-fill{height:100%;width:0%;border-radius:4px;transition:width .15s ease}
#dl-fill{background:linear-gradient(90deg,#4ade80,#22c55e);}
#conv-fill{background:linear-gradient(90deg,#60a5fa,#3b82f6);}
.conv-indet{position:relative;overflow:hidden;}
.conv-indet::after{content:'';position:absolute;top:0;left:-40%;width:40%;height:100%;background:rgba(255,255,255,0.35);border-radius:4px;animation:convSlide 1.2s ease-in-out infinite;}
@keyframes convSlide{0%{left:-40%}100%{left:100%}}
#status{margin-top:12px;font-size:14px;min-height:22px;text-align:center}
.ok{color:#4ade80}.err{color:#f87171}.info{color:#60a5fa}
</style>
</head>
<body>
<h1>YouTube to M4A</h1>
<p class="subtitle">Descarga audio en alta calidad</p>
<img id="thumb" src="" alt="">
<input id="url" type="text" placeholder="https://www.youtube.com/watch?v=..." autocomplete="off">
<button id="btn">Descargar M4A</button>

<div id="dl-container" class="progress-container">
    <div class="progress-label"><span>Descargando...</span><span id="dl-text">0%</span></div>
    <div class="progress-bar"><div id="dl-fill" class="progress-fill"></div></div>
</div>

<div id="conv-container" class="progress-container" style="margin-top:12px;">
    <div class="progress-label"><span>Procesando audio...</span><span id="conv-text">procesando</span></div>
    <div class="progress-bar"><div id="conv-fill" class="progress-fill"></div></div>
</div>

<div id="status"></div>
<script>
const $=id=>document.getElementById(id),urlEl=$('url'),btn=$('btn'),st=$('status'),thumb=$('thumb');
const dlCont=$('dl-container'),dlFill=$('dl-fill'),dlText=$('dl-text');
const convCont=$('conv-container'),convFill=$('conv-fill'),convText=$('conv-text');
const getId=u=>{const m=u.match(/(?:youtu\.be\/|v=|v\/|embed\/)([^&?\s]{11})/);return m?m[1]:null};
urlEl.addEventListener('input',()=>{const id=getId(urlEl.value);if(id){thumb.src='https://img.youtube.com/vi/'+id+'/hqdefault.jpg';thumb.style.opacity='1'}else{thumb.style.opacity='0'}});
btn.addEventListener('click',()=>{const url=urlEl.value.trim();if(!url)return;btn.disabled=true;st.className='info';st.textContent='Iniciando...';dlCont.style.display='block';dlFill.style.width='0%';dlText.textContent='0%';convCont.style.display='none';convFill.style.width='0%';convText.textContent='...';if(window.chrome&&window.chrome.webview){window.chrome.webview.postMessage(JSON.stringify({action:'download',url:url}))}});
window.onDownloadProgress=(pct,dlSize,totalSize)=>{dlFill.style.width=pct+'%';dlText.textContent=dlSize?dlSize+' / '+totalSize:pct+'%'};
window.onConverting=()=>{dlFill.style.width='100%';dlText.textContent='100%';convCont.style.display='block';convFill.style.width='30%';convFill.classList.add('conv-indet');convText.textContent='procesando...'};
window.onConvertingDone=()=>{convFill.classList.remove('conv-indet');convFill.style.width='100%';convText.textContent='listo'};
window.onOk=m=>{btn.disabled=false;st.className='ok';st.textContent='✅ '+m;setTimeout(()=>{dlCont.style.display='none';convCont.style.display='none'},2000)};
window.onErr=m=>{btn.disabled=false;st.className='err';st.textContent='❌ '+m;dlCont.style.display='none';convCont.style.display='none'};
window.onStatus=m=>{st.className='info';st.textContent=m};
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
    if (!g_hwnd || !js) return;
    auto script = new std::wstring(js);
    PostMessageW(g_hwnd, WM_RUN_JS, 0, reinterpret_cast<LPARAM>(script));
}

static bool ExtractEmbeddedYtDlp(std::wstring& outPath) {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring dir = std::wstring(tempPath) + L"tsmp3";
    std::wstring ytdlpPath = dir + L"\\yt-dlp.exe";

    DWORD attr = GetFileAttributesW(ytdlpPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        outPath = ytdlpPath;
        return true;
    }

    CreateDirectoryW(dir.c_str(), nullptr);

    HRSRC hRes = FindResourceW(nullptr, L"YTDLP_EXE", L"BINARY");
    if (!hRes) return false;
    HGLOBAL hData = LoadResource(nullptr, hRes);
    if (!hData) return false;
    const char* start = static_cast<const char*>(LockResource(hData));
    size_t size = SizeofResource(nullptr, hRes);


    std::ofstream out(ytdlpPath, std::ios::binary);
    if (!out) return false;
    out.write(start, static_cast<std::streamsize>(size));
    out.close();

    outPath = ytdlpPath;
    return true;
}

static bool FindYtDlp(std::wstring& outPath) {
    if (ExtractEmbeddedYtDlp(outPath)) return true;

    wchar_t selfPath[MAX_PATH];
    GetModuleFileNameW(nullptr, selfPath, MAX_PATH);
    std::wstring selfDir(selfPath);
    size_t lastSlash = selfDir.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) selfDir = selfDir.substr(0, lastSlash);

    std::wstring localYtDlp = selfDir + L"\\yt-dlp.exe";
    DWORD attr = GetFileAttributesW(localYtDlp.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        outPath = localYtDlp;
        return true;
    }

    outPath = L"yt-dlp.exe";
    return true;
}

static double ParseProgress(const std::string& line) {
    size_t pos = line.find("%");
    if (pos == std::string::npos || pos == 0) return -1.0;
    
    size_t start = pos - 1;
    while (start > 0 && (isdigit(static_cast<unsigned char>(line[start - 1])) || line[start - 1] == '.')) {
        start--;
    }
    
    std::string numStr = line.substr(start, pos - start);
    try {
        return std::stod(numStr);
    } catch (...) {
        return -1.0;
    }
}

static double ParseSizeToMB(const std::string& sizeStr) {
    std::string s = sizeStr;
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) s.pop_back();
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    if (s.empty()) return 0.0;

    size_t i = 0;
    while (i < s.length() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.')) i++;
    if (i == 0) return 0.0;
    double val = std::stod(s.substr(0, i));
    std::string unit = s.substr(i);
    for (auto& c : unit) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (unit.find("gib") != std::string::npos || unit.find("gb") != std::string::npos) return val * 1024.0;
    if (unit.find("mib") != std::string::npos || unit.find("mb") != std::string::npos) return val;
    if (unit.find("kib") != std::string::npos || unit.find("kb") != std::string::npos) return val / 1024.0;
    return val / (1024.0 * 1024.0);
}

static std::string FormatMB(double mb) {
    if (mb >= 1024.0) {
        char buf[32]; snprintf(buf, sizeof(buf), "%.2f GB", mb / 1024.0); return std::string(buf);
    } else {
        char buf[32]; snprintf(buf, sizeof(buf), "%.1f MB", mb); return std::string(buf);
    }
}

static bool ParseDownloadInfo(const std::string& line, double& pct, std::string& dlSize, std::string& totalSize) {
    pct = ParseProgress(line);
    dlSize.clear(); totalSize.clear();

    auto skipSpaces = [](const std::string& s, size_t& pos) {
        while (pos < s.length() && s[pos] == ' ') pos++;
    };

    // Formato moderno: "... 12.5MiB / 45.2MiB at ..."
    size_t slashPos = line.find(" / ");
    if (slashPos != std::string::npos) {
        size_t dlEnd = slashPos;
        while (dlEnd > 0 && line[dlEnd - 1] == ' ') dlEnd--;
        size_t dlStart = dlEnd;
        while (dlStart > 0 && line[dlStart - 1] != ' ') dlStart--;
        dlSize = line.substr(dlStart, dlEnd - dlStart);

        size_t totalStart = slashPos + 3;
        skipSpaces(line, totalStart);
        size_t totalEnd = totalStart;
        while (totalEnd < line.length() && line[totalEnd] != ' ') totalEnd++;
        totalSize = line.substr(totalStart, totalEnd - totalStart);

        dlSize = FormatMB(ParseSizeToMB(dlSize));
        totalSize = FormatMB(ParseSizeToMB(totalSize));
        return pct >= 0;
    }

    // Formato clásico: "... 12.5% of   52.97MiB at ..."
    size_t ofPos = line.find(" of ");
    if (ofPos != std::string::npos) {
        size_t totalStart = ofPos + 4;
        skipSpaces(line, totalStart);
        if (totalStart < line.length() && line[totalStart] == '~') totalStart++;
        size_t totalEnd = totalStart;
        while (totalEnd < line.length() && line[totalEnd] != ' ') totalEnd++;
        totalSize = line.substr(totalStart, totalEnd - totalStart);
        double totalMB = ParseSizeToMB(totalSize);
        totalSize = FormatMB(totalMB);
        if (pct >= 0) {
            dlSize = FormatMB(totalMB * pct / 100.0);
        } else {
            dlSize = "?";
        }
        return true;
    }

    return pct >= 0;
}

static void PostDownloadProgress(double pct, const std::string& dlSize = "", const std::string& totalSize = "") {
    wchar_t buf[512];
    std::wstring wDl = dlSize.empty() ? L"" : Utf8ToWstr(dlSize.c_str());
    std::wstring wTotal = totalSize.empty() ? L"" : Utf8ToWstr(totalSize.c_str());
    swprintf(buf, 512, L"window.onDownloadProgress(%d,'%s','%s')", static_cast<int>(pct), wDl.c_str(), wTotal.c_str());
    PostScript(buf);
}

struct DownloadCtx {
    HANDLE hProcess;
    std::string logPath;
};

static DWORD WINAPI MonitorThread(LPVOID param) {
    DownloadCtx* ctx = static_cast<DownloadCtx*>(param);
    HANDLE hProcess = ctx->hProcess;
    std::string logPath = ctx->logPath;
    delete ctx;

    double lastPct = -1.0;
    std::string lastDlSize, lastTotalSize;
    std::streampos lastPos = 0;
    DWORD startTick = GetTickCount();
    bool convertingSent = false;

    while (true) {
        // Check if process still running
        DWORD exitCode = STILL_ACTIVE;
        if (hProcess) {
            GetExitCodeProcess(hProcess, &exitCode);
        }

        // Read new data from log file
        std::ifstream log(logPath, std::ios::in);
        if (log.is_open()) {
            // Seek to last position
            log.seekg(0, std::ios::end);
            std::streampos fileSize = log.tellg();
            
            if (fileSize > lastPos) {
                log.seekg(lastPos);
                std::string line;
                while (std::getline(log, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (line.find("[download]") != std::string::npos) {
                        double pct; std::string dlSize, totalSize;
                        if (ParseDownloadInfo(line, pct, dlSize, totalSize)) {
                            if (pct >= 0.0 && pct != lastPct) {
                                lastPct = pct;
                                lastDlSize = dlSize;
                                lastTotalSize = totalSize;
                                PostDownloadProgress(pct, dlSize, totalSize);
                            }
                        }
                    } else if (!convertingSent && (line.find("[ExtractAudio]") != std::string::npos || line.find("[ffmpeg]") != std::string::npos || line.find("Deleting") != std::string::npos)) {
                        PostScript(L"window.onConverting()");
                        convertingSent = true;
                    }
                }
                lastPos = fileSize;
            }
            log.close();
        }

        if (exitCode != STILL_ACTIVE) break;

        // Safety timeout: avoids leaving the UI stuck forever if yt-dlp hangs.
        if (GetTickCount() - startTick > 30 * 60 * 1000) {
            TerminateProcess(hProcess, 124);
            break;
        }

        Sleep(150);
    }

    // Read any remaining log data
    {
        std::ifstream log(logPath, std::ios::in);
        if (log.is_open()) {
            log.seekg(lastPos);
            std::string line;
            while (std::getline(log, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.find("[download]") != std::string::npos) {
                    double pct; std::string dlSize, totalSize;
                    if (ParseDownloadInfo(line, pct, dlSize, totalSize)) {
                        if (pct >= 0.0 && pct != lastPct) {
                            lastPct = pct;
                            PostDownloadProgress(pct, dlSize, totalSize);
                        }
                    }
                } else if (!convertingSent && (line.find("[ExtractAudio]") != std::string::npos || line.find("[ffmpeg]") != std::string::npos || line.find("Deleting") != std::string::npos)) {
                    PostScript(L"window.onConverting()");
                    convertingSent = true;
                }
            }
            log.close();
        }
    }

    // Signal conversion completion
    if (convertingSent) {
        PostScript(L"window.onConvertingDone()");
    }

    // Get final exit code
    DWORD exitCode = 0;
    if (hProcess) {
        GetExitCodeProcess(hProcess, &exitCode);
        CloseHandle(hProcess);
    }

    // Clean up log file
    DeleteFileA(logPath.c_str());

    if (exitCode == 0) {
        PostDownloadProgress(100.0, lastDlSize, lastTotalSize);
        PostScript(L"window.onOk('Descarga completada en Downloads')");
    } else {
        wchar_t errBuf[256];
        swprintf(errBuf, 256, L"window.onErr('Error en la descarga (c\\u00f3digo %lu)')", exitCode);
        PostScript(errBuf);
    }

    return 0;
}

static void RunDownload(const std::wstring& url) {
    std::wstring ytdlpPath;
    if (!FindYtDlp(ytdlpPath)) {
        PostScript(L"window.onErr('No se encontr\\u00f3 yt-dlp embebido ni en PATH')");
        return;
    }

    wchar_t profile[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, profile))) {
        PostScript(L"window.onErr('No se pudo obtener carpeta Downloads')");
        return;
    }
    std::wstring outTemplate = std::wstring(profile) + L"\\Downloads\\%(title)s.%(ext)s";

    // Create temp log file for stderr
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    wchar_t logFile[MAX_PATH];
    GetTempFileNameW(tempPath, L"tsmp3", 0, logFile);
    std::string logPathA;
    {
        int len = WideCharToMultiByte(CP_ACP, 0, logFile, -1, nullptr, 0, nullptr, nullptr);
        logPathA.resize(len - 1);
        WideCharToMultiByte(CP_ACP, 0, logFile, -1, logPathA.data(), len, nullptr, nullptr);
    }

    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE hLog = CreateFileW(
        logFile,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        &sa,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        nullptr
    );
    if (hLog == INVALID_HANDLE_VALUE) {
        DeleteFileW(logFile);
        PostScript(L"window.onErr('No se pudo crear log temporal')");
        return;
    }

    HANDLE hNullIn = CreateFileW(
        L"NUL",
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        &sa,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    // Launch yt-dlp directly. stdout and stderr go to the same temp log file.
    std::wstring args = L"\"" + ytdlpPath + L"\" -f bestaudio --no-playlist --newline --progress -o \""
        + outTemplate + L"\" \"" + url + L"\"";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = (hNullIn != INVALID_HANDLE_VALUE) ? hNullIn : nullptr;
    si.hStdOutput = hLog;
    si.hStdError = hLog;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    BOOL created = CreateProcessW(
        nullptr,
        const_cast<LPWSTR>(args.c_str()),
        nullptr, nullptr,
        TRUE,
        CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
        nullptr, nullptr,
        &si, &pi
    );

    CloseHandle(hLog);
    if (hNullIn != INVALID_HANDLE_VALUE) CloseHandle(hNullIn);

    if (!created) {
        DeleteFileW(logFile);
        PostScript(L"window.onErr('No se pudo iniciar yt-dlp')");
        return;
    }

    CloseHandle(pi.hThread);

    // Start monitor thread
    DownloadCtx* ctx = new DownloadCtx{ pi.hProcess, logPathA };
    HANDLE hThread = CreateThread(nullptr, 0, MonitorThread, ctx, 0, nullptr);
    if (hThread) CloseHandle(hThread);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_RUN_JS:
        if (lParam) {
            std::wstring* script = reinterpret_cast<std::wstring*>(lParam);
            if (g_webview) g_webview->ExecuteScript(script->c_str(), nullptr);
            delete script;
        }
        return 0;
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
    wc.lpszClassName = L"TSMP3";
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, L"TSMP3", L"TSMP3",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 740,
        nullptr, nullptr, hInst, nullptr);
    g_hwnd = hwnd;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    wchar_t localAppData[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData);
    std::wstring webviewDataDir = std::wstring(localAppData) + L"\\TSMP3\\WebView2";
    CreateCoreWebView2EnvironmentWithOptions(nullptr, webviewDataDir.c_str(), nullptr,
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