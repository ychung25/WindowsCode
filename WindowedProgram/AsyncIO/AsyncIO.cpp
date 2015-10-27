// AsyncIO.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "AsyncIO.h"

#define CK_READ  1
#define CK_WRITE 2

#define BUFFSIZE              (64 * 1024) // The size of an I/O buffer

HANDLE g_hIoCompletionPort;

// Each I/O Request needs an OVERLAPPED structure and a data buffer
class CIOReq : public OVERLAPPED {
public:
	CIOReq() {
		Internal = InternalHigh = 0;
		Offset = OffsetHigh = 0;
		hEvent = NULL;
		m_nBuffSize = 0;
		m_pvData = NULL;
	}

	~CIOReq() {
		if (m_pvData != NULL)
			VirtualFree(m_pvData, 0, MEM_RELEASE);
	}

	BOOL AllocBuffer(SIZE_T nBuffSize) {
		m_nBuffSize = nBuffSize;
		m_pvData = VirtualAlloc(NULL, m_nBuffSize, MEM_COMMIT, PAGE_READWRITE);
		CHAR* pChar = (CHAR*)m_pvData;
		*pChar = 'A';
		return(m_pvData != NULL);
	}

	BOOL Read(HANDLE hDevice, PLARGE_INTEGER pliOffset = NULL) {
		if (pliOffset != NULL) {
			Offset = pliOffset->LowPart;
			OffsetHigh = pliOffset->HighPart;
		}
		return(::ReadFile(hDevice, m_pvData, m_nBuffSize, NULL, this));
	}

	BOOL Write(HANDLE hDevice, PLARGE_INTEGER pliOffset = NULL) {
		if (pliOffset != NULL) {
			Offset = pliOffset->LowPart;
			OffsetHigh = pliOffset->HighPart;
		}
		return(::WriteFile(hDevice, m_pvData, m_nBuffSize, NULL, this));
	}

	PVOID GetData()
	{
		return m_pvData;
	}

private:
	SIZE_T m_nBuffSize;
	PVOID  m_pvData;
};



WCHAR* pszWindowClassName = L"MyWindowClass";

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ThreadPoc(LPVOID lpParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ASYNCIO));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDI_ASYNCIO);
	wcex.lpszClassName = pszWindowClassName;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	RegisterClassExW(&wcex);

	HWND hWnd = CreateWindowW(pszWindowClassName, L"", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);


	g_hIoCompletionPort = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE, NULL, 0, 0);

	CreateThread(NULL, 0, ThreadPoc, NULL, 0, NULL);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
	{
		if (wParam == 'R')
		{
			HANDLE hFile = CreateFile(L"C:\\AWD\\something.txt", GENERIC_WRITE,
				0, NULL, CREATE_ALWAYS,
				FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED, NULL);

			CreateIoCompletionPort(hFile, g_hIoCompletionPort, CK_WRITE, 0);

			CIOReq* pIOReq = new CIOReq();
			pIOReq->AllocBuffer(BUFFSIZE);
			CHAR* pChar = (CHAR*)(pIOReq->GetData());
			*pChar = 'A';
			*(pChar + 1) = 'B';
			pIOReq->Write(hFile);
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code that uses hdc here...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

DWORD WINAPI ThreadPoc(LPVOID lpParam)
{
	ULONG_PTR CompletionKey;
	DWORD dwNumBytes;
	CIOReq* pior;

	GetQueuedCompletionStatus(
		g_hIoCompletionPort,
		&dwNumBytes,
		&CompletionKey,
		(OVERLAPPED**)&pior,
		INFINITE);

	if (CompletionKey == CK_WRITE)
	{
		CloseHandle(g_hIoCompletionPort);
	}

	return S_OK;
}