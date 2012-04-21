/*
Simple game of Life

Warning: the code is not polished!

Author : Alessandro Gentilini
Source : https://agentilini@code.google.com/p/simple-game-of-life/
License: GNU General Public License v3
*/

#include "stdafx.h"
#include "life.h"

#include <vector>
#include <random>
#include <algorithm>
#include <sstream>
#include <deque>
#include <map>
#include <numeric>
#include <array>
#include <fstream>
#include <iostream>

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
void play();

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
   
   Cells( int rows=170, int cols=170, unsigned long seed=std::default_random_engine::default_seed, double p=0 ):r(rows),c(cols),rle_end_line('$'),rle_end_pattern('!')
   {
      re.seed(seed);
      std::bernoulli_distribution bern(p);
      for ( int r = 0; r < rows; r++ ) {
         std::generate_n( std::begin(d[r]), cols, [&](){return bern(re);} );
         std::generate_n( std::begin(cnt[r]), cols, [&](){return 0;} );
      }
   }

   void reset()
   {
      for ( int i = 0; i < r; i++ ) {
         std::generate_n( std::begin(d[i]), c, [&](){return false;} );
         std::generate_n( std::begin(cnt[i]), c, [&](){return 0;} );
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

   char rle_char(const T& t) const
   {
      switch(t){
      case live: return 'o';
      case dead: return 'b';
      }
      return 0;
   }

   // http://conwaylife.com/wiki/Rle
   std::string rle() const
   {
      std::ostringstream oss;
      oss << "x = " << c << ", y = " << r << "\n";

      for ( int i = 0; i < r; i++ ) {
         T cur = d[i][0];
         size_t len = 0;
         for ( int j = 0; j < c; j++ ) {
            if ( d[i][j]==cur ) {
               len++;
               if ( j==c-1 ) {
                  oss << len << rle_char(cur);
               }
            } else {
               oss << len << rle_char(cur);
               cur = d[i][j];
               len = 1;
            }
         }
         oss << rle_end_line;
      }
      oss << rle_end_pattern << "\n";
      return oss.str();
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
         }
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

   bool operator==(const Cells& rhs ) const
   {
      return d == rhs.d;
   }

   int r,c;
private:
   static const size_t sz = 170;
   std::array< std::array< T, sz >, sz > d;
   std::array< std::array< int, sz >, sz > cnt;
   std::vector< int > perc;
   std::map< int, int > memo_perc;

   char rle_end_line, rle_end_pattern;
};

class Generations
{
public:
   Generations (size_t sz=10):size(sz){}
   
   size_t count( const Cells& c ) const 
   {
      return std::find( q.begin(), q.end(), c )!=q.end()?1:0;
   }

   void insert( const Cells& c )
   {
      if ( q.size() < size ) {
         q.push_back( c );
      } else {
         q.pop_front();
         q.push_back( c );
      }
   }
private:
   std::deque< Cells > q;
   const size_t size;
};


// globals!
Cells cells;
Cells temp;
Grid grid;
Generations generations;

bool draw_sticky = false;
bool log_status = false;
int cnt = 0;
HBRUSH black;
HBRUSH white;
std::string message;
DWORD seed = 0;
std::ofstream log_file;


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


struct Times
{
   Times():kernel(0),user(0),wall(0){}

   Times& operator-( const Times& rhs ) 
   {
      kernel -= rhs.kernel;
      user   -= rhs.user;
      wall   -= rhs.wall;
      return *this;
   }

   Times& operator+( const Times& rhs ) 
   {
      kernel += rhs.kernel;
      user   += rhs.user;
      wall   += rhs.wall;
      return *this;
   }

   Times& operator/( int i  ) 
   {
      kernel /= i;
      user   /= i;
      wall   /= i;
      return *this;
   }

   std::string to_string() const
   {
      std::ostringstream oss;
      oss << "k" << kernel << "u" << user << "w" << wall;
      return oss.str();
   }

   ULONGLONG kernel,user;
   LONGLONG  wall;
};

class StopWatch
{
public:
   void start()
   {
      start_t = now();
   }

   Times delta_sec() const
   {
      Times t(now() - start_t);
      t.kernel = (t.kernel*100)/1000000000;
      t.user   = (t.user  *100)/1000000000;
      LARGE_INTEGER f;
      QueryPerformanceFrequency(&f);
      t.wall /= f.QuadPart;
      return t; 
   }

   Times delta_msec() const
   {
      Times t(now() - start_t);
      t.kernel = (t.kernel*100)/1000000;
      t.user   = (t.user  *100)/1000000;
      LARGE_INTEGER f;
      QueryPerformanceFrequency(&f);
      t.wall = (t.wall * 1000) / f.QuadPart;
      return t; 
   }

private:
   ULONGLONG ft2ull( const FILETIME& ft ) const
   {
      ULARGE_INTEGER uli;
      uli.HighPart = ft.dwHighDateTime;
      uli.LowPart  = ft.dwLowDateTime;
      return uli.QuadPart;
   }

   Times now() const
   {
      Times t;
      FILETIME c, e, k, u;
      GetProcessTimes( GetCurrentProcess(), &c, &e, &k, &u );
      t.kernel = ft2ull(k);
      t.user   = ft2ull(u);
      
      LARGE_INTEGER li;
      QueryPerformanceCounter(&li);
      t.wall = li.QuadPart;
      return t;
   }

   Times start_t;
};

std::ostream& operator<<(std::ostream& o,const Times& t)
{
   o << t.to_string();
   return o;
}

void init()
{
   cnt = 0;
   cells.reset();
   temp.reset();

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

   cells.set(F_pentomino);

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

   //cells.set( acorn );
}

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

   
   black = HBRUSH(GetStockObject(BLACK_BRUSH));
   white = HBRUSH(GetStockObject(WHITE_BRUSH));
   log_file.open("simple-life.log");

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

void draw_cnt(HDC hdc, RECT cr)
{
   TextOutA(hdc,cr.right-message.length()*8,cr.bottom-20,message.c_str(),message.length());
}

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
      std::ostringstream oss;
      oss << cnt;
      message = oss.str();
      draw_cnt(hdc,cr);
      EndPaint(hWnd, &ps);
   }
}

