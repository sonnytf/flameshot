<div align="center">
  <p>
    <h1>
      <img src="data/img/app/org.flameshot.Flameshot.svg" alt="Flameshot" width="64" height="64" />
      <br />
      Flameshot (跨平台強化版 & Wayland 深度優化版)
    </h1>
    <h4>懸浮 [Drag Me] 拖曳貼圖、內建離線 OCR 工具列、標註調整控制點、DPR 像素級縮放、純 Wayland 架構與零轉圈游標。</h4>
    <p>
      <a href="./README.md">English</a> · <b>繁體中文</b>
    </p>
  </p>
</div>

---

## 1. 專案概述 (Project Overview)

本專案為 **Flameshot** 的**跨平台（Windows、macOS 與 Linux 純 Wayland）**深度優化與生產力強化版本。

整合了標註動態調整控制點、懸浮 `[Drag Me]` 拖曳釘選分享、原生離線 `[OCR]` 工具列、高解析度多螢幕 DPR 像素級裁切，以及針對現代 Linux 桌面（Debian 13+、Ubuntu 26.04+、Fedora 43+）的純粹 Wayland 架構。

在現代 Wayland 環境下，官方原生版本常遭遇截圖啟動與退出時的游標轉圈卡頓（Mutter 15 秒 Busy Cursor）、釘選貼圖無法直接拖曳至其他應用程式、缺乏原生 OCR 文字識別，以及截圖處理程序結束後 Wayland 剪貼簿資料立即遺失等痛點。本專案透過徹底剝離過時的 X11 舊式代碼，建構純粹的 Wayland 架構，並內建懸浮拖曳分享與離線 OCR 等高效率生產力功能，同時提供全平台自動化 GitHub Releases 發行。

---

## 2. 核心功能強化 (Key Enhancements)

### 🧲 懸浮 `[Drag Me]` 拖曳釘選貼圖
- **暗黑磨砂抗鋸齒膠囊**：釘選貼圖視窗底部正中央內建抗鋸齒半透明深色膠囊按鈕，具備經典 6 點握把紋理。
- **直接拖曳分享 (`text/uri-list`)**：點擊拖曳膠囊即可將圖片直接丟進 Chrome、Firefox、Slack、Discord、Telegram、檔案管理員或郵件客戶端。
- **拖曳自動淡出 (15% 不透明度)**：拖曳過程中貼圖視窗自動淡出至 15% 透明度，確保清楚透視下層目標放置視窗。
- **釋放自動關閉與保留模式**：拖曳放置成功後自動關閉貼圖視窗；若拖曳時按住 `Shift` 鍵，則放置完成後依然保留視窗。
- **平滑透明度調節**：支援 `Alt + 滑鼠滾輪` 即時調整視窗透明度（底層以 GPU Alpha Backing Buffer 渲染，消除 Wayland QPA 警告）。

### 🔍 截圖工具列原生 `[OCR]` 文字識別
- **原生 OCR 動作工具**：截圖選區工具列直接內建 `[OCR]` 按鈕（`CaptureTool::TYPE_OCR = 25`）。
- **中英雙語離線識別**：採用本地 Tesseract OCR 引擎，預設針對繁體中文與英文（`chi_tra+eng`）混合文本最佳化。
- **直達 Wayland 剪貼簿**：識別後的純文字自動標準化並寫入 Wayland 系統剪貼簿，附帶系統通知回饋。

### ⚡ Wayland 零轉圈游標架構 (Zero-Spinning Cursor)
- **根除 `MetaStartupSequence` 轉圈**：徹底解決 Mutter (`libmutter-18`) 因殘留 `xdg_activation_v1` Token 導致的 15 秒 Busy Cursor 問題。
- **`Qt::WA_ShowWithoutActivating`**：所有 Overlay 與視窗映射均不觸發異常 `requestActivateWindow()`。
- **鍵盤即時焦點**：完整保留原生 `wl_keyboard.enter` 焦點對齊，`Esc`、`Ctrl+C` 及各類快捷鍵即按即響應。

### 🛡️ 純 Wayland 現代核心與雙二進制架構
- **精簡 X11 遺留代碼**：全面移除舊式 X11 截圖截取、視窗 ID 檢索與專屬設定選項（`useX11LegacyScreenshot`、`captureActiveMonitor`）。
- **雙二進制生命週期**：透過常駐背景守護行程（`flameshot-daemon`）持久保管 Wayland 剪貼簿 MIME 資料，`flameshot gui` 則作為輕量瞬時 Portal 客戶端運作。

### 📐 標註圖案動態調整控制點 (Dynamic Resize Handles)
- **八向與端點控制握把**：已選取的矩形、圓形、反色與馬賽克方塊顯示 8 個周邊調整點；直線、箭頭與螢光筆具備端點調整點；對話氣泡/序號計數器支援拖曳箭頭指標尖端；畫筆自由路徑支援外框 8 向等比例縮放。
- **直覺方向游標反饋**：游標移至控制點時自動切換方向調整游標（`SizeHor`、`SizeVer`、`SizeFDiag`、`SizeBDiag`、`OpenHand`、`ClosedHand`）。
- **復原與刪除安全機制**：在拖曳調整過程若物件被刪除或執行 Undo，立即安全取消拖曳行為，避免記憶體或座標異常。

