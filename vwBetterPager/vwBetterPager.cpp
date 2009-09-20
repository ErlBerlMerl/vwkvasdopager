#include "stdafx.h"
#include "vwBetterPager.h"

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include "Messages.h"
#include "Defines.h"
#include <time.h>
#include <iostream>
//#include "commctrl.h"

int messages = 0;

int WINW = 1; // window picture width
int WINH = 1; // window picture heigt

int COEF = 1; // scale factor

int NUMDESKX; // desktop count
int NUMDESKY; // desktop count

// Window handlers
HWND vwHandle = 0;  // virtuawin

int tipXpos = 0;
int tipYpos = 0;


HWND tipwin = 0;


#include "utils.h"
#include "tooltip.h"
#include "mainWindow.h"
#include "canvasWindow.h"


void fixRect(RECT* r)
{
  if(r->left < -20000)
  {
    r->left+=25000;
    r->right+=25000;
  }
  if(r->top < -20000)
  {
    r->top+=25000;
    r->bottom+=25000;
  }
}

void clipRect(RECT* r)
{
  if(r->left < 0)
    r->left=0;
  if(r->top < 0)
    r->top=0;
  if(r->right > WINW*COEF)
    r->right = WINW*COEF;
  if(r->bottom > WINH*COEF)
    r->bottom = WINH*COEF;
}

// stuff for rendering
RECT dcr;
HBITMAP hBmp;
PAINTSTRUCT ps;


