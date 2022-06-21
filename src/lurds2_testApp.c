/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include <windows.h>
#include <CommCtrl.h>
#include <GL/GL.h>
#include <stdio.h>

#include "lurds2_sound.c"
#include "lurds2_errors.c"
#include "lurds2_performanceCounter.c"
#include "lurds2_resourceFile.c"
#include "lurds2_looa.c"
#include "lurds2_bmp.c"
#include "lurds2_jsonstream.c"
#include "lurds2_stack.c"
#include "lurds2_stringutils.c"
#include "lurds2_font.c"
#include "lurds2_plate.c"

#define VK_PAGEUP VK_PRIOR
#define VK_PAGEDOWN VK_NEXT

static char mainWindowClassName[] = "LURDS2_TEST";
static char mainWindowTitle[]   = "Lurds of the Room 2 Test App";
static char mainWindowContent[] = "Welcome to Lurds of the Room 2 Test App";
static RECT lastMainWindowRectBeforeFullScreen;
static int mainWindowFullScreen = 0;
static HWND mainWindowHandle = 0;
static HWND palettePickerHandle = 0;
static HWND platePickerHandle = 0;
static HWND brightnessSliderHandle = 0;
static SoundChannel mainWindowSoundChannel = 0;
static SoundBuffer mainWindowSoundBuffer = 0;
static HDC mainWindowHdc;
static HGLRC mainWindowGlrc;
static Bmp mainWindowBitmap;
static Font oldTimeyFont;
static Bmp* plateTestBitmapsOne;
static int mainWindowPaintCount;
static RECT mainWindowLastPaintSize;
static int mainWindowBitmapSlice_which;
static int mainWindowBitmapSlice_xOrigin = 64;  // 46 64 14 35
static int mainWindowBitmapSlice_width = 14;
static int mainWindowBitmapSlice_yOrigin = 71;
static int mainWindowBitmapSlice_yAboveOriginHeight = 25;
static int mainWindowBitmapSlice_yBelowOriginHeight = 10;
static int mainWindowMemoryLeakDetectionEnabled;

// shamelessly stolen from https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
static PIXELFORMATDESCRIPTOR pfd =
{
  sizeof(PIXELFORMATDESCRIPTOR),
  1,
  PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
  PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
  32,                   // Colordepth of the framebuffer.
  0, 0, 0, 0, 0, 0,
  0,
  0,
  0,
  0, 0, 0, 0,
  24,                   // Number of bits for the depthbuffer
  8,                    // Number of bits for the stencilbuffer
  0,                    // Number of Aux buffers in the framebuffer.
  PFD_MAIN_PLANE,
  0,
  0, 0, 0
};

