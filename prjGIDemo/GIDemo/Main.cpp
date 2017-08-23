

#include "VK_Helper.h"
#include "Application.h"
#include "Renderer.h"


#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#endif

#define APP_SHORT_NAME "Async Compute Test"

Application::Application application;





int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow) 
{
size_t longS = sizeof(LONG);

	SystemTools::SetLogFile ("stdout.txt");

	//simpleVis.Init ("..\\Engine\\shader\\gui_pass_through.vert.glsl", "..\\Engine\\shader\\gui_pass_through.frag.glsl");

    MSG msg;   // message
    bool done; // flag saying when app is complete
    int argc;
    char **argv;

    // Use the CommandLine functions to get the command line arguments.
    // Unfortunately, Microsoft outputs
    // this information as wide characters for Unicode, and we simply want the
    // Ascii version to be compatible
    // with the non-Windows side.  So, we have to convert the information to
    // Ascii character strings.
    LPWSTR *commandLineArgs = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (NULL == commandLineArgs) 
	{
        argc = 0;
    }

    if (argc > 0) 
	{
        argv = (char **)malloc(sizeof(char *) * argc);
        if (argv == NULL)
		{
            argc = 0;
        } 
		else 
		{
            for (int iii = 0; iii < argc; iii++) 
			{
                size_t wideCharLen = wcslen(commandLineArgs[iii]);
                size_t numConverted = 0;

                argv[iii] = (char *)malloc(sizeof(char) * (wideCharLen + 1));
                if (argv[iii] != NULL) 
				{
                    wcstombs_s(&numConverted, argv[iii], wideCharLen + 1,
                               commandLineArgs[iii], wideCharLen + 1);
                }
            }
        }
    } 
	else 
	{
        argv = NULL;
    }

    
	bool use_staging_buffer = false;
    bool use_break = false;
    bool validate = false;
	int frameCount = INT32_MAX;

	for (int i = 1; i < argc; i++) 
	{
        if (strcmp(argv[i], "--break") == 0) 
		{
            use_break = true;
            continue;
        }
        if (strcmp(argv[i], "--validate") == 0) 
		{
            validate = true;
            continue;
        }
        if (strcmp(argv[i], "--c") == 0 && frameCount == INT32_MAX && i < argc - 1 
			&& sscanf(argv[i + 1], "%d", &frameCount) == 1 && frameCount >= 0) 
		{
            i++;
            continue;
        }

        fprintf(stderr, "Usage:\n  %s [--use_staging] [--validate] [--break] [--c <framecount>]\n", APP_SHORT_NAME);
        fflush(stderr);
        exit(1);
    }
    // Free up the items we had to allocate for the command line arguments.
    if (argc > 0 && argv != NULL) 
	{
        for (int iii = 0; iii < argc; iii++) 
		{
            if (argv[iii] != NULL) 
			{
                free(argv[iii]);
            }
        }
        free(argv);
    }



	Renderer::Demo cubeDemo;
	Renderer::Demo *demo = &cubeDemo;

	//application.context = &demo.context;
	application.Setup (Renderer::demo_run2, Renderer::demo_resize, demo);











validate = true; use_break = true;
	


	Renderer::demo_init (demo, &application, use_break, validate, frameCount);


    //int width = 1280; int height = 720;
    int width = 1920; int height = 1080;

    HWND window = application.CreateApplicationWindow (hInstance, width, height, APP_SHORT_NAME);
    
	demo->swapchain.Init (demo->context, hInstance, window, width, height, 0);
	demo->quit = false;
	

    Renderer::demo_prepare(demo);

    done = false; // initialize loop condition variable


	

    // main message loop
    while (!done) 
	{
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT) // check for a quit message
			{
				done = true; // if found, quit app
			} 
			else 
			{
				/* Translate and dispatch to event queue*/
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
        RedrawWindow(demo->swapchain.window, NULL, NULL, RDW_INTERNALPAINT);
    }

    Renderer::demo_cleanup(demo);

    return 0;//(int)msg.wParam;
}

