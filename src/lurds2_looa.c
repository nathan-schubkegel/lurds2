/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include "lurds2_looa.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "lurds2_errors.h"

#define DIAGNOSTIC_LUA_ERROR(message) DIAGNOSTIC_ERROR(message);
#define DIAGNOSTIC_LUA_ERROR2(message, m2) DIAGNOSTIC_ERROR2((message), (m2));
#define DIAGNOSTIC_LUA_ERROR3(message, m2, m3) DIAGNOSTIC_ERROR3((message), (m2), (m3));
#define DIAGNOSTIC_LUA_ERROR4(message, m2, m3, m4) DIAGNOSTIC_ERROR4((message), (m2), (m3), (m4));

#define FATAL_LUA_ERROR(message) FATAL_ERROR(message);
#define FATAL_LUA_ERROR2(message, m2) FATAL_ERROR2((message), (m2));
#define FATAL_LUA_ERROR3(message, m2, m3) FATAL_ERROR3((message), (m2), (m3));
#define FATAL_LUA_ERROR4(message, m2, m3, m4) FATAL_ERROR4((message), (m2), (m3), (m4));

void* MyLuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize);
int MyLuaPanic(lua_State * luaState);
void MyLuaWarn(void *ud, const char *msg, int tocont);
void MyLoadStandardWhitelistedLibraries(lua_State * luaState);
void MyLoadAndRunLuaFile(lua_State* luaState, const wchar_t* luaFileName);

Looa Looa_Create()
{
  lua_State * luaState;
  
  luaState = 0;
  
  // Initialize a LUA state (starting point for everything related to a single LUA instance)
  luaState = lua_newstate(MyLuaAlloc, 0);
  if (luaState == 0) {
    DIAGNOSTIC_LUA_ERROR("failed to create lua state");
    goto error;
  }

  // When lua borks, display an error messagebox and quit
  lua_atpanic(luaState, MyLuaPanic);
  
  // When lua hiccups, show it in a toast notification? maybe?
  lua_setwarnf(luaState, MyLuaWarn, 0);

  MyLoadStandardWhitelistedLibraries(luaState);

  // Load the MainApp lua file
  // (this halts the application on failure)
  MyLoadAndRunLuaFile(luaState, L"looa\\lurds2.lua");

  // finally add the exported C methods
  // (this timing is clever - MainApp.lua didn't get to use the C methods when it was first loaded+executed)
  //LuaExports_PublishCMethods(luaState);
  
  return luaState;
  
error:
  if (luaState) lua_close(luaState);
}

void MyLoadStandardWhitelistedLibraries(lua_State * luaState)
{
#define MY_WHITELISTED_GLOBALS_LEN sizeof(MyWhitelistedGlobals) / sizeof(char*)
  static char * MyWhitelistedGlobals[] = 
  {
    "assert",
    "error",
    "_G",
    //"getmetatable",
    "utf8",
    "ipairs",
    "next",
    "pairs",
    "pcall",
    "rawequal",
    "rawget",
    "rawlen",
    "rawset",
    "select",
    //"setmetatable",
    "tonumber",
    "tostring",
    "type",
    "_VERSION",
    "xpcall",
    "string",
    "table",
    "math",
    //"load",
    //"dofile",
    //"loadfile",
    //"collectgarbage",
    //"print",
    "warn",
  };

  int stackTopBeforeWhitelisting;
  int stackTopAtStartOfLoop;
  int globalTableIndex;
  int keyIndex;
  int valueIndex;
  int keep;
  const char * key;
  int whitelistIndex;

#define MY_LOAD_STANDARD_LIB(name, openFunction) \
  luaL_requiref(luaState, (name), (openFunction), 1); /* 1 means "make it global" */ \
  lua_pop(luaState, 1); /*remove lib*/

  // Load standard methods for LUA libraries to call
  MY_LOAD_STANDARD_LIB("_G", luaopen_base);
  //MY_LOAD_STANDARD_LIB(LUA_LOADLIBNAME, luaopen_package);
  //MY_LOAD_STANDARD_LIB(LUA_COLIBNAME, luaopen_coroutine);
  MY_LOAD_STANDARD_LIB(LUA_TABLIBNAME, luaopen_table);
  //MY_LOAD_STANDARD_LIB(LUA_IOLIBNAME, luaopen_io);
  //MY_LOAD_STANDARD_LIB(LUA_OSLIBNAME, luaopen_os);
  MY_LOAD_STANDARD_LIB(LUA_STRLIBNAME, luaopen_string);
  MY_LOAD_STANDARD_LIB(LUA_UTF8LIBNAME, luaopen_utf8);
  MY_LOAD_STANDARD_LIB(LUA_MATHLIBNAME, luaopen_math);
  //MY_LOAD_STANDARD_LIB(LUA_DBLIBNAME, luaopen_debug);

  // enumerate lua global table _G[]
  stackTopBeforeWhitelisting = lua_gettop(luaState);
  lua_pushglobaltable(luaState);
  globalTableIndex = lua_gettop(luaState);
  lua_pushnil(luaState); // nil makes lua_next() get the first item in table
  while (lua_next(luaState, -2) != 0)
  {
    stackTopAtStartOfLoop = lua_gettop(luaState);

    // key is at index -2
    // value is at index -1
    // copy them, and only look at the copies, to guarantee we don't change them
    // because lua_next() can get goofed by lua_tostring() because it changes
    // the value on the stack to a string
    lua_pushnil(luaState);
    lua_pushnil(luaState);
    lua_copy(luaState, -4, -2);
    lua_copy(luaState, -3, -1);
    keyIndex = lua_gettop(luaState) - 1;
    valueIndex = keyIndex + 1;

    // only retain whitelisted LUA libraries
    keep = 0;
    if (lua_isstring(luaState, keyIndex))
    {
      key = lua_tostring(luaState, keyIndex);
      
      for (whitelistIndex = 0; whitelistIndex < MY_WHITELISTED_GLOBALS_LEN; whitelistIndex++)
      {
        if (strcmp(key, MyWhitelistedGlobals[whitelistIndex]) == 0)
        {
          keep = 1;
          break;
        }
      }
    }

    if (!keep)
    {
      //DIAGNOSTIC_ERROR2("dropping ", key);
      
      // remove key from global table
      lua_pushnil(luaState);
      lua_copy(luaState, keyIndex, -1);
      lua_pushnil(luaState);
      lua_settable(luaState, globalTableIndex);
    }
    else
    {
      //DIAGNOSTIC_ERROR2("keeping ", key); 
    }
    
    // remove 'value', keep 'key' and use it for next iteration
    if (lua_gettop(luaState) < stackTopAtStartOfLoop)
    {
      FATAL_LUA_ERROR("popped too much from lua stack in while loop");
    }
    lua_pop(luaState, lua_gettop(luaState) - stackTopAtStartOfLoop + 1);
  }

  // remove lua initialization stuff
  if (lua_gettop(luaState) < stackTopBeforeWhitelisting)
  {
    FATAL_LUA_ERROR("popped too much from lua stack after while loop");
  }
  lua_pop(luaState, lua_gettop(luaState) - stackTopBeforeWhitelisting);
}

