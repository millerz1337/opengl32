# Monarch Lite

Client-side OpenGL mod for Minecraft 1.8.9 (LWJGL 2). Injects into the game process and hooks a handful of GL functions from opengl32.dll at runtime. No config files to fiddle with, everything is toggled from the in-game menu.

## Features

- **Fullbright** - removes the lighting pass
- **NoHurtCam** - removes the camera shake on damage
- **ViewModel** - negative Z-axis rotations are dropped, so arrows render flat on the ground
- **Xray** - depth test forced to ALWAYS outside the viewmodel/GUI passes
- Toggle menu (DELETE by default) with a small settings panel for rebinding the menu key and the Xray hotkey

## Usage

1. Build or grab the DLL from `x64\Release\`.
2. Load it into the game (injector of your choice).
3. Open the menu with DELETE and toggle whatever you want.

Settings persist to `%LOCALAPPDATA%\Leet\config.ini`, so toggles and keybinds survive restarts.

## Build

- Visual Studio 2022, v143 toolset, Release x64
- No external dependencies - ImGui and MinHook are compiled in

```
msbuild src\cheat.vcxproj /p:Configuration=Release /p:Platform=x64
```

## Notes

- Windows 10/11 x64 only. The game needs to be running with an OpenGL context before the DLL is injected.
- Hooks are only installed while a module that uses them is enabled, the menu hook (wglSwapBuffers) stays active the whole time.
- Because it patches code in another process, antivirus software will likely flag it. Add an exclusion for the folder you run it from.
- Tested on 1.8.9 with Java 8. Other LWJGL 2 versions *should* work but nothing is guaranteed.