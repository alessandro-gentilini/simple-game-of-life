/*
Simple game of Life

Warning: the code is not polished!

Author:  Alessandro Gentilini
Source:  https://agentilini@code.google.com/p/simple-game-of-life/
License: GNU General Public License v3
*/

#include "stdafx.h"
#include "life.h"

#include <vector>
#include <random>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <set>
#include <map>

#undef max
#undef min

#define MAX_LOADSTRING 100


HINSTANCE hInst;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);


int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int nCmdShow)
{
   LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
   LoadString(hInstance, IDC_LIFE, szWindowClass, MAX_LOADSTRING);
   MyRegisterClass(hInstance);

   if (!InitInstance(hInstance, nCmdShow)) {
      return FALSE;
   }

   HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LIFE));
   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0)) {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
   WNDCLASSEX wcex;

   wcex.cbSize = sizeof(WNDCLASSEX);

   wcex.style         = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc   = WndProc;
   wcex.cbClsExtra      = 0;
   wcex.cbWndExtra      = 0;
   wcex.hInstance      = hInstance;
   wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LIFE));
   wcex.hCursor      = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground   = (HBRUSH)(COLOR_WINDOW+1);
   wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_LIFE);
   wcex.lpszClassName   = szWindowClass;
   wcex.hIconSm      = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

   return RegisterClassEx(&wcex);
}

class Grid
{
public:
   Grid():r(0),c(0),w(0),h(0),cw(0),ch(0){}
   void set(RECT rct, size_t rows, size_t cols)
   {
      if ( rows==0 || cols==0 ) return;
      rect = rct;
      r = rows;
      c = cols;
      w = std::min(rct.right-rct.left,rct.bottom-rct.top);
      h = w;
      cw = w/cols;
      ch = h/rows;
      if ( ch < 3 || cw < 3 ) {
         //MessageBoxA(0,"cells are to small","game of life",MB_OK);
      }
   }
   RECT rect_at(size_t rows, size_t cols)
   {
      RECT rct = {0,0,0,0};
      if ( rows >= r || cols >= c ) return rct;
      rct.left   = cols*cw;
      rct.top    = rows*ch;
      rct.right  = rct.left + cw;
      rct.bottom = rct.top + ch;
      return rct;
   }
   size_t r,c;
private:
   RECT rect;
   LONG w,h,cw,ch;
};

class Point
{
public:
   Point(int a, int b):x(a),y(b){}
   int x,y;
};


class Animal
{
public:
   void push_back(const Point& p){a.push_back(p);}
   size_t size() const {return a.size();}
   Point operator[](size_t i)const{return a[i];}
private:
   std::vector< Point > a;
};


class Cells
{
public:
   typedef bool T; 
   static const T dead = false;
   static const T live = true;
   std::default_random_engine re;
   std::bernoulli_distribution violating;

   Cells( int rows, int cols, double p ):r(rows),c(cols),cnt_max(0)
   {
      violating.param(0.00004);
      std::bernoulli_distribution bern(p);
      d.resize( rows );
      cnt.resize( rows );
      for ( int r = 0; r < rows; r++ ) {
         d[r].resize( cols );
         std::generate_n( d[r].begin(), cols, [&](){return bern(re);} );
         cnt[r].resize( cols );
         std::generate_n( cnt[r].begin(), cols, [&](){return 0;} );
      }
   }
   
   T at(int row, int col) const
   {
      if ( out_of_range(row,col) ) throw std::exception("out of range in "__FUNCTION__);
      return d[row][col];
   }

   int at_sticky(int row, int col) const
   {
      if ( out_of_range(row,col) ) throw std::exception("out of range in "__FUNCTION__);
      return cnt[row][col];
   }

   void set(int row, int col, T value)
   {
      if ( out_of_range(row,col) ) return;
      d[row][col] = value;
      if ( value==live ) {
         cnt[row][col]++;
         cnt_max = std::max(cnt_max,cnt[row][col]);
      }
   }

   void set(const Animal& a, int row = -1, int col = -1 )
   {
      if ( row==-1 && col==-1 ) {
         for ( size_t i = 0; i < a.size(); i++ ) {
            set( a[i].x+c/2,a[i].y+r/2,live);
         }
      } else if ( row!=-1 && col!=-1 ) {
         for ( size_t i = 0; i < a.size(); i++ ) {
            set( a[i].x+col,a[i].y+row,live);
         }
      }
   }

   bool out_of_range(int row, int col) const
   {
      return row <0 || col < 0 || row >= r || col >= c; 
   }

   bool is_border(int row, int col) const
   {
      return row-1 < 0 || row + 1 >= r || col - 1 < 0 || col + 1 >= c;
   }