void play()
{
   while( generations.count(cells)==0 ) {
      for ( int r = 0; r < cells.r; r++ ) {
         for ( int c = 0; c < cells.c; c++ ) {
            Cells::T v(cells.next_at(r,c));
            temp.set(r,c,v);
         }
      }
      if ( log_status ) {
         log_file << cnt << "\n" << cells.rle();
      }
      generations.insert(cells);
      cells = temp;
      cnt++;
   }
}

void OnTimer(HWND hWnd)
{
   static StopWatch sw;
   if ( cnt==0 ) {
      sw.start();
   }
   for ( size_t r = 0; r < grid.r; r++ ) {
      for ( size_t c = 0; c < grid.c; c++ ) {
         Cells::T v(cells.next_at(r,c));
         temp.set(r,c,v);
      }
   }
   if ( log_status ) {
      log_file << cnt << "\n" << cells.rle();
   }
   if ( generations.count(cells) ) {
      KillTimer(hWnd,0);
      Sleep(3000);
      draw_sticky = true;
      cells = temp;
      std::ostringstream oss;
      Times delta = sw.delta_sec();
      oss << "seed=" << seed << " " << cnt << ", " << cnt/delta.wall << "fps, (" << delta << ")sec";
      message = oss.str();
      std::ofstream o("simple-life.txt",std::ios_base::app);
      o << message << "\n";
      o.close();
      InvalidateRect(hWnd,NULL,TRUE);
   } else {
      generations.insert(cells);
      cells = temp;
      cnt++;
      InvalidateRect(hWnd,NULL,TRUE);
   }
}

template < class T >
typename T::value_type average( const T& seq )
{
   return std::accumulate( seq.begin(), seq.end(), typename T::value_type() ) / seq.size();
}

void OnStart(HWND hWnd)
{
   init();
   log_status = false;
   draw_sticky = false;
   SetTimer(hWnd,0,10,NULL);
}

void OnRestart(HWND hWnd)
{
   SetTimer(hWnd,0,10,NULL);
}

void OnStop(HWND hWnd)
{
   KillTimer(hWnd,0);
}

void OnBench(HWND hWnd)
{
   const size_t sz = 3;
   std::array< Times, sz > ts;
   std::array< double, sz > speed;
   for ( size_t i = 0; i < sz; i++ ) {
      init();

      StopWatch t;
      t.start();
      play();
      ts[i] = t.delta_msec();

      speed[i] = double(cnt)/ts[i].wall;
   }

   std::ostringstream oss;
   oss << cnt << ", " << int(1000*average(speed)) << "fps, (" << average(ts) << ")ms";
   message = oss.str();

   std::ofstream o("perf.txt",std::ios_base::app);
   o << message << "\n";
   o.close();

   draw_sticky = true;
   InvalidateRect(hWnd,NULL,TRUE);
}

void OnBenchAndLog(HWND hWnd)
{
   log_status = true;
   OnBench(hWnd);
}

void OnRandom(HWND hWnd)
{
   OnStop(hWnd);
   log_status = false;
   draw_sticky = false;
   cnt = 0 ;
   seed = GetTickCount();
   cells = Cells(170,170,seed,0.05);
   temp.reset();
   OnRestart(hWnd);
}

void OnPlayAndLog(HWND hWnd)
{ 
   log_status = true;
   init();
   draw_sticky = false;
   SetTimer(hWnd,0,10,NULL);
}

bool CommandProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   int wmId    = LOWORD(wParam);
   int wmEvent = HIWORD(wParam);
   switch (wmId)
   {
   case IDM_ABOUT:
      DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
      break;
   case IDM_BENCH:
      OnBench(hWnd);
      break;
   case IDM_BENCH_AND_LOG:
      OnBenchAndLog(hWnd);
      break;
   case IDM_START:
      OnStart(hWnd);
      break;
   case IDM_PLAY_AND_LOG:
      OnPlayAndLog(hWnd);
      break;
   case IDM_RANDOM:
      OnRandom(hWnd);
      break;
   case IDM_RESTART:
      OnRestart(hWnd);
      break;
   case IDM_STOP:
      OnStop(hWnd);
      break;
   case IDM_EXIT:
      DestroyWindow(hWnd);
      break;
   default:
      return false;
   }
   return true;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
   case WM_COMMAND:
      if ( !CommandProc(hWnd,message,wParam,lParam) ) {
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
