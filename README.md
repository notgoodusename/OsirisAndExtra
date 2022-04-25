# Osiris And Extra

## Use a VAC bypass or you WILL get banned

A Free open-source cheat for **Counter-Strike: Global Offensive**, Based on [Osiris](https://github.com/danielkrupinski/Osiris). Designed as an internal cheat (DLL) loadable into game process. Compatible with the Steam version of the game. Only Available for Windows.

## Screenshots

someone add screenshots

## Features 
* **Aimbot** - aim assistance 
* **Fake angles** - makes your body appeared somewhere it's not
* **Triggerbot** - automatically fires when crosshair is on enemy 
* **Backtrack** - abuse lag compensation to move players back in time 
* **Glow** - render glow effect on entities 
* **Chams** - color player models to improve visibility 
* **Stream Proof ESP** - show information about players, dropped weapons and projectiles 
* **Visuals** - miscellaneous visual options 
* **Skin Changer** - use skins, stickers, knives and gloves that aren't in your inventory
* **Sound** - modify volume of certain sound effects 
* **Misc** - miscellaneous features 
* **Config** - JSON-based configuration system 

## Categories
*   **Legitbot**
     **1. Main window**
     **2. Backtrack**
     **3. Triggerbot**
     **4. LegitAA**
     **5. Config**
     
*   **Ragebot**
     **1. Main window**
     **2. Backtrack**
     **3. RageAA**
     **4. Fake angles**
     **5. Fake lag**

*   **Visuals**
     **1. Main window**
     **2. Stream Proof ESP**
     **3. Chams**
     **4. Glow**
     **5. Skins**

*   **Misc**
     **1. Main**
     **2. Sound**

*   **Config**
     **1. Main**


## Getting started

### Prerequisites
Microsoft Visual Studio 2019 16.10 (or newer), platform toolset v142 and Windows SDK 10.0.x.x are required in order to compile OsirisAndExtra. If you don't have them, you can download VS [here](https://visualstudio.microsoft.com/) (Windows SDK is installed during Visual Studio Setup).

### Downloading

There are two ways of downloading the source code:

#### Without [git](https://git-scm.com)

Choose this option if you want pure source and you're not going to contribute to the repo. Download size ~600 KB.

To download source code this way [click here](https://github.com/notgoodusename/OsirisAndExtra/archive/main.zip).

#### With [git](https://git-scm.com)

Choose this option if you're going to contribute to the repo or you want to use version control system. Download size ~4 MB. Git is required to step further, if not installed download it [here](https://git-scm.com).

Open git command prompt and enter following command:

    git clone --depth=1 https://github.com/notgoodusename/OsirisAndExtra.git

The `OsirisAndExtra` folder should have been successfully created, containing all the source files.

### Compiling from source

When you have equipped a copy of the source code, next step is opening **Osiris.sln** in Microsoft Visual Studio 2019.

Then change build configuration to `Release | x86` and simply press **Build solution**.

If everything went right you should receive `Osiris.dll`  binary file.

### Loading / Injecting into game process

Open your favorite [DLL injector](https://en.wikipedia.org/wiki/DLL_injection) and just inject `Osiris.dll` into `csgo.exe` process.

When injected, menu is openable under `INSERT` key.

### Optimizations
If your CPU supports AVX / AVX2 / AVX-512 instruction set, you can enable it in project settings. This should result in more performant code, optimized for your CPU. Currently SSE2 instructions are selected in project settings.

## FAQ

### How do I open menu?
Press <kbd>INSERT</kbd> while focused on CS:GO window.

### Where is my config file saved?
Configuration files are saved inside `Osiris` folder in your `Documents` folder (`%USERPROFILE%\Documents\Osiris`). The config is in human readable format and can be edited (e.g, using notepad). Sometimes after updates configuration file needs to be deleted and recreated.

### What hooking methods OsirisAndExtra uses?
Currently implemented hooking methods are:
*   MinHook - trampoline hook (preferred) 
*   VmtHook - hook a function directly in a vtable
*   VmtSwap - create a copy of a vtable and swap the pointer on the class instance

## Acknowledgments

* Thanks to [ocornut](https://github.com/ocornut) and [contributors](https://github.com/ocornut/imgui/graphs/contributors) for creating and maintaining an amazing GUI library - [Dear imgui](https://github.com/ocornut/imgui).

## License

> Copyright (c) 2018-2022 Daniel Krupiński

This project is licensed under the [MIT License](https://opensource.org/licenses/mit-license.php) - see the [LICENSE](https://github.com/notgoodusename/OsirisAndExtra/main/LICENSE) file for details.

## See also

[Osiris](https://github.com/danielkrupinski/Osiris) - Awesome free and open source cheat for CS:GO, the base for this cheat
