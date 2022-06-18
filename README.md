Lurds of the Room 2
=================

About
---
This is a project to clone Lords of the Realm 2, a 1995 game produced by Sierra and enjoyed by millions.

Prime Goals:
* Multiplayer network support. (It's only fun to play if you can play with your friends!)
* ANSI C. (It's only fun to develop if you must write it from the ground up!)
* Also Lua. (C is for systems work - I really want something higher level for business logic)
* Leverage original resources from Lords of the Realm 2. (It's only Lurds of the Room 2 if it looks like Lords of the Realm 2!)

Development
---
The project is built with TCC (Tiny C Compiler).
* Build: Invoke `build.bat` in the root directory.
* Run: Invoke `build.bat -run` in the root directory.
* Test: Invoke `build.bat -test` in the root directory. Currently the test app is a GUI application with exploratory/learning/example code demonstrating the various game engine features. Interpreting the results is human/manual. Sorry :)

Release
---
Invoke `build.bat -publish` in the root directory. The `publish` folder becomes populated with all files needed to distribute and play Lurds of the Room 2.

Licensing
---
The contents of this repo are free and unencumbered software released into the public domain under The Unlicense. You have complete freedom to do anything you want with the software, for any purpose. Please refer to <http://unlicense.org/> .
