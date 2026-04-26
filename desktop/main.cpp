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

#include <thread>
#include <chrono>
#include "AudioConverter.h"

static ComPtr<ICoreWebView2Controller> g_controller;
static ComPtr<ICoreWebView2> g_webview;
static HWND g_hwnd = nullptr;
static std::wstring g_preferredBrowser = L"auto";
static bool g_useOAuth2 = false;

static void PostScript(const wchar_t* js);
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
        body {
            font-family: 'Outfit', -apple-system, sans-serif;
            background: linear-gradient(135deg, #0f0f0f 0%, #1a1a1a 100%);
            color: #ffffff;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
            overflow: hidden;
            transition: all 0.5s ease;
        }

        .settings-btn {
            position: absolute;
            top: 20px;
            right: 20px;
            cursor: pointer;
            z-index: 1000;
            transition: transform 0.3s ease;
            opacity: 0.7;
        }
        .settings-btn:hover { transform: rotate(90deg); opacity: 1; }

        .settings-panel {
            position: fixed;
            top: 0;
            right: -320px;
            width: 300px;
            height: 100%;
            background: rgba(20, 20, 20, 0.95);
            backdrop-filter: blur(20px);
            box-shadow: -10px 0 30px rgba(0,0,0,0.5);
            padding: 30px;
            transition: right 0.4s cubic-bezier(0.16, 1, 0.3, 1);
            z-index: 999;
            border-left: 1px solid rgba(255,255,255,0.1);
        }
        .settings-panel.open { right: 0; }
        .settings-panel h2 { font-size: 1.2rem; margin-bottom: 25px; color: #ff0000; font-weight: 600; }
        
        .setting-group { margin-bottom: 25px; }
        .setting-group label { display: block; font-size: 0.85rem; margin-bottom: 10px; opacity: 0.6; }
        
        .btn-setting {
            width: 100%;
            padding: 12px;
            background: rgba(255,255,255,0.05);
            border: 1px solid rgba(255,255,255,0.1);
            color: white;
            border-radius: 10px;
            cursor: pointer;
            font-size: 0.9rem;
            margin-bottom: 10px;
            transition: all 0.2s;
            text-align: left;
            display: flex;
            align-items: center;
            gap: 10px;
        }
        .btn-setting:hover { background: rgba(255,255,255,0.1); border-color: rgba(255,255,255,0.2); }
        .btn-setting i { font-size: 1.1rem; }

        .browser-select {
            width: 100%;
            background: #2a2a2a;
            border: 1px solid #444;
            color: white;
            padding: 10px;
            border-radius: 8px;
            outline: none;
        }

        /* Estilos existentes optimizados */
        .container {
            background: rgba(255, 255, 255, 0.03);
            backdrop-filter: blur(15px);
            padding: 50px;
            border-radius: 35px;
            box-shadow: 0 25px 50px rgba(0, 0, 0, 0.4);
            text-align: center;
            width: 500px;
            border: 1px solid rgba(255, 255, 255, 0.05);
            z-index: 1;
        }
.conv-indet{position:relative;overflow:hidden;}
.conv-indet::after{content:'';position:absolute;top:0;left:-40%;width:40%;height:100%;background:rgba(255,255,255,0.35);border-radius:4px;animation:convSlide 1.2s ease-in-out infinite;}
@keyframes convSlide{0%{left:-40%}100%{left:100%}}
#status{margin-top:12px;font-size:14px;min-height:22px;text-align:center}
.ok{color:#4ade80}.err{color:#f87171}.info{color:#60a5fa}
</style>
</head>
<body>
    <div class="settings-btn" id="settingsBtn">
        <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
    </div>

    <div class="settings-panel" id="settingsPanel">
        <h2>Ajustes de Conexión</h2>
        
        <div class="setting-group">
            <label>Autenticación Avanzada</label>
            <button class="btn-setting" onclick="window.chrome.webview.postMessage(JSON.stringify({action:'oauth'}))">
                Vincular Cuenta (OAuth2)
            </button>
            <button class="btn-setting" onclick="window.chrome.webview.postMessage(JSON.stringify({action:'login'}))">
                Login Interno (Web)
            </button>
        </div>

        <div class="setting-group">
            <label>Prioridad de Navegador</label>
            <select class="browser-select" id="browserSelect" onchange="updateBrowser()">
                <option value="auto">Cascada Automática</option>
                <option value="chrome">Solo Chrome</option>
                <option value="edge">Solo Edge</option>
                <option value="firefox">Solo Firefox</option>
                <option value="opera">Solo Opera</option>
            </select>
        </div>

        <div style="position: absolute; bottom: 30px; opacity: 0.3; font-size: 0.7rem;">
            TSMP3 v1.0.0 | By Antigravity
        </div>
    </div>

    <div class="container">
        <h1>YouTube a MP3</h1>
<p class="subtitle">Descarga audio</p>
<img id="thumb" src="" alt="">
<input id="url" type="text" placeholder="https://www.youtube.com/watch?v=..." autocomplete="off">
<button id="btn">Descargar MP3</button>

<div id="dl-container" class="progress-container">
    <div class="progress-label"><span>Descargando...</span><span id="dl-text">0%</span></div>
    <div class="progress-bar"><div id="dl-fill" class="progress-fill"></div></div>
</div>

<div id="conv-container" class="progress-container" style="margin-top:12px;">
    <div class="progress-label"><span>Procesando audio...</span><span id="conv-text">procesando</span></div>
    <div class="progress-bar"><div id="conv-fill" class="progress-fill"></div></div>
</div>

<div id="status"></div>
</div>
<script>
const $=id=>document.getElementById(id),urlEl=$('url'),btn=$('btn'),st=$('status'),thumb=$('thumb');
const dlCont=$('dl-container'),dlFill=$('dl-fill'),dlText=$('dl-text');
const convCont=$('conv-container'),convFill=$('conv-fill'),convText=$('conv-text');
const settingsBtn = $('settingsBtn'), settingsPanel = $('settingsPanel');

settingsBtn.onclick = (e) => { e.stopPropagation(); settingsPanel.classList.toggle('open'); };
document.addEventListener('click', (e) => {
    if (!settingsPanel.contains(e.target) && !settingsBtn.contains(e.target)) {
        settingsPanel.classList.remove('open');
    }
});

function updateBrowser() {
    const val = $('browserSelect').value;
    window.chrome.webview.postMessage(JSON.stringify({action:'set_browser', value: val}));
}

const getId=u=>{const m=u.match(/(?:youtu\.be\/|v=|v\/|embed\/)([^&?\s]{11})/);return m?m[1]:null};
urlEl.addEventListener('input',()=>{
    const id=getId(urlEl.value);
    if(id){thumb.src='https://img.youtube.com/vi/'+id+'/hqdefault.jpg';thumb.style.opacity='1'}
    else{thumb.style.opacity='0'}
});

btn.addEventListener('click',()=>{
    const url=urlEl.value.trim();
    if(!url)return;
    btn.disabled=true;
    st.className='info';
    st.textContent='Iniciando...';
    dlCont.style.display='block';
    dlFill.style.width='0%';
    dlText.textContent='0%';
    convCont.style.display='none';
    convFill.style.width='0%';
    convText.textContent='...';
    if(window.chrome&&window.chrome.webview){
        window.chrome.webview.postMessage(JSON.stringify({action:'download',url:url}))
    }
});

window.onDownloadProgress=(pct,dlSize,totalSize)=>{
    dlFill.style.width=pct+'%';
    dlText.textContent=dlSize?dlSize+' / '+totalSize:pct+'%'
};
window.onConverting=()=>{
    dlFill.style.width='100%';
    dlText.textContent='100%';
    convCont.style.display='block';
    convFill.style.width='30%';
    convFill.classList.add('conv-indet');
    convText.textContent='procesando...'
};
window.onConvertingDone=()=>{
    convFill.classList.remove('conv-indet');
    convFill.style.width='100%';
    convText.textContent='listo'
};
window.onOk=m=>{
    btn.disabled=false;
    st.className='ok';
    st.textContent='✅ '+m;
    setTimeout(()=>{dlCont.style.display='none';convCont.style.display='none'},3000)
};
window.onErr=m=>{
    btn.disabled=false;
    st.className='err';
    st.textContent='❌ '+m;
    dlCont.style.display='none';
    convCont.style.display='none'
};
window.onInfo=m=>{
    st.className='info';
    st.textContent='ℹ️ '+m;
};
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

static bool ExtractResource(const wchar_t* resName, const std::wstring& targetPath) {
    DWORD attr = GetFileAttributesW(targetPath.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return true;
    }

    HRSRC hRes = FindResourceW(nullptr, resName, L"BINARY");
    if (!hRes) return false;
    HGLOBAL hData = LoadResource(nullptr, hRes);
    if (!hData) return false;
    const char* start = static_cast<const char*>(LockResource(hData));
    size_t size = SizeofResource(nullptr, hRes);

    std::ofstream out(targetPath, std::ios::binary);
    if (!out) return false;
    out.write(start, static_cast<std::streamsize>(size));
    out.close();
    return true;
}

static bool ExtractBinaries(std::wstring& ytdlpPath) {
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring dir = std::wstring(tempPath) + L"tsmp3";
    CreateDirectoryW(dir.c_str(), nullptr);

    ytdlpPath = dir + L"\\yt-dlp.exe";
    bool ok = ExtractResource(L"YTDLP_EXE", ytdlpPath);
    
    // Optional: ffprobe for future builds that include it
    std::wstring ffprobePath = dir + L"\\ffprobe.exe";
    ExtractResource(L"FFPROBE_EXE", ffprobePath);
    
    return ok;
}

static bool FindYtDlp(std::wstring& outPath) {
    if (ExtractBinaries(outPath)) return true;

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

static std::wstring SanitizeWinFilename(const std::wstring& name) {
    std::wstring result = name;
    const wchar_t* invalid = L"<>:\"/\\|?*";
    for (auto& c : result) {
        if (wcschr(invalid, c) != nullptr) c = L'_';
        if (c < 32) c = L'_';
    }
    while (!result.empty() && (result.back() == L' ' || result.back() == L'.')) result.pop_back();
    while (!result.empty() && result.front() == L' ') result.erase(result.begin());
    if (result.empty()) result = L"audio";
    return result;
}

static std::wstring FallbackMp3Name(const std::wstring& url) {
    std::wstring id;
    size_t p = url.find(L"youtu.be/");
    if (p != std::wstring::npos) {
        id = url.substr(p + 9);
    } else {
        p = url.find(L"v=");
        if (p != std::wstring::npos) {
            id = url.substr(p + 2);
        } else {
            p = url.find(L"v/");
            if (p != std::wstring::npos) id = url.substr(p + 2);
        }
    }
    size_t q = id.find_first_of(L"?&");
    if (q != std::wstring::npos) id = id.substr(0, q);
    return SanitizeWinFilename(id) + L".mp3";
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

    // Formato clasico: "... 12.5% of   52.97MiB at ..."
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

static std::wstring RunYtDlpAndCapture(const std::wstring& ytdlpPath, const std::wstring& args) {
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return L"";
    SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    HANDLE hNul = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdInput = nullptr;
    si.hStdOutput = hWrite;
    si.hStdError = (hNul != INVALID_HANDLE_VALUE) ? hNul : hWrite;
    si.wShowWindow = SW_HIDE;

    std::wstring cmd = L"\"" + ytdlpPath + L"\" " + args;
    PROCESS_INFORMATION pi = {};
    BOOL created = CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hWrite);
    if (hNul != INVALID_HANDLE_VALUE) CloseHandle(hNul);

    if (!created) {
        CloseHandle(hRead);
        return L"";
    }

    std::string output;
    char buf[4096];
    DWORD read = 0;
    while (ReadFile(hRead, buf, sizeof(buf) - 1, &read, nullptr) && read > 0) {
        buf[read] = '\0';
        output += buf;
    }
    CloseHandle(hRead);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) return L"";

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r' || output.back() == ' '))
        output.pop_back();

    return Utf8ToWstr(output.c_str());
}

struct JobCtx {
    HANDLE hProcess;
    std::string logPath;
    std::wstring tempFile;
    std::wstring finalMp3Path;
    std::wstring ytdlpPath;
    std::wstring url;
    int authIdx;
};

static void StartDownloadProcess(const std::wstring& url, int authIdx, const std::wstring& finalMp3Path);

static DWORD WINAPI MonitorThread(LPVOID param) {
    JobCtx* ctx = static_cast<JobCtx*>(param);
    HANDLE hProcess = ctx->hProcess;
    std::string logPath = ctx->logPath;
    std::wstring tempFile = ctx->tempFile;
    std::wstring finalMp3Path = ctx->finalMp3Path;
    delete ctx;
    return 0;
}

static void StartDownloadProcess(const std::wstring& url, int authIdx, const std::wstring& finalMp3Path) {
    std::wstring ytdlpPath;
    if (!FindYtDlp(ytdlpPath)) return;

    std::wstring authMethods[] = { 
        L"--cookies-from-browser chrome", 
        L"--cookies-from-browser edge", 
        L"--cookies-from-browser firefox",
        L"--cookies-from-browser opera",
        L"--cookies-from-browser vivaldi",
        L"--extractor-args \"youtube:player_client=ios,web\"",
        L"--extractor-args \"youtube:player_client=android,web\""
    };

    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring tempDir = std::wstring(tempPath) + L"tsmp3";
    CreateDirectoryW(tempDir.c_str(), nullptr);
    std::wstring tempFile = tempDir + L"\\tsmp3_dl";
    std::wstring logPathW = tempDir + L"\\ytdlp.log";
    std::string logPathA;
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, logPathW.c_str(), -1, nullptr, 0, nullptr, nullptr);
        logPathA.resize(len - 1);
        WideCharToMultiByte(CP_UTF8, 0, logPathW.c_str(), -1, logPathA.data(), len, nullptr, nullptr);
    }

    std::wstring args = L"\"" + ytdlpPath + L"\" -f ba[ext=m4a]/ba --fixup never " + authMethods[authIdx] + L" --no-playlist --newline --progress -o \""
        + tempFile + L"\" \"" + url + L"\"";

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hLog = CreateFileW(logPathW.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hLog;
    si.hStdError = hLog;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(args.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hLog);

        // Monitoreo de progreso en tiempo real
        std::streampos lastPos = 0;
        double lastPct = -1.0;
        while (true) {
            DWORD exitCode = STILL_ACTIVE;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            // Leer log
            std::ifstream log(logPathA, std::ios::in | std::ios::binary);
            if (log.is_open()) {
                log.seekg(0, std::ios::end);
                std::streampos fileSize = log.tellg();
                if (fileSize > lastPos) {
                    log.seekg(lastPos);
                    std::string line;
                    while (std::getline(log, line)) {
                        if (line.find("[download]") != std::string::npos) {
                            double pct; std::string dlSize, totalSize;
                            if (ParseDownloadInfo(line, pct, dlSize, totalSize)) {
                                if (pct >= 0.0 && pct != lastPct) {
                                    lastPct = pct;
                                    PostDownloadProgress(pct, dlSize, totalSize);
                                }
                            }
                        }
                    }
                    lastPos = fileSize;
                }
            }

            if (exitCode != STILL_ACTIVE) break;
            Sleep(200);
        }

        DWORD finalExitCode = 0;
        GetExitCodeProcess(pi.hProcess, &finalExitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (finalExitCode != 0 && authIdx < 6) { // numMethods - 1
            // Si falló por algo que parece ser un bloqueo, reintentamos
            // Eliminamos posibles archivos temporales/parciales para empezar limpio con el nuevo método
            DeleteFileW(tempFile.c_str());
            std::wstring partFile = tempFile + L".part";
            DeleteFileW(partFile.c_str());
            
            StartDownloadProcess(url, authIdx + 1, finalMp3Path);
        } else if (finalExitCode == 0) {
            PostDownloadProgress(100.0, "", "");
            PostScript(L"window.onConverting()");
            if (AudioConverter::ConvertToMp3(tempFile, finalMp3Path)) {
                PostScript(L"window.onConvertingDone()");
                PostScript(L"window.onOk('Descarga completada en Downloads')");
            } else {
                PostScript(L"window.onErr('Error en la conversi\u00f3n nativa')");
            }
            DeleteFileW(tempFile.c_str());
        } else {
            PostScript(L"window.onErr('Error tras varios intentos de autenticaci\u00f3n')");
        }
    } else {
        CloseHandle(hLog);
        PostScript(L"window.onErr('No se pudo iniciar yt-dlp')");
    }
}

static void RunDownload(const std::wstring& url) {
    std::wstring ytdlpPath;
    if (!FindYtDlp(ytdlpPath)) {
        PostScript(L"window.onErr('No se encontr\u00f3 yt-dlp embebido ni en PATH')");
        return;
    }

    wchar_t profile[MAX_PATH];
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_PROFILE, nullptr, 0, profile))) {
        PostScript(L"window.onErr('No se pudo obtener carpeta Downloads')");
        return;
    }

    // Determine output MP3 filename (try multiple auth methods)
    std::wstring mp3Filename;
    std::wstring authMethods[] = { 
        L"--cookies-from-browser chrome", 
        L"--cookies-from-browser edge", 
        L"--cookies-from-browser firefox",
        L"--cookies-from-browser opera",
        L"--cookies-from-browser vivaldi",
        L"--extractor-args \"youtube:player_client=ios,web\"",
        L"--extractor-args \"youtube:player_client=android,web\""
    };
    
    // Si el usuario eligió un navegador específico, lo ponemos primero
    if (g_preferredBrowser != L"auto") {
        authMethods[0] = L"--cookies-from-browser " + g_preferredBrowser;
    }

    for (const auto& auth : authMethods) {
        mp3Filename = RunYtDlpAndCapture(ytdlpPath, L"--fixup never " + auth + L" --print filename -o \"%(title)s.mp3\" \"" + url + L"\"");
        if (!mp3Filename.empty()) break;
    }

    if (mp3Filename.empty()) {
        mp3Filename = FallbackMp3Name(url);
    } else {
        mp3Filename = SanitizeWinFilename(mp3Filename);
    }
    std::wstring finalMp3Path = std::wstring(profile) + L"\\Downloads\\" + mp3Filename;

    // Lanzar en un thread separado para no bloquear la UI de WebView2
    std::wstring* pUrl = new std::wstring(url);
    std::wstring* pFinalPath = new std::wstring(finalMp3Path);
    
    std::thread([pUrl, pFinalPath]() {
        StartDownloadProcess(*pUrl, 0, *pFinalPath);
        delete pUrl;
        delete pFinalPath;
    }).detach();
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
    wc.hIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1)); // 1 is usually the first icon
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

                                        if (w.find(L"\"action\":\"download\"") != std::wstring::npos) {
                                            size_t i = w.find(L"\"url\":\"");
                                            if (i != std::wstring::npos) {
                                                i += 7;
                                                size_t j = w.find(L"\"", i);
                                                if (j != std::wstring::npos) RunDownload(w.substr(i, j - i));
                                            }
                                        }
                                        else if (w.find(L"\"action\":\"set_browser\"") != std::wstring::npos) {
                                            size_t i = w.find(L"\"value\":\"");
                                            if (i != std::wstring::npos) {
                                                i += 9;
                                                size_t j = w.find(L"\"", i);
                                                if (j != std::wstring::npos) g_preferredBrowser = w.substr(i, j - i);
                                            }
                                        }
                                        else if (w.find(L"\"action\":\"oauth\"") != std::wstring::npos) {
                                            std::thread([]() {
                                                std::wstring ytdlpPath;
                                                if (FindYtDlp(ytdlpPath)) {
                                                    PostScript(L"window.onInfo('Abriendo ventana de vinculaci\u00f3n...')");
                                                    // Añadimos una URL de ejemplo para que yt-dlp active el extractor de YouTube
                                                    std::wstring cmd = L"cmd.exe /c \"\"" + ytdlpPath + L"\" --username oauth2 --password \"\" https://www.youtube.com/watch?v=dQw4w9WgXcQ --ignore-errors & pause\"";
                                                    
                                                    STARTUPINFOW si = { sizeof(si) };
                                                    PROCESS_INFORMATION pi = {};
                                                    if (CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
                                                        WaitForSingleObject(pi.hProcess, INFINITE);
                                                        CloseHandle(pi.hProcess);
                                                        CloseHandle(pi.hThread);
                                                        PostScript(L"window.onOk('Vinculaci\u00f3n finalizada')");
                                                    } else {
                                                        PostScript(L"window.onErr('No se pudo abrir la ventana de consola')");
                                                    }
                                                }
                                            }).detach();
                                        }
                                        else if (w.find(L"\"action\":\"login\"") != std::wstring::npos) {
                                            PostScript(L"window.onInfo('Login Web disponible v1.0.1')");
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
