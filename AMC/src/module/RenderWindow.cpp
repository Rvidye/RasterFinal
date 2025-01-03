#include <RenderWindow.h>
#include <iostream>

namespace AMC {
	static uint64_t clockOffset = 0;
	static LARGE_INTEGER frequency;
	static LARGE_INTEGER startCount;

	uint64_t getTimerValue() {
		LARGE_INTEGER currentCount;
		QueryPerformanceCounter(&currentCount);
		return (uint64_t)(currentCount.QuadPart - startCount.QuadPart);
	}

	RenderWindow::RenderWindow(HINSTANCE instance, const std::wstring& windowClass,const std::wstring& windowTitle, UINT width, UINT height,BOOL fullScreen):
	mInstance(instance),
	mWindowClass(windowClass),
	mWindowTitle(windowTitle),
	mWidth(width),
	mHeight(height),
	mFullScreen(fullScreen),
	mClosed(FALSE),
	mWindowHandle(NULL),
	mKeyboard(NULL),
	mMouse(NULL),
	mHDC(NULL),
	mHGLRC(NULL)
	{}

	RenderWindow::~RenderWindow()
	{}

	void RenderWindow::InitializeWindow()
	{
		int width,height;

		width = GetSystemMetrics(SM_CXSCREEN);
		height = GetSystemMetrics(SM_CYSCREEN);

		mWindowHandle = CreateWindow(
			mWindowClass.c_str(),
			mWindowTitle.c_str(),
			WS_OVERLAPPEDWINDOW,
			(width - mWidth) / 2,
			(height - mHeight) / 2,
			mWidth,
			mHeight,
			NULL,
			NULL,
			mInstance,
			(void*)this);

		if (mWindowHandle == NULL) {
			DWORD error = GetLastError();
			LPVOID errorMsg;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error,
				0, // Default language
				(LPWSTR)&errorMsg,
				0,
				NULL
			);
			OutputDebugString(L"Create Window Failed\n");
			OutputDebugString((LPCWSTR)errorMsg);
			exit(EXIT_FAILURE);
		}
		mClosed = FALSE;
		ShowWindow(mWindowHandle, SW_SHOW);
		SetForegroundWindow(mWindowHandle);
		SetFocus(mWindowHandle);
		LOG_INFO(L"Initialize Window Complete");
	}

	void RenderWindow::ShutDown()
	{
		this->UninitializeGL();
		//ShowCursor(TRUE);
		if (mWindowHandle)
			DestroyWindow(mWindowHandle);

		mWindowHandle = NULL;

		// Remove the application instance
		UnregisterClass(mWindowClass.c_str(), mInstance);

		mInstance = NULL;
		LOG_INFO(L"Window Deleted Successfully");
	}

	void RenderWindow::SetFullScreen(BOOL fullscreen)
	{
		static DWORD dwStyle;
		static WINDOWPLACEMENT wp;
		MONITORINFO mi;

		wp.length = sizeof(WINDOWPLACEMENT);

		if (fullscreen) 
		{
			dwStyle = GetWindowLong(mWindowHandle, GWL_STYLE);
			if (dwStyle & WS_OVERLAPPEDWINDOW) 
			{
				mi.cbSize = sizeof(MONITORINFO);

				if (GetWindowPlacement(mWindowHandle, &wp) && GetMonitorInfo(MonitorFromWindow(mWindowHandle, MONITORINFOF_PRIMARY), &mi))
				{
					SetWindowLong(mWindowHandle, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
					SetWindowPos(mWindowHandle, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
				}
			}
		}
		else
		{
			SetWindowLong(mWindowHandle, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(mWindowHandle, &wp);
			SetWindowPos(mWindowHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
		mFullScreen = fullscreen;
	}

	BOOL RenderWindow::IsClosed()
	{
		return mClosed;
	}

	BOOL RenderWindow::GetFullScreen()
	{
		return mFullScreen;
	}

	UINT RenderWindow::GetWindowWidth()
	{
		return mWidth;
	}

	UINT RenderWindow::GetWindowHeight()
	{
		return mHeight;
	}

	HWND RenderWindow::GetWindowHandle()
	{
		return mWindowHandle;
	}

	FLOAT RenderWindow::AspectRatio()
	{
		return (FLOAT)mWidth/(FLOAT)mHeight;
	}

#ifdef _MYDEBUG
	
	static void GLAPIENTRY DebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		char debugSource[32] = { 0 };
		switch (source)
		{
		case GL_DEBUG_SOURCE_API:
			strcpy_s(debugSource, "OpenGL");
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			strcpy_s(debugSource, "Windows");
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			strcpy_s(debugSource, "Shader Compiler");
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			strcpy_s(debugSource, "Third Party");
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			strcpy_s(debugSource, "Application");
			break;
		case GL_DEBUG_SOURCE_OTHER:
			strcpy_s(debugSource, "Other");
			break;
		}

		char debugType[32] = { 0 };
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
			strcpy_s(debugType, "Error");
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			strcpy_s(debugType, "Deprecated behavior");
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			strcpy_s(debugType, "Undefined behavior");
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			//strcpy_s(debugType, "Portability");
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			strcpy_s(debugType, "Performance");
			break;
		case GL_DEBUG_TYPE_OTHER:
			strcpy_s(debugType, "Message");
			break;
		case GL_DEBUG_TYPE_MARKER:
			strcpy_s(debugType, "Marker");
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			strcpy_s(debugType, "Push group");
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			strcpy_s(debugType, "Pop group");
			break;
		}

		char severityType[32] = { 0 };
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			strcpy_s(severityType, "[OGL-Error]");
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			strcpy_s(severityType, "[OGL-Warning]");
			break;
		case GL_DEBUG_SEVERITY_LOW:
			strcpy_s(severityType, "[OGL-Info]");
			break;
		}

		//if ((id != 131076) && (id != 131184) && (id != 131185) && (id != 131186) && (id != 131188) && (id != 131204) && (id != 131218))
		{
			char buffer[4096];
			sprintf_s(buffer, "%s %s: %s %d: %s\n", severityType, debugSource, debugType, id, message);
			OutputDebugStringA(buffer);
		}
	}
#endif
	LRESULT CALLBACK WndProcTemp(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CREATE:
			return 0;
		case WM_DESTROY:
			return 0;
		}
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	void RenderWindow::InitializeGL()
	{

		//WNDCLASSEX wc2;
		//wc2.cbSize = sizeof(WNDCLASSEX);
		//wc2.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
		//wc2.lpfnWndProc = &WndProcTemp;
		//wc2.cbClsExtra = 0;
		//wc2.cbWndExtra = 0;
		//wc2.hInstance = mInstance;
		//wc2.hIcon = 0;
		//wc2.hCursor = LoadCursor(0, IDC_ARROW);
		//wc2.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		//wc2.lpszMenuName = 0;
		//wc2.lpszClassName = TEXT("oglversionchecksample_class1");
		//wc2.hIconSm = 0;

		//if (RegisterClassEx(&wc2) == 0) {
		//	fprintf(stderr, "Windows failed to register class `%sl`!\n", wc2.lpszClassName);
		//	exit(1);
		//}
		//HWND temp_hWnd = CreateWindowEx(
		//	0,										// dwExStyle
		//	TEXT("oglversionchecksample_class1"),	// lpClassName
		//	TEXT("oglversionchecksample"),			// lpWindowName
		//	WS_OVERLAPPEDWINDOW,					// dwStyle
		//	CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,	// X, Y, nWidth, nHeight
		//	0, 0, mInstance, 0);

		//if (temp_hWnd == nullptr) {
		//	fprintf(stderr, "Windows failed to create window `%sl`!\n", wc2.lpszClassName);
		//	exit(1);
		//}

		mHDC = GetDC(mWindowHandle);
		if (mHDC == nullptr) {
			fprintf(stderr, "Windows failed to provide a display context!\n");
			exit(1);
		}
		PIXELFORMATDESCRIPTOR pfd;
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cRedBits = 0;
		pfd.cRedShift = 0;
		pfd.cGreenBits = 0;
		pfd.cGreenShift = 0;
		pfd.cBlueBits = 0;
		pfd.cBlueShift = 0;
		pfd.cAlphaBits = 0;
		pfd.cAlphaShift = 0;
		pfd.cAccumBits = 0;
		pfd.cAccumRedBits = 0;
		pfd.cAccumGreenBits = 0;
		pfd.cAccumBlueBits = 0;
		pfd.cAccumAlphaBits = 0;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		pfd.cAuxBuffers = 0;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pfd.bReserved = 0;
		pfd.dwLayerMask = 0;
		pfd.dwVisibleMask = 0;
		pfd.dwDamageMask = 0;

		// ChoosePixelFormat https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-choosepixelformat
		// SetPixelFormat https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setpixelformat
		int pixelfmt = ChoosePixelFormat(mHDC, &pfd);
		if (pixelfmt == 0) {
			fprintf(stderr, "Failed to choose pixel format!\n");
			exit(1);
		}

		if (!SetPixelFormat(mHDC, pixelfmt, &pfd)) {
			fprintf(stderr, "Could not set pixel format!\n");
			exit(1);
		}
		printf("Pixel format: %d\n", pixelfmt);

		HGLRC temp_gl_context = wglCreateContext(mHDC);
		if (temp_gl_context == nullptr) {
			fprintf(stderr, "Failed to create GL context!\n");
			exit(1);
		}

		if (!wglMakeCurrent(mHDC, temp_gl_context)) {
			fprintf(stderr, "Failed to make GL context current!\n");
			exit(1);
		}

		PROC proc = wglGetProcAddress("wglGetExtensionsStringEXT");
		const char* (*_wglGetExtensionsStringEXT)(void) = (const char* (*)(void)) proc;
		printf("WGL Extensions Available:\n%s\n\n", _wglGetExtensionsStringEXT());

		if (glewInit() != GLEW_OK)
			LOG_ERROR(L"GLEW INIT Failed");//AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, LOG_INFO, L"GLEW INIT Failed");

		//mHDC = GetDC(mWindowHandle);
		//{
		//	int attrib[] =
		//	{
		//		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		//		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		//		WGL_ACCELERATION_ARB,	WGL_FULL_ACCELERATION_ARB,
		//		WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
		//		WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
		//		WGL_COLOR_BITS_ARB,     32,
		//		WGL_DEPTH_BITS_ARB,     24,
		//		WGL_STENCIL_BITS_ARB,   8,
		//		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
		//		WGL_SAMPLE_BUFFERS_ARB, 1,
		//		WGL_SAMPLES_ARB,        4, // 4x MSAA
		//		0,
		//	};

		//	int format;
		//	UINT formats;
		//	if (!wglChoosePixelFormatARB(mHDC, attrib, NULL, 1, &format, &formats) || formats == 0) 
		//	{
		//		//AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, LOG_INFO, L"wglChoosePixelFormatARB failed");
		//		LOG_ERROR(L"wglChoosePixelFormatARB failed");
		//	}

		//	PIXELFORMATDESCRIPTOR desc = { .nSize = sizeof(desc) };
		//	int ok = DescribePixelFormat(mHDC, format, sizeof(desc), &desc);
		//	if (!ok)
		//	{
		//		//AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, LOG_INFO, L"DescribePixelFormat failed");
		//		LOG_ERROR(L"DescribePixelFormat failed");
		//	}

		//	if (SetPixelFormat(mHDC, format, &desc) == FALSE)
		//	{
		//		//AMC::Log::GetInstance()->WriteLogFile(__FUNCTION__, LOG_INFO, L"SetPixelFormat failed");
		//		LOG_ERROR(L"SetPixelFormat failed");
		//	}
		//}

		//// Create Modern OpenGL Context
		//{
		//	int attrib[] =
		//	{
		//		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		//		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		//		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		//		WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
		//		0,
		//	};

		//	mHGLRC = wglCreateContextAttribsARB(mHDC, NULL, attrib);
		//	if (!mHGLRC)
		//	{
		//		//fprintf(gpFile,"wglCreateContextAttribsARB failed ");
		//		LOG_ERROR(L"wglCreateContextAttribsARB failed");
		//	}

		//	wglMakeCurrent(mHDC, mHGLRC);
		//}

		//wglDeleteContext(temp_gl_context);
		//ReleaseDC(temp_hWnd, temp_dc);
		//DestroyWindow(temp_hWnd);

		//glewExperimental = GL_TRUE; // Ensure GLEW uses modern techniques for managing OpenGL functionality
		//GLenum glewStatus = glewInit();
		//if (glewStatus != GLEW_OK) {
		//	LOG_ERROR(L"GLEW initialization failed for actual context");
		//}

#ifdef _MYDEBUG

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		glDebugMessageCallback(DebugOutput, NULL);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls

		ImGui::StyleColorsDark();
		ImGui_ImplWin32_InitForOpenGL(mWindowHandle);
		ImGui_ImplOpenGL3_Init();
#endif
		// Initialize Timers
		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&startCount);

		// warmup resize

	}

	void RenderWindow::UninitializeGL()
	{
		#ifdef _MYDEBUG
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		#endif
		// Release All OpenGL Stuff Before Destroying Window
		if (mHGLRC)
		{
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(mHGLRC);
			mHGLRC = NULL;
		}
		if (mHDC)
		{
			ReleaseDC(mWindowHandle, mHDC);
			mHDC = NULL;
		}
	}

	DOUBLE RenderWindow::getTime(void)
	{
		return (DOUBLE)(getTimerValue() - clockOffset) / frequency.QuadPart;
	}

	void RenderWindow::SetWindowSize(UINT w, UINT h)
	{
		this->mWidth = w;
		this->mHeight = h;
	}

	void RenderWindow::SetKeyboardFunc(const KeyboardCallback& kbcb)
	{
		mKeyboard = kbcb;
	}

	void RenderWindow::SetMouseFunc(const MouseCallback& mcb)
	{
		mMouse = mcb;
	}

	void RenderWindow::SetResizeFunc(const ResizeCallback& rcb)
	{
		mResize = rcb;
	}

};

