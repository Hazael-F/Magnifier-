# 🔍 Magnifier+ 

Link to Latest Release (v1.0.1): https://github.com/Hazael-F/Magnifier-/releases/tag/v1.0.1

DIRECT DOWNLOAD: https://github.com/Hazael-F/Magnifier-/releases/download/v1.0.1/Magnifier+_v1.0.1.zip

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

=========================================================================

## 🙏 Credits

**Magnifier+** was developed with the help of these resources and contributors:

### Core Development
- [@Hazael-F](https://github.com/Hazael-F) - Main developer
- [DeepSeek Chat](https://deepseek.com) - AI coding assistant

### Libraries & APIs
- [Windows Magnification API](https://learn.microsoft.com/en-us/windows/win32/api/_magapi/) - Powering the zoom functionality
- `shlwapi.lib` - For configuration file handling
- Windows GDI - Graphics and overlay rendering

### Special Thanks
- Microsoft Docs team for API documentation
- Open-source screen magnifier projects for inspiration

=======================================================================

### ⚙️ Configuration
Edit `MagnifierPlus.ini` to customize:
```ini
[Window]
Width=300
Height=300

[Zoom]
Size=100

[Movement]
StepSize=5

[Adjustments]
Horizontal=0
Vertical=0

[Performance]
FPS=60
