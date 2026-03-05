# LG Inverter UI

3-button Web Bluetooth interface for your ESP32 BLE preset commands:

- `E` -> Emergency OFF
- `1` -> 50%
- `2` -> 70%
- `3` -> 90/95 toggle

Includes:

- Live metrics cards (connection, target/current %, PWM, last command)
- Debug monitor with BLE TX/RX log view

## Firmware for live metrics

Use the matching ESP32 sketch:

`firmware/esp32_ble_presets_debug.ino`

It sends status lines over BLE notify in this format:

`STAT cmd=1 target=50 current=47 pwm=446`

## One-command deploy (PowerShell)

Create a GitHub Personal Access Token (classic) with:

- `repo`
- `workflow`
- `pages` (if available on your account)

Then run:

```powershell
cd C:\Users\D347H\hvmicro\lg-inverter-ui
.\deploy.ps1 -GitHubUser YOUR_GITHUB_USERNAME -RepoName lg-inverter-ui -Token YOUR_TOKEN
```

After deploy, open:

`https://YOUR_GITHUB_USERNAME.github.io/lg-inverter-ui/`

Use Chrome/Edge on phone for Web Bluetooth support.
