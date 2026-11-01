<div align="center">
  <p>
    <h1>
      <img src="data/img/app/org.flameshot.Flameshot.svg" alt="Flameshot" width="64" height="64" />
      <br />
      Flameshot (Ubuntu 26.04+ Wayland Edition)
    </h1>
    <h4>Floating [Drag Me] pin, offline OCR toolbar, pure Wayland stack & zero spinning cursor.</h4>
    <p>
      <b>English</b> · <a href="./README.zh-TW.md">繁體中文</a>
    </p>
  </p>
</div>

---

## 1. Project Overview

This is a customized, production-tuned fork of **Flameshot** engineered specifically for **Ubuntu 26.04 LTS (and newer) under GNOME Wayland**.

Standard upstream Flameshot builds on modern Wayland desktops suffer from spinning busy cursors on capture launch/exit, missing drag-and-drop workflows for pinned screenshots, lack of integrated OCR, and clipboard data loss when short-lived GUI processes terminate. This edition solves these challenges by transitioning to a pure Wayland architecture, stripping legacy X11 code paths, and introducing streamlined drag-and-drop and offline OCR workflows.

---

## 2. Key Enhancements

### 🧲 Floating `[Drag Me]` Pin & Drop
- **Frosted Dark Capsule**: Every pinned screenshot features an antialiased, semi-transparent dark floating badge at bottom-center with a subtle 6-dot grip texture.
- **Drag-and-Drop (`text/uri-list`)**: Drag directly from the pinned screenshot badge into web browsers, Slack, Discord, Telegram, file managers, or email clients.
- **Visual Target Visibility (15% Opacity Fade)**: While actively dragging, the pin window automatically fades to 15% opacity so you can clearly see the underlying drop target.
- **Auto-Close on Drop**: The pin window automatically closes when the drop operation succeeds. Holding `Shift` during drag retains the pinned window on screen.
- **Smooth Opacity Adjustment**: Adjust pin transparency on the fly with `Alt + Mouse Wheel` (rendered via GPU alpha backing buffer, avoiding Wayland QPA errors).

### 🔍 Native Capture Toolbar `[OCR]`
- **Offline OCR Action Tool**: An integrated `[OCR]` button resides directly on the capture selection toolbar (`CaptureTool::TYPE_OCR = 25`).
- **Bilingual Extraction**: Leverages local Tesseract OCR configured for Traditional Chinese and English (`chi_tra+eng`) with automatic fallback.
- **Instant Wayland Clipboard Copy**: Extracted text is instantly normalized and placed onto the Wayland clipboard accompanied by a desktop notification.

### ⚡ Wayland Zero-Spinning-Cursor Architecture
- **Elimination of `MetaStartupSequence` Busy Cursors**: Bypasses the 15-second spinning cursor bug caused by orphan `xdg_activation_v1` tokens in Mutter (`libmutter-18`).
- **`Qt::WA_ShowWithoutActivating`**: Overlay and dialog windows map cleanly without triggering rogue `requestActivateWindow()` token requests.
- **Immediate Input Focus**: Retains native `wl_keyboard.enter` focus mapping so `Esc`, `Ctrl+C`, and custom shortcuts respond immediately.

### 🛡️ Pure Wayland Stack & Dual-Binary Resilience
- **X11 Code Stripped**: Removed obsolete X11 legacy grabber fallback paths, window ID queries, and X11-only configuration switches (`useX11LegacyScreenshot`, `captureActiveMonitor`).
- **Dual-Binary Architecture**: Runs an isolated background daemon (`flameshot-daemon`) to hold Wayland clipboard MIME data persistently, while `flameshot gui` acts as a fast, ephemeral portal client.

---

## 3. Build & Installation (Ubuntu 26.04 LTS)

### Runtime Dependencies
Install the required system portal and OCR language packs:
```bash
sudo apt-get update
sudo apt-get install -y \
    xdg-desktop-portal-gnome \
    tesseract-ocr \
    tesseract-ocr-chi-tra \
    tesseract-ocr-eng
```