static void CenterWindow(HWND hWnd);
static void SetFullScreen(int yes, HWND hwnd);
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void CreateButton(HWND hwnd_parent, int id, char * text, int width, int left, int top);
static void PlayPeasants();
static void DrawPeasantLabels(HDC hdc);
static void DrawSomeGl(HWND hwnd);
static void HandleGlyphFinderKey(HWND hwnd, int key);
static void DrawGlyphFinderStats(HDC hdc);
static void MemoryLeakTimerProc(HWND hwnd, UINT message, UINT_PTR id, DWORD msSinceSystemStart);

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  MSG msg;
  WNDCLASS wc;

  ZeroMemory(&wc, sizeof wc);
  wc.hInstance     = hInstance;
  wc.lpszClassName = mainWindowClassName;
  wc.lpfnWndProc   = (WNDPROC)MainWndProc;
  wc.style         = CS_DBLCLKS
                      | CS_VREDRAW | CS_HREDRAW // makes window redraw if client area is resized
                      | CS_OWNDC; // makes DeviceContext of the window never released (for opengl)
  wc.hbrBackground = 0; // disables WM_ERASEBKGND, or at least the effects of it
                        // could use this to have a background again: (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

  if (FALSE == RegisterClass(&wc))
  {
    DIAGNOSTIC_ERROR("Unable to register main window class");
    return 1;
  }

  mainWindowHandle = CreateWindow(
    mainWindowClassName,
    mainWindowTitle,
    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    500,//CW_USEDEFAULT,
    240,//CW_USEDEFAULT,
    0,
    0,
    hInstance,
    0);
    
  if (NULL == mainWindowHandle)
  {
    DIAGNOSTIC_ERROR("Unable to create main window");
    return 1;
  }
  
  mainWindowHdc = GetDC(mainWindowHandle);
  if (mainWindowHdc == 0)
  {
    DIAGNOSTIC_ERROR("Unable to get DC to main window");
    return 1;
  }

  int  pixelFormatIndex;
  pixelFormatIndex = ChoosePixelFormat(mainWindowHdc, &pfd);
  if (pixelFormatIndex == 0)
  {
    DIAGNOSTIC_ERROR("Unable to find pixel format for main window");
    return 1;
  }
  
  if (!SetPixelFormat(mainWindowHdc, pixelFormatIndex, &pfd))
  {
    DIAGNOSTIC_ERROR("Unable to set pixel format of main window");
    return 1;
  }

  mainWindowGlrc = wglCreateContext(mainWindowHdc);
  if (mainWindowGlrc == 0)
  {
    DIAGNOSTIC_ERROR("Unable to create gl context for main window");
    return 1;
  }

  if (!wglMakeCurrent(mainWindowHdc, mainWindowGlrc))
  {
    DIAGNOSTIC_ERROR("Unable to make gl context current for main window");
    return 1;
  }
  
  //MessageBoxA(0, (char*)glGetString(GL_VERSION), "OPENGL VERSION", 0);
  //wglDeleteContext(mainWindowGlrc);
  
  CreateButton(mainWindowHandle, 1337, "Open", 40, 10, 5);
  CreateButton(mainWindowHandle, 1338, "LoadBuffer", 80, 50, 5);
  CreateButton(mainWindowHandle, 1339, "Play", 40, 130, 5);
  CreateButton(mainWindowHandle, 1340, "Stop", 40, 170, 5);
  CreateButton(mainWindowHandle, 1341, "KillBuffer", 80, 210, 5);
  CreateButton(mainWindowHandle, 1342, "Kill", 40, 290, 5);
  CreateButton(mainWindowHandle, 1343, "Peasants", 70, 330, 5);
  CreateButton(mainWindowHandle, 1344, "ResFile", 60, 400, 5);
  CreateButton(mainWindowHandle, 1345, "Looa", 40, 10, 35);
  CreateButton(mainWindowHandle, 1346, "BigError", 60, 50, 35);
  CreateButton(mainWindowHandle, 1347, "Bmp", 40, 110, 35);
  CreateButton(mainWindowHandle, 1348, "Invalidate", 70, 150, 35);
  CreateButton(mainWindowHandle, 1349, "MemLeak", 80, 220, 35);
  CreateButton(mainWindowHandle, 1350, "Focus", 50, 300, 35);
  CreateButton(mainWindowHandle, 1351, "StackTests", 85, 350, 35);
  CreateButton(mainWindowHandle, 1352, "JsonTests", 75, 10, 65);
  CreateButton(mainWindowHandle, 1353, "FontTests", 75, 85, 65);
  CreateButton(mainWindowHandle, 1354, "PlateTests-1", 100, 160, 65);

  // Create and populate the palette picker combobox
  palettePickerHandle = CreateWindow(WC_COMBOBOX, TEXT(""), 
     CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
     260, 65, 200, 200, mainWindowHandle, NULL, 0, NULL);
  SendMessageA(palettePickerHandle, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)"(default)");
  SendMessageA(palettePickerHandle, (UINT)CB_SETITEMDATA, (WPARAM)0, (LPARAM)PaletteFileId_NONE);
  for (PaletteFileId id = 0; id < PaletteFileId_END; id++)
  {
    int index = SendMessageA(palettePickerHandle, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)PaletteFile_GetName(id));
    SendMessageA(palettePickerHandle, (UINT)CB_SETITEMDATA, (WPARAM)index, (LPARAM)id);
  }
  SendMessage(palettePickerHandle, CB_SETCURSEL, (WPARAM)0, (LPARAM)0); // select an initial item
  
  // Create and populate the plate picker combobox
  platePickerHandle = CreateWindow(WC_COMBOBOX, TEXT(""),
     CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
     260, 95, 200, 200, mainWindowHandle, NULL, 0, NULL);
  for (PlateFileId id = 0; id < PlateFileId_END; id++)
  {
    if (PlateFile_IsSupported(id))
    {
      int index = SendMessageA(platePickerHandle, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)PlateFile_GetName(id));
      SendMessageA(platePickerHandle, (UINT)CB_SETITEMDATA, (WPARAM)index, (LPARAM)id);
      
      if (id == PlateFileId_VILLAGE)
      {
        SendMessage(palettePickerHandle, CB_SETCURSEL, (WPARAM)index, (LPARAM)0); // select an initial item
      }
    }
  }

  // create a brightness slider
  brightnessSliderHandle = CreateWindowEx(0, TRACKBAR_CLASS, "Trackbar Control", 
    WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_ENABLESELRANGE,
    10, 95, 200, 35, mainWindowHandle, 0, 0, NULL);
  //SendMessage(brightnessSliderHandle, TBM_SETRANGE, (WPARAM) TRUE /*redraw*/, (LPARAM) MAKELONG(0, 100));
  //SendMessage(brightnessSliderHandle, TBM_SETPAGESIZE, 0, (LPARAM) 1);
  //SendMessage(brightnessSliderHandle, TBM_SETPOS, (WPARAM) TRUE /*redraw*/, (LPARAM) 0);

  mainWindowFullScreen = 0;

  // Main message loop:
  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message) {

    case WM_CREATE:
      CenterWindow(hwnd);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    case WM_KEYDOWN:
      if (VK_ESCAPE == wParam)
      {
        if (mainWindowFullScreen)
        {
          SetFullScreen(0, hwnd);
        }
        else
        {
          DestroyWindow(hwnd);
        }
      }
      else if (VK_F11 == wParam)
      {
        SetFullScreen(!mainWindowFullScreen, hwnd);
      }
      else if (VK_UP == wParam || VK_DOWN == wParam || VK_LEFT == wParam || VK_RIGHT == wParam || VK_PAGEUP == wParam || VK_PAGEDOWN == wParam)
      {
        HandleGlyphFinderKey(hwnd, (int)wParam);
      }
      break;
      
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
        // if needed, lparam is button handle
        switch (LOWORD(wParam))
        {
          case 1337:
            if (mainWindowSoundChannel)
            {
              MessageBox(0, "no, it's already allocated", 0, 0);
              return 0;
            }
            mainWindowSoundChannel = SoundChannel_Open();
            if (mainWindowSoundChannel == 0)
            {
              MessageBox(0, "failed to open it", 0, 0);
            }
            else
            {
              MessageBox(0, "wurked", 0, 0);
            }
            return 0;
          
          case 1338:
            if (mainWindowSoundBuffer)
            {
              MessageBox(0, "no, it's already loaded", 0, 0);
              return 0;
            }
            mainWindowSoundBuffer = SoundBuffer_LoadFromFileW(L"C:\\games\\Lords of the Realm II\\Kt174_4.wav");
            if (mainWindowSoundBuffer == 0)
            {
              MessageBox(0, "failed to load eet from file", 0, 0);
            }
            else
            {
              MessageBox(0, "wurked", 0, 0);
            }
            return 0;

          case 1339:
            SoundChannel_Play(mainWindowSoundChannel, mainWindowSoundBuffer, 0);
            return 0;

          case 1340:
            SoundChannel_Stop(mainWindowSoundChannel);
            return 0;
            
          case 1341:
            SoundBuffer_Release(mainWindowSoundBuffer);
            mainWindowSoundBuffer = 0;
            return 0;
            
          case 1342:
            SoundChannel_Release(mainWindowSoundChannel);
            mainWindowSoundChannel = 0;
            return 0;
            
          case 1343:
            PlayPeasants();
            return 0;
            
          case 1344:
          {
            char* data;
            int fileSize;
            data = ResourceFile_Load(L"unlicense.txt", &fileSize);
            if (data != 0)
            {
              if (fileSize != 219)
              {
                MessageBox(0, "file size unexpected", 0, 0);
              }
              else if (strncmp(data, "This is free and unencumbered software released into the public domain under The Unlicense.", 91) != 0)
              {
                MessageBox(0, "compare unexpected", 0, 0);
                MessageBox(0, data, 0, 0);
              }
              else
              {
                MessageBox(0, "file good", 0, 0);
              }
            }
            free(data);
            return 0;
          }
          
          case 1345:
          {
            Looa l;
            l = Looa_Create();
            if (l)
            {
              MessageBox(0, "looa good", 0, 0);
            }
            return 0;
          }
          
          case 1346:
            DIAGNOSTIC_ERROR4("here's a big message: ",
              "abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz "
              "abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz "
              "abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz "
              "abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz ",
              "1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 "
              "1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 "
              "1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 "
              "1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 1234567890 "
              "gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey "
              "gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey "
              "gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey "
              "gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey gabba gabba hey "
              "hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho "
              "hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho "
              "hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho "
              "hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho hiney hiney ho "
              "the thug life chose me the thug life chose me the thug life chose me the thug life chose me the thug life chose me "
              "the thug life chose me the thug life chose me the thug life chose me the thug life chose me the thug life chose me "
              "the thug life chose me the thug life chose me the thug life chose me the thug life chose me the thug life chose me "
              "the thug life chose me the thug life chose me the thug life chose me the thug life chose me the thug life chose me ",
              "and still more content at the end");
              break;

          case 1347:
          {
            if (mainWindowBitmap) Bmp_Release(mainWindowBitmap);
            mainWindowBitmap = Bmp_LoadFromResourceFile(L"old_timey_font.bmp");
            Bmp_SetPixelPerfect(mainWindowBitmap, 1);
            if (mainWindowBitmap == 0) break;
            InvalidateRect(hwnd, 0, 1);
            SetFocus(hwnd);
          }
          break;
          
          case 1348:
          {
            InvalidateRect(hwnd, 0, 1);
            SetFocus(hwnd);
          }
          break;
          
          case 1349:
          {
            if (!mainWindowMemoryLeakDetectionEnabled) SetTimer(hwnd, 124, 15, (TIMERPROC)MemoryLeakTimerProc);
            else KillTimer(hwnd, 124);
            mainWindowMemoryLeakDetectionEnabled = !mainWindowMemoryLeakDetectionEnabled;
          }
          break;
          
          case 1350:
          {
            SetFocus(mainWindowHandle);
          }
          break;
          
          case 1351:
          {
            Stack s = Stack_Create(4);
            if (Stack_Count(s) != 0) DIAGNOSTIC_ERROR("stack size should be 0 here");
            for (int s2 = 0; s2 < 35; s2++)
            {
              int* s3 = Stack_Push(s);
              if (s3 == 0) DIAGNOSTIC_ERROR("push failed?");
              *s3 = s2;
              int* s4 = Stack_Peek(s);
              if (s4 != s3) DIAGNOSTIC_ERROR("peek result after push should be identical");
            }
            if (Stack_Count(s) != 35) DIAGNOSTIC_ERROR("stack size should be 35 here");
            for (int s2 = 0; s2 < 35; s2++)
            {
              int* s3 = Stack_Get(s, s2);
              if (s3 == 0) DIAGNOSTIC_ERROR("get failed?");
              if (*s3 != s2) DIAGNOSTIC_ERROR("got unexpected value");
            }
            for (int s2 = 34; s2 >= 0; s2--)
            {
              int* s5 = Stack_Peek(s);
              if (s5 == 0) DIAGNOSTIC_ERROR("peek failed?");
              else if (*s5 != s2) DIAGNOSTIC_ERROR("peek value unexpected");
              Stack_Pop(s);
              if (Stack_Count(s) != s2) DIAGNOSTIC_ERROR("stack size should be something here");
            }
            if (Stack_Count(s) != 0) DIAGNOSTIC_ERROR("stack count should finally be 0 here");
            Stack_Release(s);
            MessageBox(0, "stack tested ok i guess", 0, 0);
          }
          break;
          
          case 1352:
          {
            JsonStream s = JsonStream_Parse("blah", "test1.json");
            JsonStreamTokenType t = JsonStream_MoveNext(s);
            if (t != JsonStreamError) DIAGNOSTIC_ERROR("test1 should have failed?");
            JsonStream_Release(s);

            s = JsonStream_Parse("\"a string\"", "test2.json");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamString) DIAGNOSTIC_ERROR("test2 should have been string?");
            int32_t u;
            const char* v = JsonStream_GetString(s, &u);
            if (u != 8) DIAGNOSTIC_ERROR("expected test2 string length == 8");
            if (strcmp(v, "a string") != 0) DIAGNOSTIC_ERROR("expected test2 string == a string");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamEnd) DIAGNOSTIC_ERROR("test2 should have ended?");
            JsonStream_Release(s);
            
            MessageBox(0, "starting test3...", 0, 0);
            s = JsonStream_Parse("// yeah \n { \"num\": 33, \"name\" : \"charley! \\\"the unicorn\\\" manure-pile\\r\\nSteve.\", \"nom\": { \"whut\": null }, \"dem\": [false, true, 33, \"44\", {}] } /* ok this is the end */", "test3.json");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamObjectStart) DIAGNOSTIC_ERROR("test3 first step should have been object start?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamPropertyName) DIAGNOSTIC_ERROR("test3 second step should have been property name?");
            v = JsonStream_GetString(s, &u);
            if (u != 3) DIAGNOSTIC_ERROR("expected test3 second step property name length == 3");
            if (strcmp(v, "num") != 0) DIAGNOSTIC_ERROR("expected test3 second step property name == num");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamNumber) DIAGNOSTIC_ERROR("test3 third step should have been number?");
            int64_t w = JsonStream_GetNumberInt(s);
            if (w != 33) DIAGNOSTIC_ERROR("test3 third step should have been == 33?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamPropertyName) DIAGNOSTIC_ERROR("test3 fourth step should have been property name?");
            v = JsonStream_GetString(s, &u);
            if (u != 4) DIAGNOSTIC_ERROR("expected test3 fourth step property name length == 4");
            if (strcmp(v, "name") != 0) DIAGNOSTIC_ERROR("expected test3 fourth step property name == name");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamString) DIAGNOSTIC_ERROR("test3 fifth step should have been string?");
            v = JsonStream_GetString(s, &u);
            if (u != 42) DIAGNOSTIC_ERROR("expected test3 fifth step property name length == 42");
            if (strcmp(v, "charley! \"the unicorn\" manure-pile\r\nSteve.") != 0) DIAGNOSTIC_ERROR("expected test3 second step property name == charlie! and so on");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamPropertyName) DIAGNOSTIC_ERROR("test3 sixth step should have been property name?");
            v = JsonStream_GetString(s, &u);
            if (u != 3) DIAGNOSTIC_ERROR("expected test3 sixth step property name length == 3");
            if (strcmp(v, "nom") != 0) DIAGNOSTIC_ERROR("expected test3 sixth step property name == nom");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamObjectStart) DIAGNOSTIC_ERROR("test3 seventh step should have been object start?");
            // I'm tired of testing now. Bleh.
            t = JsonStream_MoveNext(s);
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamNull) DIAGNOSTIC_ERROR("test3 eight step should have been null?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamObjectEnd) DIAGNOSTIC_ERROR("test3 eight step should have been null?");
            t = JsonStream_MoveNext(s);
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamArrayStart) DIAGNOSTIC_ERROR("test3 ninth step should have been array start?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamFalse) DIAGNOSTIC_ERROR("test3 tenth step should have been false?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamTrue) DIAGNOSTIC_ERROR("test3 eleventh step should have been true?");
            t = JsonStream_MoveNext(s);
            t = JsonStream_MoveNext(s);
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamObjectStart) DIAGNOSTIC_ERROR("test3 twelfth step should have been object start?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamObjectEnd) DIAGNOSTIC_ERROR("test3 thirteenth step should have been object end?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamArrayEnd) DIAGNOSTIC_ERROR("test3 fourteenth step should have been array end?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamObjectEnd) DIAGNOSTIC_ERROR("test3 fifteenth step should have been object end?");
            t = JsonStream_MoveNext(s);
            if (t != JsonStreamEnd) DIAGNOSTIC_ERROR("test3 fifteenth step should have been stream end?");
            JsonStream_Release(s);

            MessageBox(0, "json tested ok i guess", 0, 0);
          }
          break;
          
          case 1353:
          {
            if (oldTimeyFont != 0) Font_Release(oldTimeyFont);
            oldTimeyFont = Font_LoadFromResourceFile(L"old_timey_font.json");
            if (oldTimeyFont == 0) { DIAGNOSTIC_ERROR("no fonts 4 u"); break; }
            InvalidateRect(hwnd, 0, 1);
          }
          break;
          
          case 1354:
          {
            if (plateTestBitmapsOne != 0) Plate_Release(plateTestBitmapsOne);
            plateTestBitmapsOne = Plate_LoadFromFile(PlateFileId_VILLAGE);
            if (plateTestBitmapsOne == 0) { DIAGNOSTIC_ERROR("no plates 4 u"); break; }
            InvalidateRect(hwnd, 0, 1);
          }
          break;

          default:
            return DefWindowProc(hwnd, message, wParam, lParam);
            break;
        }
      }
      else if (HIWORD(wParam) == CBN_SELCHANGE)
      {
        if ((HWND)lParam == palettePickerHandle || (HWND)lParam == platePickerHandle)
        {
          int plateFileId = SendMessage(platePickerHandle, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
          if (plateFileId < 0) plateFileId = PlateFileId_VILLAGE;
          else plateFileId = SendMessage(platePickerHandle, (UINT) CB_GETITEMDATA, (WPARAM)plateFileId, (LPARAM)0);

          int paletteFileId = SendMessage(palettePickerHandle, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
          if (paletteFileId < 0) paletteFileId = PaletteFileId_NONE;
          else paletteFileId = SendMessage(palettePickerHandle, (UINT) CB_GETITEMDATA, (WPARAM)paletteFileId, (LPARAM)0);

          if (plateTestBitmapsOne != 0)
          {
            Plate_Release(plateTestBitmapsOne);
            plateTestBitmapsOne = 0;
          }
          if (PlateFile_IsSupported(plateFileId))
          {
            plateTestBitmapsOne = Plate_LoadFromFileWithCustomPalette(plateFileId, paletteFileId);
            if (plateTestBitmapsOne == 0) { DIAGNOSTIC_ERROR("no plates 4 u"); break; }
          }
          InvalidateRect(hwnd, 0, 1);
        }
        else
        {
          return DefWindowProc(hwnd, message, wParam, lParam);
        }
      }
      else return DefWindowProc(hwnd, message, wParam, lParam);
      break;
      
    case WM_HSCROLL:
    {
      if (hwnd == brightnessSliderHandle)
      {
        InvalidateRect(hwnd, 0, 1);
      }
      else return DefWindowProc(hwnd, message, wParam, lParam);
      break;
    }

    case WM_SYSCOMMAND:
      if (SC_MAXIMIZE == wParam) SetFullScreen(1, hwnd);
      else if (SC_RESTORE == wParam)
      {
        SetFullScreen(0, hwnd);
        return DefWindowProc(hwnd, message, wParam, lParam);
      }
      else return DefWindowProc(hwnd, message, wParam, lParam);
      break;
      
    case WM_NCLBUTTONDBLCLK:
      // TODO: should technically inspect wParam
      SetFullScreen(1, hwnd);
      break;
      
    case WM_PAINT:
    {
      mainWindowPaintCount++;
      DrawSomeGl(hwnd);

      PAINTSTRUCT ps;
      HDC         hdc;
      RECT        rc;
      hdc = BeginPaint(hwnd, &ps);

      // draw some hello text
      GetClientRect(hwnd, &rc);
      SetTextColor(hdc, RGB(240,240,96));
      SetBkMode(hdc, TRANSPARENT);
      DrawText(hdc, mainWindowContent, -1, &rc, DT_CENTER|DT_SINGLELINE|DT_VCENTER);

      // provide an indication of how many paints have occurred      
      rc.top = 30;
      char nurp[50];
      itoa(mainWindowPaintCount, nurp, 10);
      DrawText(hdc, nurp, -1, &rc, DT_CENTER|DT_SINGLELINE|DT_VCENTER);
      
      DrawPeasantLabels(hdc);
      
      DrawGlyphFinderStats(hdc);

      EndPaint(hwnd, &ps);
      break;
    }

    default:
          return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}


static void SetFullScreen(int yes, HWND hwnd)
{  
#define WS_NOTFULLSCREEN  (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE)
#define WS_FULLSCREEN     (WS_POPUP      |              WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS               | WS_VISIBLE)

  HMONITOR monitor;
  MONITORINFO monitorInfo;
  int monitorWidth, monitorHeight;

  monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  monitorInfo.cbSize = sizeof(monitorInfo);
  if (GetMonitorInfo(monitor, &monitorInfo))
  {
    if (yes)
    {
      monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
      monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
      
      GetWindowRect(hwnd, &lastMainWindowRectBeforeFullScreen);
      mainWindowFullScreen = 1;
      SetWindowLongPtr(hwnd, GWL_STYLE, WS_FULLSCREEN);
      SetWindowPos(hwnd, HWND_TOP, 
        monitorInfo.rcMonitor.left, 
        monitorInfo.rcMonitor.top, 
        monitorWidth,
        monitorHeight,
        SWP_FRAMECHANGED);
    }
    else if (!yes)
    {
      int newWidth, newHeight, newTop, newLeft;

      monitorWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
      monitorHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;
      
      newWidth = lastMainWindowRectBeforeFullScreen.right - lastMainWindowRectBeforeFullScreen.left;
      if (newWidth > monitorWidth) newWidth = monitorWidth;
      newHeight = lastMainWindowRectBeforeFullScreen.bottom - lastMainWindowRectBeforeFullScreen.top;
      if (newHeight > monitorHeight) newHeight = monitorHeight;
      newLeft = lastMainWindowRectBeforeFullScreen.left;
      if (newLeft < monitorInfo.rcWork.left) newLeft = monitorInfo.rcWork.left;
      newTop = lastMainWindowRectBeforeFullScreen.top;
      if (newTop < monitorInfo.rcWork.top) newTop = monitorInfo.rcWork.top;
      
      mainWindowFullScreen = 0;
      SetWindowLongPtr(hwnd, GWL_STYLE, WS_NOTFULLSCREEN);
      SetWindowPos(hwnd, HWND_TOP,
        newLeft,
        newTop,
        newWidth,
        newHeight,
        SWP_FRAMECHANGED);
    }
  }
}

static void CenterWindow(HWND hwnd_self)
{
    HWND hwnd_parent;
    RECT rw_self, rc_parent, rw_parent;
    int xpos, ypos;

    hwnd_parent = GetParent(hwnd_self);
    if (NULL == hwnd_parent)
        hwnd_parent = GetDesktopWindow();

    GetWindowRect(hwnd_parent, &rw_parent);
    GetClientRect(hwnd_parent, &rc_parent);
    GetWindowRect(hwnd_self, &rw_self);

    xpos = rw_parent.left + (rc_parent.right + rw_self.left - rw_self.right) / 2;
    ypos = rw_parent.top + (rc_parent.bottom + rw_self.top - rw_self.bottom) / 2;

    SetWindowPos(
        hwnd_self, NULL,
        xpos, ypos, 0, 0,
        SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE
        );
}

static void CreateButton(HWND hwnd_parent, int id, char * text, int width, int left, int top)
{
  HWND hwndButton = CreateWindow( 
    "BUTTON",  // Predefined class; Unicode assumed 
    text,      // Button text 
    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
    left,         // x position 
    top,         // y position 
    width,        // Button width
    30,        // Button height
    hwnd_parent, // Parent window
    (HMENU)id,       // No menu, but for child controls this is the "control identifier" passed to the parent control on button click event
    (HINSTANCE)GetWindowLongPtr(hwnd_parent, GWLP_HINSTANCE), 
    NULL);      // Pointer not needed.
}

int peasantsLoaded;
SoundChannel channels[3];
SoundBuffer arrowFire;
SoundBuffer arrowHit;
SoundBuffer die[3];
char peasantFirstOpenTime[1000];
char peasantRemainingOpenTime[1000];
char peasantLoadTime[1000];
char peasantPlayTime[1000];
LONGLONG peasantPlayTicks;

static void Arrow1TimerProc(
  HWND hwnd,
  UINT message,
  UINT_PTR id,
  DWORD msSinceSystemStart
)
{
  PerformanceCounter startTime;
  startTime = PerformanceCounter_Start();
  
  SoundChannel_Play(channels[0], arrowFire, 0);

  peasantPlayTicks += PerformanceCounter_MeasureTicks(startTime);
}

static void Arrow2TimerProc(
  HWND hwnd,
  UINT message,
  UINT_PTR id,
  DWORD msSinceSystemStart
)
{
  PerformanceCounter startTime;
  startTime = PerformanceCounter_Start();
  
  SoundChannel_Play(channels[0], arrowFire, 0);

  peasantPlayTicks += PerformanceCounter_MeasureTicks(startTime);
}

int hitCount = 0;

static void HitTimerProc(
  HWND hwnd,
  UINT message,
  UINT_PTR id,
  DWORD msSinceSystemStart
)
{
  PerformanceCounter startTime;
  startTime = PerformanceCounter_Start();

  SoundChannel_Play(channels[1], arrowHit, 0);
  hitCount++;
  if (hitCount == 2)
  {
    SoundChannel_Play(channels[2], die[0], 0);
  }
  else if (hitCount == 3)
  {
    SoundChannel_Play(channels[2], die[1], 0);
  }
  else if (hitCount == 4)
  {
    SoundChannel_Play(channels[2], die[2], 0);
    hitCount = 0;
  }
  
  peasantPlayTicks += PerformanceCounter_MeasureTicks(startTime);
}

static void UpdatePeasantLabelsProc(
  HWND hwnd,
  UINT message,
  UINT_PTR id,
  DWORD msSinceSystemStart
)
{
  double seconds;
  seconds = PerformanceCounter_TicksToSeconds(peasantPlayTicks);
  sprintf(peasantPlayTime, "SoundBuffer_Play()s took %f seconds", seconds);
  InvalidateRect(mainWindowHandle, 0, 1);
}

static void DrawPeasantLabels(HDC hdc)
{
  RECT rc;
  GetClientRect(mainWindowHandle, &rc);
  SetTextColor(hdc, RGB(240,240,96));
  SetBkMode(hdc, TRANSPARENT);
  rc.top += 80;
  DrawText(hdc, peasantFirstOpenTime, -1, &rc, DT_SINGLELINE);
  rc.top += 30;
  DrawText(hdc, peasantRemainingOpenTime, -1, &rc, DT_SINGLELINE);
  rc.top += 30;
  DrawText(hdc, peasantLoadTime, -1, &rc, DT_SINGLELINE);
  rc.top += 30;
  DrawText(hdc, peasantPlayTime, -1, &rc, DT_SINGLELINE);
}

static void PlayPeasants()
{
  if (peasantsLoaded) return;
  peasantsLoaded = 1;

  PerformanceCounter startTime;
  double seconds;

  startTime = PerformanceCounter_Start();
  channels[0] = SoundChannel_Open();
  seconds = PerformanceCounter_MeasureSeconds(startTime);
  sprintf(peasantFirstOpenTime, "first SoundChannel_Open() took %f seconds", seconds);

  startTime = PerformanceCounter_Start();
  channels[1] = SoundChannel_Open();
  channels[2] = SoundChannel_Open();
  seconds = PerformanceCounter_MeasureSeconds(startTime);
  sprintf(peasantRemainingOpenTime, "other SoundChannel_Open()s took %f seconds", seconds);

  startTime = PerformanceCounter_Start();
  die[0] = SoundBuffer_LoadFromFileW(L"C:\\games\\Lords of the Realm II\\Deadguy2.wav");
  die[1] = SoundBuffer_LoadFromFileW(L"C:\\games\\Lords of the Realm II\\Deadguy3.wav");
  die[2] = SoundBuffer_LoadFromFileW(L"C:\\games\\Lords of the Realm II\\Deadguy4.wav");
  arrowFire = SoundBuffer_LoadFromFileW(L"C:\\games\\Lords of the Realm II\\Bowmen1.wav");
  arrowHit = SoundBuffer_LoadFromFileW(L"C:\\games\\Lords of the Realm II\\Bow_hit.wav");
  seconds = PerformanceCounter_MeasureSeconds(startTime);
  sprintf(peasantLoadTime, "SoundBuffer_LoadFromFileW()s took %f seconds", seconds);

  SetTimer(mainWindowHandle, 123, 500, (TIMERPROC)Arrow1TimerProc);
  SetTimer(mainWindowHandle, 124, 455, (TIMERPROC)Arrow2TimerProc);
  SetTimer(mainWindowHandle, 125, 700, (TIMERPROC)HitTimerProc);
  SetTimer(mainWindowHandle, 126, 300, (TIMERPROC)UpdatePeasantLabelsProc);
}

static void DrawSomeGl(HWND hwnd)
{
  // determine window width/height
  RECT bounds;
  GetClientRect(hwnd, &bounds);
  int width, height;
  width = abs(bounds.right - bounds.left);
  height = abs(bounds.bottom - bounds.top);

  // update viewport when window has resized
  // (so GL is aware there's more area to draw, rather than stretching stuff out)
  if (memcmp(&mainWindowLastPaintSize, &bounds, sizeof(RECT)) != 0) {
    glViewport(0, 0, width, height);
  }
  mainWindowLastPaintSize = bounds;

  // Initialize Projection Matrix so drawing is done in pixel coordinates
  // with top left at (0, 0) and bottom right at (width, height) just like desktop graphics
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho(0, width, height, 0, -100, 100);

  //Initialize Modelview Matrix
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  //Initialize clear color
  glClearColor( 0.2f, 0.3f, 0.4f, 1.0f ); // blue-ish

  // apply the clear color 
  glClear( GL_COLOR_BUFFER_BIT );
  
  //Render quad
  glBegin( GL_QUADS );
      glColor3f(0.5f, 0.5f, 0.5f); // gray
      glVertex2f(20, 20);
      glVertex2f(width - 20, 20);
      glVertex2f(width - 20, height - 20);
      glVertex2f(20, height - 20);
  glEnd();

  if (mainWindowBitmap)
  {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslated(100, 100, 0);
    Bmp_Draw(mainWindowBitmap);

    glLoadIdentity();
    glTranslated(50, 150, 0);
    glScaled(3, 3, 1);
    Bmp_DrawPortion(mainWindowBitmap, mainWindowBitmapSlice_xOrigin, mainWindowBitmapSlice_yOrigin - mainWindowBitmapSlice_yAboveOriginHeight, mainWindowBitmapSlice_width, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight);
    
    // draw a single-pixel orange box around the bmp portion
    glBegin( GL_QUADS );
        glColor3f(1.0f, 0.5f, 0.0f); // orange

        glVertex2f(-1, -1);
        glVertex2f(mainWindowBitmapSlice_width + 1, -1);
        glVertex2f(mainWindowBitmapSlice_width + 1, 0);
        glVertex2f(-1, 0);
        
        glVertex2f(-1, -1);
        glVertex2f(-1, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight + 1);
        glVertex2f(0, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight + 1);
        glVertex2f(0, -1);
        
        glVertex2f(mainWindowBitmapSlice_width, -1);
        glVertex2f(mainWindowBitmapSlice_width, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight + 1);
        glVertex2f(mainWindowBitmapSlice_width + 1, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight + 1);
        glVertex2f(mainWindowBitmapSlice_width + 1, -1);
        
        glVertex2f(-1, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight);
        glVertex2f(mainWindowBitmapSlice_width + 1, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight);
        glVertex2f(mainWindowBitmapSlice_width + 1, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight + 1);
        glVertex2f(-1, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight + 1);
    glEnd();
    
    // give an indication of which are the "upper" and "lower" portions
    glTranslated(-11, 0, 0);
    glBegin( GL_QUADS );
        glColor3f(0.0f, 1.0f, 0.0f); // green
        glVertex2f(0.0f, 0.0f);
        glVertex2f(10.0f, 0.0f);
        glVertex2f(10.0f, mainWindowBitmapSlice_yAboveOriginHeight);
        glVertex2f(0.0f, mainWindowBitmapSlice_yAboveOriginHeight);
        glColor3f(1.0f, 0.0f, 0.0f); // red
        glVertex2f(0.0f, mainWindowBitmapSlice_yAboveOriginHeight);
        glVertex2f(10.0f, mainWindowBitmapSlice_yAboveOriginHeight);
        glVertex2f(10.0f, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight);
        glVertex2f(0.0f, mainWindowBitmapSlice_yAboveOriginHeight + mainWindowBitmapSlice_yBelowOriginHeight);
    glEnd();
    glPopMatrix();
  }
  
  if (oldTimeyFont)
  {
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glTranslated(10, 130, 0);
    
    glScaled(2, 2, 1);
    glColor4f(0.0f, 0.0f, 1.0f, 0.7f); // transparent blue
    FontMeasurement m = Font_RenderSingleLine(oldTimeyFont, "The quick brown fox trips over the zarking lazy dog. Ha!");
    
    // give an indication of which are the "upper" and "lower" portions
    glTranslated(-10, 0, 0);
    glBegin( GL_QUADS );
        glColor3f(0.0f, 1.0f, 0.0f); // green
        glVertex2f(0.0f, 0.0f);
        glVertex2f(10.0f, 0.0f);
        glVertex2f(10.0f, m.universalLineHeight);
        glVertex2f(0.0f, m.universalLineHeight);
        glColor3f(1.0f, 0.0f, 0.0f); // red
        glVertex2f(0.0f, m.universalLineHeight);
        glVertex2f(10.0f, m.universalLineHeight);
        glVertex2f(10.0f, m.universalLineHeight + 1);//m.heightDown);
        glVertex2f(0.0f, m.universalLineHeight + 1);//m.heightDown);
    glEnd();
    
    glPopMatrix();
  }
  
  if (plateTestBitmapsOne)
  {
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
      
    int wNext = 10;
    int hNext = 170;
    int tallestInThisRow = 0;
    for (Bmp* it = plateTestBitmapsOne; *it != 0; it++)
    {
      if (wNext + Bmp_GetWidth(*it) * 2 > windowWidth)
      {
        hNext += tallestInThisRow;
        tallestInThisRow = 0;
        wNext = 10;
      }

      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      glLoadIdentity();
      glTranslated(wNext, hNext, 0);
      
      glScaled(2, 2, 1);
      Bmp_Draw(*it);
      
      float brightness = SendMessage(brightnessSliderHandle, TBM_GETPOS, 0, 0) / 100.0f;
      // draw a quad over the bitmap to brighten its color; suggested at https://stackoverflow.com/a/6554145/2221472
      glEnable(GL_BLEND);
      glBlendFunc(GL_DST_COLOR, GL_ONE);
      glColor3f(brightness, brightness, brightness);
      glBegin( GL_QUADS );
        glVertex2f(0.0f, 0.0f);
        glVertex2f(Bmp_GetWidth(*it), 0.0f);
        glVertex2f(Bmp_GetWidth(*it), Bmp_GetHeight(*it));
        glVertex2f(0.0f, Bmp_GetHeight(*it));
      glEnd();
      glDisable(GL_BLEND);

      glPopMatrix();
      
      wNext += Bmp_GetWidth(*it) * 2;
      if (Bmp_GetHeight(*it) * 2 > tallestInThisRow)
      {
        tallestInThisRow = Bmp_GetHeight(*it) * 2;
      }
    }
  }
  
  //Check for error
  GLenum error = glGetError();
  if( error != GL_NO_ERROR )
  {
      DIAGNOSTIC_ERROR("Error drawing some OpenGL!");
      return;
  }

  /* SwapBuffers causes an implicit OpenGL command queue flush,
   * but only on double buffered windows. On single buffered
   * windows SwapBuffers has no effect at all. Also calling
   * glFlush right before SwapBuffers has no ill effect other
   * than that the OpenGL queue gets flushed a few CPU cycles
   * earlier. */
  glFlush();
  SwapBuffers(mainWindowHdc);
}

static void MemoryLeakTimerProc(
  HWND hwnd,
  UINT message,
  UINT_PTR id,
  DWORD msSinceSystemStart
)
{
  InvalidateRect(hwnd, 0, 1);
}

static void HandleGlyphFinderKey(HWND hwnd, int key)
{
  if (key == VK_UP)
  {
    switch (mainWindowBitmapSlice_which)
    {
      case 0: // origin
        mainWindowBitmapSlice_yOrigin--;
        break;
      case 1: // upper size
        mainWindowBitmapSlice_yAboveOriginHeight++;
        break;
      case 2: // lower size
        mainWindowBitmapSlice_yBelowOriginHeight--;
        break;
    }
  }
  else if (key == VK_DOWN)
  {
    switch (mainWindowBitmapSlice_which)
    {
      case 0: // origin
        mainWindowBitmapSlice_yOrigin++;
        break;
      case 1: // upper size
        mainWindowBitmapSlice_yAboveOriginHeight--;
        break;
      case 2: // lower size
        mainWindowBitmapSlice_yBelowOriginHeight++;
        break;
    }
  }
  else if (key == VK_LEFT)
  {
    switch (mainWindowBitmapSlice_which)
    {
      case 0: // origin
        mainWindowBitmapSlice_xOrigin--;
        break;
      case 1: // upper size
        mainWindowBitmapSlice_width--;
        break;
      case 2: // lower size
        mainWindowBitmapSlice_width--;
        break;
    }
  }
  else if (key == VK_RIGHT)
  {
    switch (mainWindowBitmapSlice_which)
    {
      case 0: // origin
        mainWindowBitmapSlice_xOrigin++;
        break;
      case 1: // upper size
        mainWindowBitmapSlice_width++;
        break;
      case 2: // lower size
        mainWindowBitmapSlice_width++;
        break;
    }
  }
  else if (key == VK_PAGEUP)
  {
    mainWindowBitmapSlice_which++;
    if (mainWindowBitmapSlice_which > 2) mainWindowBitmapSlice_which = 0;
  }
  else if (key == VK_PAGEDOWN)
  {
    mainWindowBitmapSlice_which--;
    if (mainWindowBitmapSlice_which < 0) mainWindowBitmapSlice_which = 2;
  }

  if (mainWindowBitmapSlice_width < 0) mainWindowBitmapSlice_width = 0;
  if (mainWindowBitmapSlice_yAboveOriginHeight < 0) mainWindowBitmapSlice_yAboveOriginHeight = 0;
  if (mainWindowBitmapSlice_yBelowOriginHeight < 0) mainWindowBitmapSlice_yBelowOriginHeight = 0;

  InvalidateRect(hwnd, 0, 1);
}

static void DrawGlyphFinderStats(HDC hdc)
{
  if (mainWindowBitmap)
  {
    RECT rc;
    GetClientRect(mainWindowHandle, &rc);
    SetTextColor(hdc, RGB(0,255,255));
    SetBkMode(hdc, TRANSPARENT);
    rc.left += 500;

    char buffer[1000];
    char nurp[50];    
    
    strcpy(buffer, "xOrigin = ");
    itoa(mainWindowBitmapSlice_xOrigin, nurp, 10);
    strcat(buffer, nurp);
    DrawText(hdc, buffer, -1, &rc, DT_SINGLELINE);

    rc.top += 15;
    strcpy(buffer, "yOrigin = ");
    itoa(mainWindowBitmapSlice_yOrigin, nurp, 10);
    strcat(buffer, nurp);
    DrawText(hdc, buffer, -1, &rc, DT_SINGLELINE);
    
    rc.top += 15;
    strcpy(buffer, "width = ");
    itoa(mainWindowBitmapSlice_width, nurp, 10);
    strcat(buffer, nurp);
    DrawText(hdc, buffer, -1, &rc, DT_SINGLELINE);

    rc.top += 15;
    strcpy(buffer, "yAboveOriginHeight = ");
    itoa(mainWindowBitmapSlice_yAboveOriginHeight, nurp, 10);
    strcat(buffer, nurp);
    DrawText(hdc, buffer, -1, &rc, DT_SINGLELINE);
    
    rc.top += 15;
    strcpy(buffer, "yBelowOriginHeight = ");
    itoa(mainWindowBitmapSlice_yBelowOriginHeight, nurp, 10);
    strcat(buffer, nurp);
    DrawText(hdc, buffer, -1, &rc, DT_SINGLELINE);
    
    rc.top += 15;
    strcpy(buffer, "which = ");
    switch (mainWindowBitmapSlice_which)
    {
      case 0: strcat(buffer, "origin"); break;
      case 1: strcat(buffer, "upper size"); break;
      case 2: strcat(buffer, "lower size"); break;
    }
    DrawText(hdc, buffer, -1, &rc, DT_SINGLELINE);
  }
}