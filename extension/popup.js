document.addEventListener('DOMContentLoaded', async () => {
  const statusEl = document.getElementById('status');
  const openBtn = document.getElementById('openCompanion');

  try {
    const res = await chrome.runtime.sendMessage({ type: 'CHECK_COMPANION' });
    if (res.connected) {
      statusEl.textContent = 'Companion connected ✅';
      statusEl.className = 'status ok';
    } else {
      statusEl.textContent = 'Companion not running ❌';
      statusEl.className = 'status error';
      openBtn.style.display = 'block';
    }
  } catch (err) {
    statusEl.textContent = 'Extension error ❌';
    statusEl.className = 'status error';
  }

  openBtn.addEventListener('click', () => {
    chrome.tabs.create({ url: 'https://github.com/joaquin018/tstmp3/releases' });
  });
});
