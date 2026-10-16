# Skip Equip Animation
  - Adds two animation variables using the BehaviorDataInjector.
  - The 'SkipEquipAnimation' bool variable controls the skipping process.
  - The 'LoadBoundObjectDelay' integer variable controls the delay (milliseconds) of loading bound object. 0 means twice the current delta time of the game.
  - You can set these animation variables with any method.

    For example the Papyrus scripting:
    
    - > _Game.GetPlayer().SetAnimationVariableBool("SkipEquipAnimation", true)_
    
    [Click for more info](https://ck.uesp.net/wiki/SetAnimationVariableBool_-_ObjectReference)

    or using Payload Interpreter mod's features:
    
    - > _PIE.@SGVB|SkipEquipAnimation|1_
    
    [Click for more info](https://github.com/D7ry/PayloadInterpreter)

    If you want equip animations to be skipped by default: Simply open the 'SKSE\Plugins\BehaviorDataInjector\SkipEquipAnimation_BDI.json' and set the 'SkipEquipAnimation' variable's value to true.

  - Compatible with 1.5.97 - 1.6.x - 1.7.x game versions.

  - [NexusMods Page](https://www.nexusmods.com/skyrimspecialedition/mods/120926)

# CommonLibSSE NG

Because this uses [CommonLibSSE NG](https://github.com/alandtse/CommonLibSSE-NG), it supports Skyrim SE, AE, GOG, and VR. 

Hook IDs and offsets must still be found manually for each version.

- Set the `CommonLibSSEPath_NG` environment variable to the path of your CommonlibSSE-NG folder. 

# Requirements

- [Visual Studio 2022](https://visualstudio.microsoft.com/) (_the free Community edition_)
- [`vcpkg`](https://github.com/microsoft/vcpkg)
  - 1. Clone the repository using git OR [download it as a .zip](https://github.com/microsoft/vcpkg/archive/refs/heads/master.zip)
  - 2. Go into the `vcpkg` folder and double-click on `bootstrap-vcpkg.bat`
  - 3. Edit your system or user Environment Variables and add a new one:
    - Name: `VCPKG_ROOT`  
      Value: `C:\path\to\wherever\your\vcpkg\folder\is`
  - The latest version of vcpkg needs a default repository defined in the json. If you're using an older version of vcpkg, simply delete the default repository definition in `vcpkg-configuration.json`

## Opening the project

Once you have Visual Studio 2022 installed, you can open this folder in basically any C++ editor, e.g. [VS Code](https://code.visualstudio.com/) or [CLion](https://www.jetbrains.com/clion/) or [Visual Studio](https://visualstudio.microsoft.com/)
- > _for VS Code, if you are not automatically prompted to install the [C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) and [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools) extensions, please install those and then close VS Code and then open this project as a folder in VS Code_

# Project setup

By default, when this project compiles it will output a `.dll` for your SKSE plugin into the `build/` folder.

If you want to configure this project to output your plugin files
into your Skyrim Special Edition's "`Data`" folder:

- Set the `SKYRIM_FOLDER` environment variable to the path of your Skyrim installation  
  e.g. `C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition`

If you want to configure this project to output your plugin files
into your "`mods`" folder:  
(_for Mod Organizer 2 or Vortex_)

- Set the `SKYRIM_MODS_FOLDER` environment variable to the path of your mods folder:  
  e.g. `C:\Users\<user>\AppData\Local\ModOrganizer\Skyrim Special Edition\mods`  
  e.g. `C:\Users\<user>\AppData\Roaming\Vortex\skyrimse\mods`

# Debugging
In order to attach a debugger, you must own a legal copy of Skyrim with the exe stripped using Steamless. Note that users with MO2 should have `-forcesteamloader` as an SKSE argument for plugins to load normally with a stub-removed exe.

For VSCode users, they must have a `launch.json` file like the one below:

```json

{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "C++ Debugger",
            "type": "cppvsdbg",
            "request": "attach"
        },
        {
            "type": "cmake",
            "request": "launch",
            "name": "Debug portfile(s)",
            "cmakeDebugType": "external",
            "pipeName": "\\\\.\\pipe\\vcpkg_ext_portfile_dbg",
            "preLaunchTask": "Debug vcpkg commands"
        }
    ]
}
```

## Credits
* [CharmedBayron](https://github.com/CharmedBaryon/CommonLibSSE-NG),
[Ryan McKenzie](https://github.com/Ryan-rsm-McKenzie), [Alan Tse](https://github.com/alandtse) and
[powerof3](https://github.com/powerof3) for CommonlibSSE-NG.
* [Mrowr Purr](https://github.com/SkyrimScripting) for his guide videos.
* [Igor](https://github.com/IgorAlanAlbuquerque) for his improvements of the functionality.