// main rendering procedure
int redrawWindow(HWND hwnd)
{
  RECT wrc;
  HWND h, hOld;
  HDC hdc,ohdc;
  int i,j;
 
  int now = GetTickCount();
  if(now - lastredraw < 100)
    return 0;

  lastredraw = now;
  
  ohdc = BeginPaint(canvasWindowHandle, &ps);
  hdc = CreateCompatibleDC(ohdc);
  GetClientRect(canvasWindowHandle, &dcr);
  hBmp = CreateCompatibleBitmap(ohdc, dcr.right-dcr.left, dcr.bottom-dcr.top);
  SelectObject(hdc,hBmp);
  SelectObject(hdc, font);
  SetBkMode(hdc, TRANSPARENT);

  curdesk = (int)SendMessage(vwHandle, VW_CURDESK, 0, 0);

  // draw desktop backgrounds
  for(i=0;i<NUMDESKY;i++)
  {
    for(j=0;j<NUMDESKX;j++)
    {
      if((i*NUMDESKX+j)+1 == curdesk)
        SelectObject(hdc, activeDesk);
      else
        SelectObject(hdc, inactiveDesk);

      Rectangle(hdc,WINW*j,WINH*i,WINW*(j+1)+1,WINH*(i+1)+1);
    }
  }

  // find last window
  if((h = GetForegroundWindow()) == NULL)
    h = GetTopWindow(NULL);
  hOld = h;
  while(h)
  {
    hOld = h;
    h = GetNextWindow(h, GW_HWNDNEXT);
  }
  h = hOld;

  //draw windows
  while(h)
  {
    RECT r;  
    if(h != hwnd) // not render myself
    {
      if(GetWindowRect(h, &r))
      {
        char text[vwWINDOWNAME_MAX];

        // get window desktop
        int flag  = (int)SendMessage(vwHandle, VW_WINGETINFO, (WPARAM) h, NULL);

        // if window is managed
        if(flag)
        {
          // dragged window is rendered last          
          if(h != dragged)
          {
            // TODO: add hung vindow display
            // TODO: add non-managed window display

            int desk = vwWindowGetInfoDesk(flag);
            //desk = desk-1;

            // handle vw's "-25000 shift" trick
            fixRect(&r);
            
            // get window caption
            if(!GetWindowText(h,text,vwWINDOWNAME_MAX))
              text[0] = 0;

            // select brush

			if(h == GetForegroundWindow())
            //if(desk == curdesk)
			{
              SelectObject(hdc, activeWindow);
			  SelectObject(hdc, activeWframe);
			}
            else
			{
              SelectObject(hdc, inactiveWindow);          
			  SelectObject(hdc, inactiveWframe);
			}
          
            RECT tr;
            int deskx = (desk-1)%NUMDESKX;
            int desky = (desk-1)/NUMDESKX; 

			r.left /= COEF;
			r.right /= COEF;
			r.top /= COEF;
			r.bottom /= COEF;

			if(r.left<1)
				r.left = 1;
			if(r.top<1)
				r.top = 1;
			if(r.right > WINW-1)
				r.right = WINW-1;
			if(r.bottom > WINH-1)
				r.bottom = WINH-1;

            tr.left = r.left+WINW*deskx+1;
            tr.top = r.top+WINH*desky+1;
            tr.right = r.right+WINW*deskx;
            tr.bottom = r.bottom+WINH*desky;

            Rectangle(hdc, r.left+WINW*deskx, r.top+WINH*desky, r.right+WINW*deskx+1, r.bottom+WINH*desky);
			if((r.right-r.left > 16) && (r.bottom - r.top) > 16)
			{

				DrawIconEx(
					hdc, 
					(r.left + (r.right - r.left)/2)+WINW*deskx - 7, 
					(r.top + (r.bottom - r.top)/2)+WINH*desky - 7, 
					(HICON)GetClassLong(h,GCL_HICON), 
					16, 16, 
					0, 0, 
					DI_NORMAL
				);
			}
            //DrawText(hdc, text, (int)strlen(text), &tr, DT_LEFT	| DT_TOP | DT_SINGLELINE);          
          }
        }
      }
    }
    h = GetNextWindow(h, GW_HWNDPREV);
  }

  if(dragged)
  {
    RECT r;
    RECT tr;
    char text[vwWINDOWNAME_MAX] ;

    GetWindowRect(dragged, &r);
    
    fixRect(&r);

    if(!GetWindowText(dragged,text,vwWINDOWNAME_MAX))
      text[0] = 0;
    
    SelectObject(hdc, activeWindow);

    int deskx = (overdesk-1)%NUMDESKX;
    int desky = (overdesk-1)/NUMDESKX; 

	r.left /= COEF;
	r.right /= COEF;
	r.top /= COEF;
	r.bottom /= COEF;

	if(r.left<1)
		r.left = 1;
	if(r.top<1)
		r.top = 1;
	if(r.right > WINW-1)
		r.right = WINW-1;
	if(r.bottom > WINH-1)
		r.bottom = WINH-1;

    tr.left = r.left+WINW*deskx+1;
    tr.top = r.top+WINH*desky+1;
    tr.right = r.right+WINW*deskx;
    tr.bottom = r.bottom+WINH*desky;

    Rectangle(hdc, r.left+WINW*deskx, r.top+WINH*desky, r.right+WINW*deskx+1, r.bottom+WINH*desky);
    if((r.right-r.left > 16) && (r.bottom - r.top) > 16)
			{

				DrawIconEx(
					hdc, 
					(r.left + (r.right - r.left)/2)+WINW*deskx - 7, 
					(r.top + (r.bottom - r.top)/2)+WINH*desky - 7, 
					(HICON)GetClassLong(dragged,GCL_HICON), 
					16, 16, 
					0, 0, 
					DI_NORMAL
				);
			}
	//DrawText(hdc, text, (int)strlen(text), &tr, DT_LEFT	| DT_TOP | DT_SINGLELINE);
  }

  GetClientRect(hwnd, &wrc);

  SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
  SelectObject(hdc, pagerFrame);
    // draw desktop backgrounds
  for(i=0;i<NUMDESKY;i++)
  {
    for(j=0;j<NUMDESKX;j++)
    {
      Rectangle(hdc,WINW*j,WINH*i,WINW*(j+1)+1,WINH*(i+1)+1);
    }
  }  
  Rectangle(hdc, wrc.left, wrc.top, wrc.right, wrc.bottom);

  BitBlt(ohdc, 0, 0, dcr.right - dcr.left, dcr.bottom-dcr.top, hdc, 0, 0, SRCCOPY);

  DeleteDC(hdc);
  DeleteObject(hBmp);
  EndPaint(canvasWindowHandle, &ps);

  return 0L;
}




//int tipinit = 0;

/*
* Main startup function
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{  
  MSG msg;
  mainWindowCreate(hInstance);
  createBrushes();

  //while(canvasWindowCreate() == 0)
 // {
  //  Sleep(1000);
 // }
  
  //tooltipCreate(canvasWindowHandle);

  /* main messge loop */
  try
  {
    while (GetMessage(&msg, NULL, 0, 0) != 0)
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  catch(std::runtime_error e)
  {
    MessageBox(mainWindowHandle, e.what(), "runtime error", 1);
  }

  deleteBrushes();

  return (int)msg.wParam;
}
