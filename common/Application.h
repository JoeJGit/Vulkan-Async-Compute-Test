#pragma once

#include "SystemTools.h"
#include <stdint.h>
#include <assert.h>



#define ERR_EXIT(err_msg, err_class)                                           \
    do {                                                                       \
        MessageBox(NULL, err_msg, err_class, MB_OK);                           \
        exit(1);                                                               \
    } while (0)



namespace Application
{
	enum 
	{
		KEY_CTRL,
		KEY_SHIFT,
		KEY_ALT,
		KEY_SUPER,
		KEY_SPACE,
		KEY_HOME,
		KEY_END,
		KEY_UP,
		KEY_DOWN,
		KEY_LEFT,
		KEY_RIGHT,
		KEY_PGUP,
		KEY_PGDOWN,
		KEY_TAB,
		KEY_LBUTTON,
		KEY_MBUTTON,	
		KEY_RBUTTON,
		KEY_BACK,	
		KEY_CLEAR,	
		KEY_RETURN,
		KEY_INSERT,
		KEY_DELETE,
		KEY_ESCAPE,
		//KEY_0, // '0'...'9'
		//KEY_A, // 'A'...'Z'
	};

	struct Application 
	{
		static Application *globalApplication; // using global variable because Win32 don't let us store 64 bit user data

		void (*drawFunction)(Application*);
		void (*resizeFunction)(Application*, int, int);
		void* userData;

		uint64_t keys[4];
		uint64_t prevKeys[4];
		int mousePosX, mousePosY;
		int mousePrevX, mousePrevY;
		int mouseMulX, mouseMulY;
		bool forceMouseCapture;
		char charBuffer[64];
		int charIndex;
		int mouseWheel;

		Application ()
		{
			// there may only be ONE Application!
			assert (globalApplication == nullptr);
			globalApplication = this;

			drawFunction = nullptr;
			resizeFunction = nullptr;
			userData = nullptr;

			keys[0] = keys[1] = keys[2] = keys[3] = 0;
			prevKeys[0] = prevKeys[1] = prevKeys[2] = prevKeys[3] = 0;
			mousePosX = 0, mousePosY = 0;
			mousePrevX = 0, mousePrevY = 0;
			mouseMulX = 0, mouseMulY = 0;
			forceMouseCapture = false;
			charIndex = 0;
			mouseWheel = 0;
		}

		void Setup (
			void (*drawFunction)(Application*), 
			void (*resizeFunction)(Application*, int, int),
			void* userData)
		{
			this->drawFunction = drawFunction;
			this->resizeFunction = resizeFunction;
			this->userData = userData;
		}


		void StorePrevKeyState ()
		{
			prevKeys[0] = keys[0];	prevKeys[1] = keys[1];
			prevKeys[2] = keys[2];	prevKeys[3] = keys[3];
		}

		void SetKeyState (const int key, const int state)
		{
			if (state) keys[key>>6] |= (uint64_t(1)<<uint64_t(key&63)); 
			else keys[key>>6] &= ~(uint64_t(1)<<uint64_t(key&63));
		}

		bool KeyDown (const int key)
		{
			uint64_t mask = keys[key>>6] & (uint64_t(1)<<uint64_t(key&63));
			return mask != 0;
		}

		bool KeyPressed (const int key)
		{
			uint64_t bit = (uint64_t(1)<<uint64_t(key&63));
			uint64_t mask = ((prevKeys[key>>6]^keys[key>>6]) & bit) && (keys[key>>6] & bit);
			return mask != 0;
		}

		bool KeyReleased (const int key)
		{
			uint64_t bit = (uint64_t(1)<<uint64_t(key&63));
			uint64_t mask = ((prevKeys[key>>6]^keys[key>>6]) & bit) && (prevKeys[key>>6] & bit);
			return mask != 0;
		}

		bool MouseCaptured ()
		{
			if (KeyDown(KEY_ESCAPE)) return false;
			if (forceMouseCapture) return true;
			uint64_t anyKey = keys[0] | keys[1] | keys[2] | keys[3];
			return anyKey != 0;
		}

		void SetMousePosition (const int x, const int y)
		{
			mousePosX = x;
			mousePosY = y;
		}

		void StorePrevMousePosition ()
		{
			mousePrevX = mousePosX;
			mousePrevY = mousePosY;
		}

		void AddChar (char c)
		{
			if (charIndex < 63)
			{
				charBuffer[charIndex++] = c;
				charBuffer[charIndex] = 0;
			}
		}

		char PopChar ()
		{
			if (charIndex == 0) return 0;
			char r = charBuffer[0];
			for (int i=0; i<charIndex; i++) charBuffer[i] = charBuffer[i+1];
			return r;
		}

		int MouseWheelDelta ()
		{
			int r = mouseWheel;
			mouseWheel = 0;
			return r;
		}


		