### 🖥️ 原生 DPR 縮放與像素級精確裁剪 (`dprscaling`)
- **保留螢幕真實像素**：在分數縮放（如 125%、150%、175%）的合成器環境下，保留截圖原生解析度，防止模糊二次放大。
- **邊緣對齊轉換演算法**：選區轉設備像素時直接縮放邊界座標（Edges）而非僅依長寬換算，避免 fractional ratio 下出現單像素偏差或邊界接縫。
- **跨螢幕取色滴管對齊**：滴管取色工具完整校正多顯示器座標偏移與個別 DPR 比例，Wayland 多螢幕放大鏡取色完全精準。

### 📦 自動化全平台打包與 GitHub Release 發行
- **GitHub Actions 自動化**：支援 Linux Pure Wayland（Debian 13 / Ubuntu 26.04 的 `.deb`、Fedora 43 / 44 的 `.rpm`）、Windows（`.msi` 安裝檔、便攜版 `.zip`）與 macOS（`.dmg`），推送 `v*` 標籤時自動建構並透過 `softprops/action-gh-release@v2` 附加發行資產。

---

## 3. 安裝與編譯指南 (Installation & Build Guide)

### 預編譯版本下載 (Pre-built Releases)
直接從我們的 [GitHub Releases](https://github.com/sonnytf/flameshot/releases) 下載開箱即用的二進位安裝包：
- **Linux Pure Wayland**：`.deb` (Debian 13+, Ubuntu 26.04+), `.rpm` (Fedora 43+)
- **Windows (x64)**：`.msi` 安裝檔與免安裝便攜版 `.zip`
- **macOS (Apple Silicon & Intel)**：`.dmg` 安裝映象檔

---

### Linux Wayland 本地編譯與 Systemd 常駐設定

#### 執行期依賴套件 (Runtime Dependencies)
安裝系統 Portal 代理與離線 OCR 語言包：
```bash
sudo apt-get update
sudo apt-get install -y \
    xdg-desktop-portal-gnome \
    tesseract-ocr \
    tesseract-ocr-chi-tra \
    tesseract-ocr-eng
```

#### Docker 無污染乾淨編譯 (Clean-Room Build)
於專案根目錄執行 Docker 一鍵打包 `.deb`：
```bash
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
    -DDISABLE_UPDATE_CHECKER=OFF \
    -DCPACK_GENERATOR=DEB

  cmake --build build -j\$(nproc)
  cpack -G DEB --config build/CPackConfig.cmake -B build
"
```

#### 安裝 `.deb` 套件
```bash
sudo dpkg -i build/flameshot-*.deb
```

### 配置 Systemd 使用者常駐服務
確保截圖完成關閉後，剪貼簿內容依然完好留存：
```bash
# 1. 建立 Daemon 包裝執行檔
sudo tee /usr/local/bin/flameshot-daemon > /dev/null << 'EOF'
#!/usr/bin/env bash
exec /usr/bin/flameshot
EOF
sudo chmod +x /usr/local/bin/flameshot-daemon

# 2. 配置 Systemd User Service
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

# 3. 啟用並啟動服務
systemctl --user daemon-reload
systemctl --user enable --now flameshot.service
```

---

## 4. 操作與快捷鍵 (Usage & Shortcuts)

### 常用命令列指令
| 指令 | 說明 |
| :--- | :--- |
| `flameshot gui` | 啟動互動式截圖視窗 |
| `flameshot full -c` | 全螢幕截圖並立即複製至剪貼簿 |
| `flameshot config` | 開啟圖形化設定介面 |

### 釘選貼圖視窗操作
| 操作目標 | 快捷鍵 / 手勢 | 行為效果 |
| :--- | :--- | :--- |
| **拖曳圖片** | 點擊並按住 `[Drag Me]` 膠囊拖曳 | 將圖片拖曳至外部應用程式（視窗淡出至 `15%`） |
| **保留視窗** | 拖曳時按住 `Shift` 鍵 | 放置完成後不關閉貼圖視窗 |
| **視窗透明度** | `Alt + 滑鼠滾輪` | 平滑調節釘選貼圖透明度 |
| **縮放視窗** | `滑鼠滾輪` 或 `觸控板手勢` | 放大 / 縮小釘選圖片尺寸 |
| **複製圖片** | `Ctrl + C` | 複製釘選圖片至剪貼簿 |
| **關閉貼圖** | `Esc` / `Ctrl + Q` / 雙擊左鍵 | 關閉釘選貼圖視窗 |

---

## 授權條款 (License)

GPL-3.0-or-later
