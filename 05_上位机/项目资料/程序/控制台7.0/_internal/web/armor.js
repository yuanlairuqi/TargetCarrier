const colors = ['熄灭', '红色', '蓝色'];
const buttons = [...document.querySelectorAll('[data-color]')];
const message = document.getElementById('message');
let busy = false;
let polling = false;

async function refresh() {
    if (polling || busy || !window.pywebview?.api) return;
    polling = true;
    try {
        const state = await window.pywebview.api.snapshot();
        document.getElementById('requested').textContent = colors[state.requested] ?? '—';
        document.getElementById('color').textContent = colors[state.color] ?? '未知';
        message.textContent = state.message;
        for (const button of buttons) button.disabled = busy || !state.ready || state.pending;
    } catch (error) {
        for (const button of buttons) button.disabled = true;
        message.textContent = String(error.message || error);
    } finally {
        polling = false;
    }
}

for (const button of buttons) {
    button.addEventListener('click', async () => {
        if (busy) return;
        busy = true;
        for (const entry of buttons) entry.disabled = true;
        try {
            await window.pywebview.api.set_color(Number(button.dataset.color));
        } catch (error) {
            message.textContent = String(error.message || error);
        } finally {
            busy = false;
            await refresh();
        }
    });
}
window.addEventListener('pywebviewready', refresh, {once: true});
setInterval(refresh, 250);
refresh();