		// MS-Windows event handling function:
		static LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) 
		{
			static char keyMap[] = {
				KEY_CTRL,	VK_CONTROL,	KEY_SHIFT,	VK_SHIFT,	KEY_SPACE,	VK_SPACE,	KEY_HOME,	VK_HOME,
				KEY_END,	VK_END,		KEY_UP,		VK_UP,		KEY_DOWN,	VK_DOWN,	KEY_LEFT,	VK_LEFT,
				KEY_RIGHT,	VK_RIGHT,	KEY_TAB,	VK_TAB,		KEY_RETURN,	VK_RETURN,	KEY_LBUTTON,VK_LBUTTON,
				KEY_RBUTTON,VK_RBUTTON,	KEY_MBUTTON,VK_MBUTTON,	KEY_BACK,	VK_BACK,	KEY_CLEAR,	VK_CLEAR,
				KEY_INSERT,	VK_INSERT,	KEY_DELETE,	VK_DELETE,  KEY_ESCAPE,	VK_ESCAPE,	KEY_ALT,	VK_MENU, 
				KEY_PGUP,	VK_PRIOR,	KEY_PGDOWN, VK_NEXT,	
			};

			//if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP)
			//if (uMsg == WM_INPUT)
			{
				for (int i=0; i<sizeof(keyMap); i+=2) globalApplication->SetKeyState (keyMap[i], GetKeyState(keyMap[i+1])<0);
				for (int i=0; i<10; i++) globalApplication->SetKeyState ('0'+i,	(GetKeyState('0'+i)<0));
				for (int i=0; i<26; i++) globalApplication->SetKeyState ('A'+i,	(GetKeyState('A'+i)<0));
			}

			
			
			if (uMsg == WM_MOUSEMOVE)
			{
				if (globalApplication->MouseCaptured())
				{
					RECT winRect;
					GetClientRect (hWnd, &winRect);
					ClientToScreen (hWnd, (LPPOINT)&winRect.left);
					ClientToScreen (hWnd, (LPPOINT)&winRect.right);
					ClipCursor (&winRect);

					POINT curpos;
					GetCursorPos(&curpos);

					int width =  max (3, winRect.right - winRect.left);
					int height = max (3, winRect.bottom - winRect.top);
			
					bool moved = false;
					if (curpos.x <= winRect.left)					{curpos.x += (width-2);		globalApplication->mouseMulX++; moved = true;}
					else if (curpos.x >= winRect.left + width - 1)	{curpos.x -= (width-2);		globalApplication->mouseMulX--; moved = true;}
					if (curpos.y <= winRect.top)					{curpos.y += (height-2);	globalApplication->mouseMulY++; moved = true;}
					else if (curpos.y >= winRect.top + height - 1)	{curpos.y -= (height-2);	globalApplication->mouseMulY--; moved = true;}
			
					globalApplication->SetMousePosition (
						curpos.x - winRect.left - globalApplication->mouseMulX * (width-2), 
						curpos.y - winRect.top  - globalApplication->mouseMulY * (height-2) );

					if (moved)
					{
						SetCursorPos (curpos.x,curpos.y); // creates a new WM_MOUSEMOVE message
						//UpdateWindow(hWnd); // force WM_PAINT over WM_MOUSEMOVE
					}
				}
				else 
				{
					ClipCursor (0);
					globalApplication->mouseMulX = 0;
					globalApplication->mouseMulY = 0;
					globalApplication->SetMousePosition (LOWORD (lParam), HIWORD (lParam));
				}	
			}
			else switch (uMsg) 
			{
			case WM_CLOSE:
				PostQuitMessage(0);
				break;
			case WM_PAINT:
				//demo_run(&demo);
				if (globalApplication->drawFunction) globalApplication->drawFunction(globalApplication);
				if (globalApplication->KeyReleased(KEY_ESCAPE)) globalApplication->forceMouseCapture = !globalApplication->forceMouseCapture;
				globalApplication->StorePrevKeyState ();
				globalApplication->StorePrevMousePosition ();
				break;
			case WM_SIZE:
				// Resize the application to the new window size, except when
				// it was minimized. Vulkan doesn't support images or swapchains
				// with width=0 and height=0.
				if (wParam != SIZE_MINIMIZED) 
				{
					if (globalApplication->resizeFunction) globalApplication->resizeFunction(globalApplication, lParam & 0xffff, (lParam>>16) & 0xffff);
				}
				break;
			case WM_CHAR:
				globalApplication->AddChar ((char)wParam);
				break;
			case WM_MOUSEWHEEL:
				globalApplication->mouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) / 120;
				break;
			default:
				break;
			}

			return (DefWindowProc(hWnd, uMsg, wParam, lParam));
		}





		HWND CreateApplicationWindow (HINSTANCE hInstance, int width, int height, char *name) 
		{
			SystemTools::Log ("CreateApplicationWindow...\n");


			WNDCLASSEX win_class;

			// Initialize the window class structure:
			win_class.cbSize = sizeof(WNDCLASSEX);
			win_class.style = CS_HREDRAW | CS_VREDRAW;
			win_class.lpfnWndProc = WndProc;
			win_class.cbClsExtra = 0;
			win_class.cbWndExtra = 0;
			win_class.hInstance = hInstance;//appSet->connection; // 
			win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
			win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
			win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
			win_class.lpszMenuName = NULL;
			win_class.lpszClassName = name;
			win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

			// Register window class:
			if (!RegisterClassEx(&win_class)) 
			{
				// It didn't work, so try to give a useful error:
				SystemTools::Log ("Unexpected error trying to start the application!\n");
				exit(1);
			}

			// Create window with the registered class:
			RECT wr = {0, 0, width, height};
			AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
			HWND window = CreateWindowEx(0,
										  name,           // class name
										  name,           // app name
										  WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_SYSMENU,// window style										  
										  100, 100,           // x/y coords
										  wr.right - wr.left, // width
										  wr.bottom - wr.top, // height
										  NULL,               // handle to parent
										  NULL,               // handle to menu
										  hInstance,//appSet->connection,    
										  NULL);              // no extra parameters
			if (!window) 
			{
				// It didn't work, so try to give a useful error:
				SystemTools::Log ("Cannot create window!\n");
				exit(1);
			}

			return window;
		}

	};

};