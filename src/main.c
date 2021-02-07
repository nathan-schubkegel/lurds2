#include <windows.h>
#include <Objbase.h>

#include "sound.c"
#include "wsapi.c"
#include "errors.c"

char mainWindowClassName[] = "LURDS2";
char mainWindowTitle[]   = "Lurds of the Rolm 2";
char mainWindowContent[] = "Welcome to Lurds of the Rolm 2";
RECT lastMainWindowRectBeforeFullScreen;
int mainWindowFullScreen = 0;
HWND mainWindowHandle = 0;
SoundChannel soundChannel = 0;
SoundBuffer soundBuffer = 0;

void CenterWindow(HWND hWnd);
void SetFullScreen(int yes, HWND hwnd);
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateButton(HWND hwnd_parent, int id, char * text, int width, int left, int top);

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  HRESULT r;
  
  // start COM (initially for wsapi)
  r = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
  if (!SUCCEEDED(r))
  {
    MessageBox(0, "CoInitializeEx() failed", mainWindowTitle, 0);
    return 0;
  }
  
  // start wsapi
  r = MFStartup(MF_VERSION, MFSTARTUP_LITE);
  if (!SUCCEEDED(r))
  {
    char* message;
    switch(r)
    {
      case MF_E_BAD_STARTUP_VERSION: message = "MFStartup() failed: The Version parameter requires a newer version of Media Foundation than the version that is running."; break;
      case MF_E_DISABLED_IN_SAFEMODE: message = "MFStartup() failed: The Media Foundation platform is disabled because the system was started in \"Safe Mode\" (fail-safe boot)."; break;
      case E_NOTIMPL: message = "MFStartup() failed: Media Foundation is not implemented on the system. This error can occur if the media components are not present (See KB2703761 for more info)."; break;
      default: message = "MFStartup() failed"; break;
    }
    MessageBox(0, message, mainWindowTitle, 0);
    return 0;
  }
  
  MSG msg;
  WNDCLASS wc;

  ZeroMemory(&wc, sizeof wc);
  wc.hInstance     = hInstance;
  wc.lpszClassName = mainWindowClassName;
  wc.lpfnWndProc   = (WNDPROC)MainWndProc;
  wc.style         = CS_DBLCLKS|CS_VREDRAW|CS_HREDRAW;
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
  
  CreateButton(mainWindowHandle, 1337, "Open", 50, 10, 10);
  CreateButton(mainWindowHandle, 1338, "LoadBuffer", 100, 70, 10);
  CreateButton(mainWindowHandle, 1339, "Play", 50, 180, 10);
  CreateButton(mainWindowHandle, 1340, "Stop", 50, 240, 10);
  CreateButton(mainWindowHandle, 1341, "KillBuffer", 100, 300, 10);
  CreateButton(mainWindowHandle, 1342, "Kill", 50, 410, 10);
  
  mainWindowFullScreen = 0;

  // Main message loop:
  while (GetMessage(&msg, NULL, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  
  MessageBox(0, "shutting down", 0, 0);
  MFShutdown();

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
              SoundChannel_Bind(soundChannel, soundBuffer, 1);
            }
            return 0;

          case 1339:
            SoundChannel_Play(soundChannel);
            break;

          case 1340:
            SoundChannel_Stop(soundChannel);
            break;
            
          case 1341:
            SoundBuffer_Release(soundBuffer);
            soundBuffer = 0;
            break;
            
          case 1342:
            SoundChannel_Release(soundChannel);
            soundChannel = 0;
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
      PAINTSTRUCT ps;
      HDC         hdc;
      RECT        rc;
      hdc = BeginPaint(hwnd, &ps);

      GetClientRect(hwnd, &rc);
      SetTextColor(hdc, RGB(240,240,96));
      SetBkMode(hdc, TRANSPARENT);
      DrawText(hdc, mainWindowContent, -1, &rc, DT_CENTER|DT_SINGLELINE|DT_VCENTER);

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