   T next_at(int row, int col) const
   {
      int lives = 0;
      int deads = 0;
      for ( int i = row - 1; i < row + 2; i++ ) {
         for ( int j = col - 1; j < col + 2; j++ ) {
            if ( i==row && j==col ) continue;
            if ( out_of_range(i,j) || at(i,j)==dead ) {
               deads++;
            } else if ( at(i,j)==live ) {
               lives++;               
            } else throw std::exception("unexpected cell status in "__FUNCTION__);
         }
      }

      if ( lives+deads!=8 ) throw std::exception("bug!");
      
      T ret;
      if ( at(row,col)==live ) {
         if ( lives < 2 ) {
            ret = dead;
         } else if ( lives == 2 || lives == 3 ) {
            ret = live;
         } else if ( lives > 3 ) {
            ret = dead;
         } else throw std::exception("unexpected space!"__FUNCTION__);
      } else if ( at(row,col)==dead ) {
         if ( lives == 3 ) {
            ret = live;
         } else {
            ret = dead;
         }
      } else throw std::exception("unexpected cell status in "__FUNCTION__);
      
      return ret;
   }

   std::string rle() const
   {
      return "";
   }

   bool operator<(const Cells& rhs) const
   {
      return d < rhs.d;
   }

   int percentile( int live_count )
   {
      if ( perc.empty() ) {
         perc.resize(256);
         std::vector< int > vs;
         for ( size_t r = 0; r < cnt.size(); r++ ) {
            for ( size_t c = 0; c < cnt[r].size(); c++ ) {
               if ( cnt[r][c] ) {
                  vs.push_back(cnt[r][c]);
               }
            }
         }
         std::sort(vs.begin(), vs.end());
         std::ostringstream oss;
         for ( size_t i = 0; i < 256; i++ ) {
            perc[i] = vs[size_t(0.5+double(i*vs.size())/256)];
            oss << i << " " << perc[i] << "\n";
         }
         oss << "min=" << *std::min_element(vs.begin(), vs.end()) << " max=" << *std::max_element(vs.begin(), vs.end());
         OutputDebugStringA( oss.str().c_str() );

      }
      if ( memo_perc.count(live_count) ) {
         return memo_perc[live_count];
      } else {
         int i = 0;
         while( i < 256 && live_count > perc[i] ) {
            i++;
         }
         memo_perc[live_count] = i;
         return i;
      }
   }

   int r,c,cnt_max;
private:
   std::vector< std::vector< T > > d;
   std::vector< std::vector< int > > cnt;
   std::vector< int > perc;
   std::map< int, int > memo_perc;
};


std::pair<ULONGLONG,ULONGLONG> ptime()
{
   FILETIME c, e, k, u;
   GetProcessTimes( GetCurrentProcess(), &c, &e, &k, &u );
   
   ULARGE_INTEGER uli;
   uli.HighPart = u.dwHighDateTime;
   uli.LowPart  = u.dwLowDateTime;

   ULARGE_INTEGER kli;
   kli.HighPart = k.dwHighDateTime;
   kli.LowPart  = k.dwLowDateTime;
   return std::make_pair(kli.QuadPart,uli.QuadPart);
}

std::pair<ULONGLONG,ULONGLONG> start_time;
std::pair<ULONGLONG,ULONGLONG> end_time(0,0);
Cells cells(170,170,0/*0.25*/);
Cells temp(170,170,0);
Grid grid;

std::set< Cells > generations;

HBRUSH black;
HBRUSH white;

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   SetTimer(hWnd,0,1,nullptr);
   black = HBRUSH(GetStockObject(BLACK_BRUSH));
   white = HBRUSH(GetStockObject(WHITE_BRUSH));

   Animal blinker;
   blinker.push_back(Point(1,0));
   blinker.push_back(Point(1,1));
   blinker.push_back(Point(1,2));

   Animal toad;
   toad.push_back(Point(1,0));
   toad.push_back(Point(1,1));
   toad.push_back(Point(1,2));
   toad.push_back(Point(2,1));
   toad.push_back(Point(2,2));
   toad.push_back(Point(2,3));

   Animal glider;
   glider.push_back(Point(0,1));
   glider.push_back(Point(1,2));
   glider.push_back(Point(2,0));
   glider.push_back(Point(2,1));
   glider.push_back(Point(2,2));


   Animal F_pentomino;
   F_pentomino.push_back(Point(0,1));
   F_pentomino.push_back(Point(0,2));
   F_pentomino.push_back(Point(1,0));
   F_pentomino.push_back(Point(1,1));
   F_pentomino.push_back(Point(2,1));

   //cells.set(F_pentomino);

   Animal die_hard;
   die_hard.push_back(Point(0,6));
   die_hard.push_back(Point(1,0));
   die_hard.push_back(Point(1,1));
   die_hard.push_back(Point(2,1));
   die_hard.push_back(Point(2,5));
   die_hard.push_back(Point(2,6));
   die_hard.push_back(Point(2,7));

   //cells.set(die_hard);

   Animal acorn;
   acorn.push_back(Point(0,2));
   acorn.push_back(Point(1,4));
   acorn.push_back(Point(2,1));
   acorn.push_back(Point(2,2));
   acorn.push_back(Point(2,5));
   acorn.push_back(Point(2,6));
   acorn.push_back(Point(2,7));

   cells.set( acorn );


   
   start_time = ptime();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}





