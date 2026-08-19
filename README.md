# opengl32

Client-side OpenGL mod for Minecraft 1.8.9 (LWJGL 2). Injects into the game process and hooks a handful of GL functions from opengl32.dll at runtime. No config files to fiddle with, everything is toggled from the in-game menu.

## Features

- **Fullbright** - removes the lighting pass
- **NoHurtCam** - removes the camera shake on damage
- **ViewModel** - negative Z-axis rotations are dropped, so arrows render flat on the ground
- **Xray** - depth test forced to ALWAYS outside the viewmodel/GUI passes
- Toggle menu (DELETE by default) with a settings panel for rebinding the menu key and the Xray hotkey

## Screenshots

![Menu](https://raw.githubusercontent.com/millerz1337/opengl32/d971444e1be807d42ffd808f2dec13a930080180/assets/images/Screenshot_3.png)

![Menu](https://raw.githubusercontent.com/millerz1337/opengl32/d971444e1be807d42ffd808f2dec13a930080180/assets/images/Screenshot_2.png)

![Görsel](https://raw.githubusercontent.com/millerz1337/opengl32/81c11401efb7f772d61dfe737912e0ae9bda656b/assets/images/Screenshot_1.png)

## Usage

1. Build or grab the DLL from `x64\Release\`.
2. Load it into the game (injector of your choice).
3. Open the menu with DELETE and toggle whatever you want.

Right-click anywhere on the menu to open the Settings panel, where you can rebind the menu key and the Xray hotkey.

Settings persist to `%LOCALAPPDATA%\Leet\config.ini`, so toggles and keybinds survive restarts.

## Build

- Visual Studio 2022, v143 toolset, Release x64
- No external dependencies - ImGui and MinHook are compiled in

```
msbuild src\cheat.vcxproj /p:Configuration=Release /p:Platform=x64
```

## License

MIT
