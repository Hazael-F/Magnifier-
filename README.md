# ⚠️ Archived Legacy Version (v1.0.2)  
This branch (`archive/main`) is **deprecated** and preserved for historical reference only.  
**Active development continues in [`master`](https://github.com/Hazael-F/Magnifier-).**  

---

### **About This Version**  
- **Code Quality**: This branch contains early, unrefactored code (`main.cpp`-only implementation).  
- **Formatting**: Poorly organized
- **Latest Release**: [v1.0.2](https://github.com/Hazael-F/Magnifier-/releases/tag/v1.0.2)  
  - 🔗 [Direct Download](https://github.com/Hazael-F/Magnifier-/releases/download/v1.0.2/Magnifier+_v1.0.2.zip)  

---

### **Legacy Features**  
A lightweight screen magnifier for Windows with:  
- 5-level zoom (mouse wheel + right-click)  
- Arrow key positioning adjustments  
- System tray integration  
- INI configuration  

---

### **Why This Branch Exists**  
- Preserves the original state of Magnifier 1.0.2.  
- Demonstrates project evolution (compare with `master` for improvements).  

---

### **⚠️ Limitations**  
1. **Monolithic Code**: All logic in `main.cpp` (no separation of concerns).  
2. **Technical Debt**: Hardcoded values, minimal error handling.  
3. **Deprecated**: No further updates or bug fixes.  

---

### 🙏 Credits

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
- You, for reading this!

=======================================================================

### ⚙️ Configuration
Edit `MagnifierPlus.ini` to customize:
```ini
[Window]
Width=300
Height=300
Circular=1    ;  0 for circular  |  1 for cubic

[Tracking]
Mouse=1       ;  0 for centered  |  1 for mouse-tracked

[Zoom]
AreaSize=100

[Performance]
RefreshRate=60

[Movement]
StepSize=5    ;  for manual alignment with arrow keys

[Adjustments]
Horizontal=0  ;  offsets the window by steps (left-right)
Vertical=0    ;  offsets the window by steps (up-down)
