# 🔍 Magnifier+ 

*A lightweight, customizable screen magnifier for Windows with smooth zoom controls and system tray accessibility.*

## ✨ Features
- **5-level smooth zoom** (1x to 5x) via mouse wheel + right-click
- **Position adjustment** with arrow keys (hold right-click)
- **System tray integration** for quick access
- **Custom reticle** targeting overlay
- **INI configurable** settings (window size, zoom area, FPS, offsets)
- **Lightweight** (uses native Windows Magnification API)

## 🛠️ How to Use
1. **Run** `Magnifier+.exe`.
2. **Hold right-click + scroll** to zoom in/out.
3. **Hold right-click + arrow keys** to adjust position.
4. **Right-click system tray icon** to exit.

### ⚙️ Configuration
Edit `MagnifierPlus.ini` to customize:
```ini
[Settings]
WindowWidth=300       ; Magnifier window width
WindowHeight=300      ; Magnifier window height
BaseZoomSize=100      ; Base zoom area size (pixels)
InitialLeftOffset=-100 ; Default X offset
InitialDownOffset=-100 ; Default Y offset
TargetFPS=71          ; Refresh rate
OffsetStep=5          ; Position adjustment step size