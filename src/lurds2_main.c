/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include <windows.h>
#include <GL/GL.h>
#include <stdio.h>

#include "lurds2_sound.c"
#include "lurds2_errors.c"
#include "lurds2_performanceCounter.c"
#include "lurds2_resourceFile.c"
#include "lurds2_looa.c"

static char mainWindowClassName[] = "LURDS2";
static char mainWindowTitle[]   = "Lurds of the Rolm 2";
static char mainWindowContent[] = "Welcome to Lurds of the Rolm 2";
static RECT lastMainWindowRectBeforeFullScreen;
static int mainWindowFullScreen = 0;
static HWND mainWindowHandle = 0;
static SoundChannel soundChannel = 0;
static SoundBuffer soundBuffer = 0;
static HDC mainWindowHdc;
static HGLRC mainWindowGlrc;

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

void CenterWindow(HWND hWnd);
void SetFullScreen(int yes, HWND hwnd);
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateButton(HWND hwnd_parent, int id, char * text, int width, int left, int top);
void PlayPeasants();
void DrawPeasantLabels(HDC hdc);
void DrawSomeGl(HWND hwnd);

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
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
  wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

  if (FALSE == RegisterClass(&wc))
  {
    MessageBox(0, "Unable to register main window class", mainWindowTitle, 0);
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
    MessageBox(0, "Unable to create main window", mainWindowTitle, 0);
    return 1;
  }
  
  mainWindowHdc = GetDC(mainWindowHandle);
  if (mainWindowHdc == 0)
  {
    MessageBox(0, "Unable to get DC to main window", mainWindowTitle, 0);
    return 1;
  }

  int  pixelFormatIndex;
  pixelFormatIndex = ChoosePixelFormat(mainWindowHdc, &pfd);
  if (pixelFormatIndex == 0)
  {
    MessageBox(0, "Unable to find pixel format for main window", mainWindowTitle, 0);
    return 1;
  }
  
  if (!SetPixelFormat(mainWindowHdc, pixelFormatIndex, &pfd))
  {
    MessageBox(0, "Unable to set pixel format of main window", mainWindowTitle, 0);
    return 1;
  }

  mainWindowGlrc = wglCreateContext(mainWindowHdc);
  if (mainWindowGlrc == 0)
  {
    MessageBox(0, "Unable to create gl context for main window", mainWindowTitle, 0);
    return 1;
  }

  if (!wglMakeCurrent(mainWindowHdc, mainWindowGlrc))
  {
    MessageBox(0, "Unable to make gl context current for main window", mainWindowTitle, 0);
    return 1;
  }
  
  //MessageBoxA(0, (char*)glGetString(GL_VERSION), "OPENGL VERSION", 0);
  //wglDeleteContext(mainWindowGlrc);
  
  CreateButton(mainWindowHandle, 1337, "Open", 50, 10, 10);
  CreateButton(mainWindowHandle, 1338, "LoadBuffer", 100, 70, 10);
  CreateButton(mainWindowHandle, 1339, "Play", 50, 180, 10);
  CreateButton(mainWindowHandle, 1340, "Stop", 50, 240, 10);
  CreateButton(mainWindowHandle, 1341, "KillBuffer", 100, 300, 10);
  CreateButton(mainWindowHandle, 1342, "Kill", 50, 410, 10);
  CreateButton(mainWindowHandle, 1343, "Peasants", 90, 10, 50);
  CreateButton(mainWindowHandle, 1344, "ResFile", 80, 110, 50);
  CreateButton(mainWindowHandle, 1345, "Looa", 50, 200, 50);
  CreateButton(mainWindowHandle, 1346, "BigError", 90, 260, 50);
  
  mainWindowFullScreen = 0;

  // Main message loop:
  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return msg.wParam;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
      if (VK_F11 == wParam) SetFullScreen(!mainWindowFullScreen, hwnd);
      break;
      
    case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED)
      {
        // if needed, lparam is button handle
        switch (LOWORD(wParam))
        {
          case 1337:
            if (soundChannel)
            {
              MessageBox(0, "no, it's already allocated", 0, 0);
              return 0;
            }
            soundChannel = SoundChannel_Open();
            if (soundChannel == 0)
            {
              MessageBox(0, "failed to open it", 0, 0);
            }
            else
            {
              MessageBox(0, "wurked", 0, 0);
            }
            return 0;
          
          case 1338:
            if (soundBuffer)
            {
              MessageBox(0, "no, it's already loaded", 0, 0);
              return 0;
            }
            soundBuffer = SoundBuffer_LoadFromFileW(L"C:\\games\\Lords of the Realm II\\Kt174_4.wav");
            if (soundBuffer == 0)
            {
              MessageBox(0, "failed to load eet from file", 0, 0);
            }
            else
            {
              MessageBox(0, "wurked", 0, 0);
            }
            return 0;

          case 1339:
            SoundChannel_Play(soundChannel, soundBuffer, 0);
            return 0;

          case 1340:
            SoundChannel_Stop(soundChannel);
            return 0;
            
          case 1341:
            SoundBuffer_Release(soundBuffer);
            soundBuffer = 0;
            return 0;
            
          case 1342:
            SoundChannel_Release(soundChannel);
            soundChannel = 0;
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

          default:
            return DefWindowProc(hwnd, message, wParam, lParam);
            break;
        }
      }
      else return DefWindowProc(hwnd, message, wParam, lParam);
      break;

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
      DrawSomeGl(hwnd);

      PAINTSTRUCT ps;
      HDC         hdc;
      RECT        rc;
      hdc = BeginPaint(hwnd, &ps);

      GetClientRect(hwnd, &rc);
      SetTextColor(hdc, RGB(240,240,96));
      SetBkMode(hdc, TRANSPARENT);
      DrawText(hdc, mainWindowContent, -1, &rc, DT_CENTER|DT_SINGLELINE|DT_VCENTER);
      
      DrawPeasantLabels(hdc);

      EndPaint(hwnd, &ps);
      break;
    }

    default:
          return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}


void SetFullScreen(int yes, HWND hwnd)
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

void CenterWindow(HWND hwnd_self)
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

void CreateButton(HWND hwnd_parent, int id, char * text, int width, int left, int top)
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

void Arrow1TimerProc(
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

void Arrow2TimerProc(
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

void HitTimerProc(
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

void UpdatePeasantLabelsProc(
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

void DrawPeasantLabels(HDC hdc)
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

void PlayPeasants()
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

void DrawSomeGl(HWND hwnd)
{
  //Initialize Projection Matrix so drawing is done in pixel coordinates
  // with top left at (0, 0) and bottom right at (width, height) just like desktop graphics
  glMatrixMode( GL_PROJECTION );
  RECT bounds;
  GetClientRect(hwnd, &bounds);
  int width, height;
  width = abs(bounds.right - bounds.left);
  height = abs(bounds.bottom - bounds.top);
  glOrtho(0, width, height, 0, -100, 100);

  //Initialize Modelview Matrix
  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  
  //Initialize clear color
  glClearColor( 0.2f, 0.3f, 0.4f, 1.0f );

    // apply the clear color 
  glClear( GL_COLOR_BUFFER_BIT );
  
      //Render quad
  glBegin( GL_QUADS );
      glColor3f(0.5f, 0.5f, 0.5f);
      glVertex2f(20, 20);
      glVertex2f(width - 20, 20);
      glVertex2f(width - 20, height - 20);
      glVertex2f(20, height - 20);
  glEnd();
  
  //Check for error
  GLenum error = glGetError();
  if( error != GL_NO_ERROR )
  {
      FATAL_ERROR( "Error drawing some OpenGL!");
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