### Docker Clean-Room Build
To build a reproducible `.deb` without polluting your host development environment:
```bash
# From repository root
mkdir -p build

docker run --rm -v "$(pwd):/src" -w /src ubuntu:26.04 bash -c "
  set -e
  sed -i 's|http://archive.ubuntu.com/ubuntu/|http://free.nchc.org.tw/ubuntu/|g' /etc/apt/sources.list.d/ubuntu.sources 2>/dev/null || true
  sed -i 's|http://security.ubuntu.com/ubuntu/|http://free.nchc.org.tw/ubuntu/|g' /etc/apt/sources.list.d/ubuntu.sources 2>/dev/null || true
  apt-get update
  apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build git ca-certificates pkg-config python3 \
    qt6-base-dev qt6-base-private-dev qt6-svg-dev qt6-tools-dev qt6-tools-dev-tools \
    qt6-l10n-tools libkf6guiaddons-dev libkdsingleapplication-qt6-dev file

  cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DUSE_WAYLAND_CLIPBOARD=ON \
    -DUSE_WAYLAND_GRIM=OFF \
    -DENABLE_CACHE=OFF \
    -DUSE_KDSINGLEAPPLICATION=ON \
    -DDISABLE_UPDATE_CHECKER=ON \
    -DCPACK_GENERATOR=DEB

  cmake --build build -j\$(nproc)
  cpack --config build/CPackConfig.cmake -B build
"
```

### Installing the `.deb` Package
```bash
sudo dpkg -i build/flameshot-*.deb
```

### Systemd User Daemon Setup
Ensure clipboard contents persist after closing capture sessions:
```bash
# 1. Create wrapper binary for daemon
sudo tee /usr/local/bin/flameshot-daemon > /dev/null << 'EOF'
#!/usr/bin/env bash
exec /usr/bin/flameshot
EOF
sudo chmod +x /usr/local/bin/flameshot-daemon

# 2. Configure systemd user service
mkdir -p ~/.config/systemd/user
tee ~/.config/systemd/user/flameshot.service > /dev/null << 'EOF'
[Unit]
Description=Flameshot Screenshot Service (Wayland Daemon)
PartOf=graphical-session.target
After=graphical-session.target
ConditionEnvironment=WAYLAND_DISPLAY

[Service]
Type=simple
ExecStart=/usr/local/bin/flameshot-daemon
Restart=on-failure
RestartSec=2s
SuccessExitStatus=143
Environment=QT_QPA_PLATFORM=wayland

[Install]
WantedBy=graphical-session.target
EOF

# 3. Enable and start daemon
systemctl --user daemon-reload
systemctl --user enable --now flameshot.service
```

---

## 4. Usage & Shortcuts

### CLI Commands
| Command | Action |
| :--- | :--- |
| `flameshot gui` | Open interactive screenshot capture overlay |
| `flameshot full -c` | Capture full desktop and copy directly to clipboard |
| `flameshot config` | Open graphical settings window |

### Pinned Screenshot Controls
| Action | Shortcut / Gesture | Behavior |
| :--- | :--- | :--- |
| **Drag & Drop** | Click & drag `[Drag Me]` badge | Drags image into external application (`15%` opacity fade) |
| **Keep Open on Drop** | Hold `Shift` while dragging | Keeps pin window open after drop completes |
| **Window Opacity** | `Alt + Mouse Wheel` | Smoothly adjust pin transparency |
| **Image Zoom** | `Mouse Wheel` / `Pinch` | Zoom in and zoom out of pinned screenshot |
| **Copy Pixmap** | `Ctrl + C` | Copy pinned image to clipboard |
| **Close Pin** | `Esc` / `Ctrl + Q` / Double Click | Close pinned window |

---

## License

GPL-3.0-or-later
