Lurds of the Room 2
=================

About
---
This started as a project to clone Lords of the Realm 2, a 1995 game produced by Sierra and enjoyed by millions.
Now (upon realizing that the license of Lords of the Realm 2 prohibits creation of derivative works) it has become
a little more open to whatever sounds fun. Current goal: 3D multiplayer pogostick jumping competition! Likely to change again.

Prime Goals:
* Multiplayer network support. (It's only fun to play if you can play with your friends!)
* ANSI C. (It's only fun to develop if you must write it from the ground up!)
* Also Lua. (C is for systems work - I really want something higher level for business logic)
* Leverage original resources from Lords of the Realm 2. (It's only Lurds of the Room 2 if it looks like Lords of the Realm 2!)

Technology Choices:
* TCC (Tiny C Compiler) by Fabrice Bellard - see https://bellard.org/tcc/ (so that anybody can build from source!)
* GLFW (Graphics Library Framework) - see https://www.glfw.org/ (I originally wrote all my own Win32 code, but then needed something else to develop from my Linux laptop when I abandoned Windows)

Development
---
On Windows:
* Setup: Have Git LFS installed, so the 'deps' folder loads correctly
* Build: Invoke `build.bat` in the root directory.
* Run: Invoke `build.bat -run` in the root directory.
* Test: Invoke `build.bat -test` in the root directory. Currently the test app is a GUI application with exploratory/learning/example code demonstrating the various game engine features. Interpreting the results is human/manual. Sorry :)

On Linux:
* `sudo apt install tcc make cmake libwayland-dev libxkbcommon-dev xorg-dev`
* Powershell - see https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell-on-linux?view=powershell-7.4
* Build/Run/Test: same as for Windows, but via `pwsh ./build.ps1`

Release
---
Invoke `build.bat -publish` in the root directory. The `publish` folder becomes populated with all files needed to distribute and play Lurds of the Room 2.

Project Progress
---
| Mouse Input Handling | Windows Support | Linux Support | Lua Support |
|:-------------------|:-----------------:|:---------------:|:-------------:|
| Query current mouse position | | | |
| Query current mouse button state | | | |
| Events for mouse movement | | | |
| Events for mouse button state change | | | |
| Query app focused / mouse captured | | | |
| Events for focus gained/lost and mouse captured/lost | | | |

| Keyboard Input Handling | Windows Support | Linux Support | Lua Support |
|:-------------------|:-----------------:|:---------------:|:-------------:|
| Query current key up/down state | | | |
| Events for key down/up | | | |
| Query key capture | | | |
| Events for key capture gained/lost | | | |

| Gamepad Input Handling | Windows Support | Linux Support | Lua Support |
|:-------------------|:-----------------:|:---------------:|:-------------:|
| Enumerate gamepads | | | |
| Persistent gamepad ID (survives unplug) | | | |
| Query current button and axis states | | | |
| Events for button and axis changes | | | |
| Events for gamepad plug/unplug | | | |

| Network Play | Windows Support | Linux Support | Lua Support |
|:-------------------|:-----------------:|:---------------:|:-------------:|
| Open and close udp socket | | | |
| Explicit port and dynamic port | | | |
| Send datagram | | | |
| Receive datagram | | | |

| OpenGL Drawing | Windows Support | Linux Support | Lua Support |
|:-------------------|:-----------------:|:---------------:|:-------------:|
| Get screen size | ✅ | | |
| Clear whole screen to a color | ✅ | | |
| Load named font file | ✅ | | |
| GL modelview translation/rotation/matrix modification | ✅ | | |
| Measure / draw text (2d) | ✅ | | |
| Load a named bitmap file | ✅ | | |
| Draw bitmap file (2d) | ✅ | | |
| Load a named mesh | | | |
| Load a named bitmap u/v coordinates file (probably json) | | | |
| Render the mesh w/ a bitmap and u/v coordinates  | | | |
| Shade any drawn thing with a color | | | |

| Sound and Music | Windows Support | Linux Support | Lua Support |
|:-------------------|:-----------------:|:---------------:|:-------------:|
| Load named sounds | ✅ | | |
| Play one time | ✅ | | |
| Play looping and stop | ✅ | | |
| Stop all | ✅ | | |
| Support mp3s | | | |
| Maybe not take up tons of ram | | | |

Licensing
---
The contents of this repo are free and unencumbered software released into the public domain under The Unlicense. You have complete freedom to do anything you want with the software, for any purpose. Please refer to <http://unlicense.org/> .
