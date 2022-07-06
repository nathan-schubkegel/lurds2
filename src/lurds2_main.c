/*
This is free and unencumbered software released into the public domain under The Unlicense.
You have complete freedom to do anything you want with the software, for any purpose.
Please refer to <http://unlicense.org/>
*/

#include <windows.h>
#include <GL/GL.h>
#include <stdio.h>

//#include "lurds2_sound.c"
#include "lurds2_errors.c"
//#include "lurds2_performanceCounter.c"
//#include "lurds2_resourceFile.c"
//#include "lurds2_looa.c"
//#include "lurds2_bmp.c"
//#include "lurds2_jsonstream.c"
//#include "lurds2_stack.c"
//#include "lurds2_stringutils.c"
//#include "lurds2_font.c"

#define VK_PAGEUP VK_PRIOR
#define VK_PAGEDOWN VK_NEXT

static char mainWindowClassName[] = "LURDS2";
static char mainWindowTitle[]   = "Lurds of the Room 2";
static RECT lastMainWindowRectBeforeFullScreen;
static int mainWindowFullScreen = 0;
static HWND mainWindowHandle = 0;
static HDC mainWindowHdc;
static HGLRC mainWindowGlrc;
static RECT mainWindowLastPaintSize;

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
static void DrawSomeGl(HWND hwnd);
static void HandleKeyDown(HWND hwnd, WPARAM wParam);

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
  switch (message)
  {
    case WM_CREATE:
    {
      CenterWindow(hwnd);
      return 0; // MSDN says: If an application processes this message, it should return zero to continue creation of the window.
    }

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    case WM_KEYDOWN:
      HandleKeyDown(hwnd, wParam);
      return 0; // MSDN says: An application should return zero if it processes this message.

    case WM_PAINT:
      DrawSomeGl(hwnd);
      
      // this "validates" the painted region so the app doesn't sit in a 100% CPU burning paint loop
      return DefWindowProc(hwnd, message, wParam, lParam);

    default:
      return DefWindowProc(hwnd, message, wParam, lParam);
  }
  return 0;
}

static void HandleKeyDown(HWND hwnd, WPARAM wParam)
{
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
  {
    hwnd_parent = GetDesktopWindow();
  }

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

  //Check for error
  GLenum error = glGetError();
  if( error != GL_NO_ERROR )
  {
    DIAGNOSTIC_ERROR("Error drawing some OpenGL!");
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
