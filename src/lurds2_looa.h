/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#ifndef LURDS2_LOOA
#define LURDS2_LOOA

typedef void* Looa;

// A Looa is a combination of a Lua_State and C hosting data structures for interacting with it
Looa Looa_Create(/* TODO: stuff to add:
   path of lua script to load
   screen drawing hook
   mouse/keyboard input hook
   sound hook
   persistence (save/load game) hook
   network session client/server hooks
   game engine hook
   */);
void Looa_Release(Looa looa);
//void Looa_ProcessMouseInput();
//void Looa_ProcessKeyboardInput();
//void Looa_GameEngineTick();
//void Looa_Render();

#endif