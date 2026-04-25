const COMPANION_URL = 'http://localhost:8765';

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (request.type === 'CHECK_COMPANION') {
    checkCompanion().then(sendResponse);
    return true;
  }

  if (request.type === 'DOWNLOAD') {
    startDownload(request.url, request.format).then(sendResponse);
    return true;
  }

  if (request.type === 'GET_PROGRESS') {
    getProgressStream(sendResponse);
    return true;
  }
});

async function checkCompanion() {
  try {
    const res = await fetch(`${COMPANION_URL}/`, { method: 'GET', mode: 'cors' });
    if (!res.ok) return { connected: false, error: 'Companion not responding' };
    const data = await res.json();
    return { connected: true, token: data.token };
  } catch (err) {
    return { connected: false, error: err.message };
  }
}

async function startDownload(url, format = 'mp3') {
  try {
    const res = await fetch(`${COMPANION_URL}/download`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ url, format }),
    });
    if (!res.ok) {
      const err = await res.text();
      return { success: false, error: err };
    }
    return { success: true };
  } catch (err) {
    return { success: false, error: err.message };
  }
}

function getProgressStream(sendResponse) {
  const es = new EventSource(`${COMPANION_URL}/progress`);

  es.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      chrome.runtime.sendMessage({ type: 'PROGRESS_UPDATE', data });
    } catch (e) {
      chrome.runtime.sendMessage({ type: 'PROGRESS_UPDATE', data: { type: 'raw', message: event.data } });
    }
  };

  es.onerror = () => {
    chrome.runtime.sendMessage({ type: 'PROGRESS_UPDATE', data: { type: 'error', message: 'Progress stream closed' } });
    es.close();
  };

  sendResponse({ streaming: true });
}