void MyLoadAndRunLuaFile(lua_State* luaState, const wchar_t* luaFileName)
{
  char* cLuaFileName;
  char* luaFileData;
  int luaFileDataLength;
  int loadResult;
  
  cLuaFileName = CopyWstrToCstr(luaFileName);

  // get the file data
  luaFileData = ResourceFile_Load(luaFileName, &luaFileDataLength);
  if (luaFileData == 0) FATAL_LUA_ERROR2("Failed to load LUA file: ", cLuaFileName);

  // let Lua parse/consume it
  loadResult = luaL_loadbufferx(luaState, luaFileData, luaFileDataLength, cLuaFileName, "t");
  if (LUA_OK != loadResult)
  {
    FATAL_LUA_ERROR3(cLuaFileName, " failed to load/parse/first-time-interpret LUA chunk: ", lua_tolstring(luaState, -1, 0));
  }

  // run it
  if (LUA_OK != lua_pcall(luaState, 0, 0, 0))
  {
    FATAL_LUA_ERROR3(cLuaFileName, " failed to execute LUA chunk: ", lua_tolstring(luaState, -1, 0));
  }

  free(luaFileData);
  free(cLuaFileName);
}

/*
The lua allocator function must provide a functionality similar to realloc, 
but not exactly the same. Its arguments are 
  ud, an opaque pointer passed to lua_newstate; 
  ptr, a pointer to the block being allocated/reallocated/freed; 
  osize, the original size of the block or some code about what is being allocated; 
  nsize, the new size of the block.

When ptr is not NULL, osize is the size of the block pointed by ptr, that is, 
the size given when it was allocated or reallocated.
When ptr is NULL, osize encodes the kind of object that Lua is allocating. 
osize is any of LUA_TSTRING, LUA_TTABLE, LUA_TFUNCTION, LUA_TUSERDATA, or 
LUA_TTHREAD when (and only when) Lua is creating a new object of that type. 
When osize is some other value, Lua is allocating memory for something else.

Lua assumes the following behavior from the allocator function:
  When nsize is zero, the allocator should behave like free and return NULL.
  When nsize is not zero, the allocator should behave like realloc. 
  The allocator returns NULL if and only if it cannot fulfill the request. 
  Lua assumes that the allocator never fails when osize >= nsize.

Note that Standard C ensures that free(NULL) has no effect and 
that realloc(NULL, size) is equivalent to malloc(size). 
This code assumes that realloc does not fail when shrinking a block. 
(Although Standard C does not ensure this behavior, it seems to be a 
safe assumption.)
*/
void* MyLuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
  // eliminate compiler warning
  (void)ud;
  (void)osize;

  if (nsize == 0) 
  {
    free(ptr);
    return NULL;
  }
  else
  {
    return realloc(ptr, nsize);
  }
}

int MyLuaPanic(lua_State * luaState)
{
  // When lua borks, display an error messagebox and quit
  FATAL_LUA_ERROR2("Lua panicked: ", lua_tostring(luaState, -1));
  return 0;
}

void MyLuaWarn(void *ud, const char *msg, int tocont)
{
  // TODO: whatever?
  DIAGNOSTIC_LUA_ERROR2("Lua warning: ", msg);
}