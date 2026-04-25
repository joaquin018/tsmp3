// Bridge between web page and extension
window.addEventListener('message', async (event) => {
  // Only accept messages from the same page
  if (event.source !== window) return;

  if (event.data?.type === 'YTMP3_DOWNLOAD') {
    try {
      const res = await chrome.runtime.sendMessage({
        type: 'DOWNLOAD',
        url: event.data.url,
        format: event.data.format || 'mp3',
      });
      window.postMessage({ type: 'YTMP3_RESPONSE', id: event.data.id, ...res }, '*');
    } catch (err) {
      window.postMessage({ type: 'YTMP3_RESPONSE', id: event.data.id, success: false, error: err.message }, '*');
    }
  }

  if (event.data?.type === 'YTMP3_CHECK') {
    try {
      const res = await chrome.runtime.sendMessage({ type: 'CHECK_COMPANION' });
      window.postMessage({ type: 'YTMP3_CHECK_RESPONSE', id: event.data.id, ...res }, '*');
    } catch (err) {
      window.postMessage({ type: 'YTMP3_CHECK_RESPONSE', id: event.data.id, connected: false, error: err.message }, '*');
    }
  }
});

// Notify page that extension is present
document.addEventListener('DOMContentLoaded', () => {
  window.postMessage({ type: 'YTMP3_EXTENSION_READY' }, '*');
});
