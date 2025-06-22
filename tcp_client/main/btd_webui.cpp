
#include "btd_webui.h"

static const char* WEB_UI_HTML_CONTENT = R"RAW_HTML(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>ti:ma Config</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif; background-color: #f0f2f5; margin: 0; padding: 20px; }
        .container { max-width: 500px; margin: auto; padding: 20px; background-color: white; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        h1, h2 { color: #333; }
        label { display: block; margin-top: 15px; font-weight: bold; color: #555; }
        input[type=number] { width: 100%; padding: 10px; margin-top: 5px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; font-size: 16px; }
        .checkbox-container { display: flex; align-items: center; margin-top: 20px; }
        .checkbox-container input { width: auto; margin-right: 10px; }
        button { background-color: #007bff; color: white; padding: 12px 15px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; margin-top: 20px; width: 100%; }
        button:hover { background-color: #0056b3; }
        .reset-btn { background-color: #dc3545; }
        .reset-btn:hover { background-color: #c82333; }
        .status { padding: 10px; margin-bottom: 20px; border-radius: 4px; display: none; text-align: center; }
        .status.success { background-color: #d4edda; color: #155724; display: block; }
        .status.error { background-color: #f8d7da; color: #721c24; display: block; }
    </style>
</head>
<body>
<div class="container">
    <h1>ti:ma Configuration</h1>
    <div id="status-message" class="status"></div>

    <form id="config-form">
        <h2>Settings</h2>
        <label for="workTime">Work Time (seconds):</label>
        <input type="number" id="workTime" name="workTime" required>
        
        <label for="breakTime">Break Time (seconds):</label>
        <input type="number" id="breakTime" name="breakTime" required>

        <label for="longBreakTime">Long Break Time (seconds):</label>
        <input type="number" id="longBreakTime" name="longBreakTime" required>

        <label for="sessionCount">Sessions until Long Break:</label>
        <input type="number" id="sessionCount" name="sessionCount" required>

        <label for="timeout">Timeout (seconds):</label>
        <input type="number" id="timeout" name="timeout" required>

        <div class="checkbox-container">
            <input type="checkbox" id="gestureEnabled" name="gestureEnabled">
            <label for="gestureEnabled" style="margin-top:0;">Enable Break Gesture</label>
        </div>
        
        <button type="submit">Save Settings</button>
    </form>
    
    <hr style="margin-top: 30px; border-top: 1px solid #eee;">
    <h2>Advanced</h2>
    <button id="reset-button" class="reset-btn">Factory Reset</button>
</div>

<script>
    document.addEventListener('DOMContentLoaded', () => {
        const form = document.getElementById('config-form');
        const resetButton = document.getElementById('reset-button');
        const statusDiv = document.getElementById('status-message');

        const showStatus = (message, isError = false) => {
            statusDiv.textContent = message;
            statusDiv.className = 'status ' + (isError ? 'error' : 'success');
            setTimeout(() => { statusDiv.style.display = 'none'; }, 4000);
        };
        
        const loadConfig = async () => {
            try {
                const response = await fetch('/config');
                if (!response.ok) throw new Error('Failed to load settings');
                const config = await response.json();
                document.getElementById('workTime').value = config.workTimeSeconds;
                document.getElementById('breakTime').value = config.breakTimeSeconds;
                document.getElementById('longBreakTime').value = config.longBreakTimeSeconds;
                document.getElementById('sessionCount').value = config.longBreakSessionCount;
                document.getElementById('timeout').value = config.timeoutSeconds;
                document.getElementById('gestureEnabled').checked = config.breakGestureEnabled;
            } catch (error) {
                showStatus(error.message, true);
            }
        };

        form.addEventListener('submit', async (e) => {
            e.preventDefault();
            const formData = {
                workTimeSeconds: parseInt(form.workTime.value, 10),
                breakTimeSeconds: parseInt(form.breakTime.value, 10),
                longBreakTimeSeconds: parseInt(form.longBreakTime.value, 10),
                longBreakSessionCount: parseInt(form.sessionCount.value, 10),
                timeoutSeconds: parseInt(form.timeout.value, 10),
                breakGestureEnabled: form.gestureEnabled.checked
            };

            try {
                const response = await fetch('/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(formData)
                });
                const resultText = await response.text();
                showStatus(resultText, !response.ok);
            } catch (error) {
                showStatus('Save failed: ' + error.message, true);
            }
        });

        resetButton.addEventListener('click', async () => {
            if (!confirm('Are you sure? This will erase ALL settings and WiFi locations.')) return;
            try {
                const response = await fetch('/factoryreset', { method: 'POST' });
                const resultText = await response.text();
                showStatus(resultText, !response.ok);
                if (response.ok) { loadConfig(); } // Reload form to show defaults
            } catch (error) {
                showStatus('Reset failed: ' + error.message, true);
            }
        });
        
        loadConfig();
    });
</script>
</body>
</html>
)RAW_HTML";

extern "C" const char* get_web_ui_html() {
    return WEB_UI_HTML_CONTENT;
}