int cnt = 0;

void draw_cnt(HDC hdc, RECT cr)
{
   std::ostringstream oss;
   oss << cnt;
   if ( end_time.first && end_time.second ) {
      ULONGLONG k = ((end_time.first -start_time.first )*100)/1000000000;
      ULONGLONG u = ((end_time.second-start_time.second)*100)/1000000000;
      oss << "-k" << k
          << "-u" << u;
      if ( k+u != 0 ) {
          oss << "-f" << cnt/(k+u);
      }
   }
   std::string& s(oss.str());
   TextOutA(hdc,cr.right-s.length()*8,cr.bottom-20,s.c_str(),s.length());
}


bool draw_sticky = false;

void OnPaint(HWND hWnd)
{
   PAINTSTRUCT ps;
   HDC hdc;
   if ( draw_sticky ) {
      RECT cr;
      GetClientRect(hWnd,&cr);
      grid.set(cr,cells.r,cells.c);
      hdc = BeginPaint(hWnd, &ps);
      for ( size_t r = 0; r < grid.r; r++ ) {
         for ( size_t c = 0; c < grid.c; c++ ) {
            RECT rc = grid.rect_at(r,c);
            int gray = 255-cells.percentile( cells.at_sticky(r,c) );
            HBRUSH brush = CreateSolidBrush( RGB(gray,gray,gray) );
            FillRect(hdc,&rc,brush);
            DeleteObject(brush);
         }
      }
      draw_cnt(hdc,cr);
      EndPaint(hWnd, &ps);
   } else {
      RECT cr;
      GetClientRect(hWnd,&cr);
      grid.set(cr,cells.r,cells.c);
      hdc = BeginPaint(hWnd, &ps);
      for ( size_t r = 0; r < grid.r; r++ ) {
         for ( size_t c = 0; c < grid.c; c++ ) {
            RECT rc = grid.rect_at(r,c);
            FillRect(hdc,&rc,cells.at(r,c)?black:white);
         }
      }
      draw_cnt(hdc,cr);
      EndPaint(hWnd, &ps);
   }
}

void OnTimer(HWND hWnd)
{
   for ( size_t r = 0; r < grid.r; r++ ) {
      for ( size_t c = 0; c < grid.c; c++ ) {
         Cells::T v(cells.next_at(r,c));
         temp.set(r,c,v);
      }
   }
   if ( generations.count(cells) ) {
      end_time = ptime();
      KillTimer(hWnd,0);
      Sleep(3000);
      draw_sticky = true;
      cells = temp;
      InvalidateRect(hWnd,nullptr,TRUE);
   } else {
      generations.insert(cells);
      cells = temp;
      cnt++;
      InvalidateRect(hWnd,nullptr,TRUE);
   }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId, wmEvent;

   switch (message)
   {
   case WM_COMMAND:
      wmId    = LOWORD(wParam);
      wmEvent = HIWORD(wParam);
      // Parse the menu selections:
      switch (wmId)
      {
      case IDM_ABOUT:
         DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
         break;
      case IDM_EXIT:
         DestroyWindow(hWnd);
         break;
      default:
         return DefWindowProc(hWnd, message, wParam, lParam);
      }
      break;
   case WM_PAINT:
      OnPaint( hWnd );
      break;
   case WM_TIMER:
      OnTimer( hWnd );
      break;
   case WM_DESTROY:
      PostQuitMessage(0);
      break;
   default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   UNREFERENCED_PARAMETER(lParam);
   switch (message)
   {
   case WM_INITDIALOG:
      return (INT_PTR)TRUE;

   case WM_COMMAND:
      if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
      {
         EndDialog(hDlg, LOWORD(wParam));
         return (INT_PTR)TRUE;
      }
      break;
   }
   return (INT_PTR)FALSE;
}
