#define _CRT_SECURE_NO_WARNINGS			//VS 구문 사용 안전성을 위해 발생하는 오류 무시

#include <WinSock2.h>	//윈도우 소켓 라이브러리
#pragma comment(lib, "ws2_32.lib")	//윈도우 라이브러리와 윈도우 소켓 라이브러리 충돌 방지문
#include <Windows.h>	//윈도우 라이브러리
#include <stdio.h>		//기본 입출력 라이브러리
#include <tchar.h>		//TCHAR 관련 라이브러리
#include "resource.h"	//Dialog 관련 라이브러리

#ifdef _DEBUG
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console" )
#endif

#define WM_SOCKET1 104

typedef struct _serialConnInfo{		//시리얼 포트 입력 정보를 담기 위한 구조체
	TCHAR szPortNumber[16];		//포트 번호
	int nBaudRate = 0;			//보 레이트
	int nDataBits = 0;			//데이터 비트
	TCHAR szParityBits[16];		//패리티 비트
	int nStopBits = 0;			//스탑 비트
}serialConnInfo;

char _baudItems[][10] = {"110","300","600","1200","2400","4800","9600","14400","19200","38400","57600","115200","230400","460800","921600"};
char _dataItems[][2] = {"7","8"};
char _parityItems[][5] = {"even","odd","none"};
char _stopItems[][2] = {"1","2"};

#define DEFAULT_BAUD _baudItems[0]
#define DEFAULT_DATA _dataItems[0]
#define DEFAULT_PARITY _parityItems[0]
#define DEFAULT_STOP _stopItems[0]

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);		//윈도우 프로시저 콜백 함수 정의
HINSTANCE g_hInst;											
LPCTSTR lpszClass = "Serial_n_Socket";						//윈도우 이름 지정
TCHAR szTemp[128];
TCHAR _szUpper9[5] = {92,92,'.',92};
//TCHAR* szUpper9 = _stprintf(szUpper9, "%s", _szUpper9);

HWND hDialog1;

HWND hLabel1, hLabel2,hLabel3,hLabel4, hLabel5, hLabel6, hLabel7, hLabel8, hLabel9, hLabel10,
hLabel11, hLabel12, hLabel13;

HWND hTxtBox1, hTxtBox2, hTxtBox3, hTxtBox4, hTxtBox5, hTxtBox6, hTxtBox7, hTxtBox8, hTxtBox9, hTxtBox10,
hTxtBox11, hTxtBox12, hTxtBox13;

HWND hCmbBox1, hCmbBox2, hCmbBox3, hCmbBox4;

HWND hBtn1, hBtn2, hBtn3, hBtn4, hBtn5, hBtn6, hBtn7,hBtn8;

//Serial Port Initialize.
HANDLE hSerial = NULL;				//시리얼 포트 1
HANDLE hSerial2 = NULL;				//시리얼 포트 2
DCB dcbSerialParams = { 0 };		//DCB 시리얼 파라메터 구조체
COMMTIMEOUTS timeouts = { 0 };		//타임아웃 구조체
DWORD RXData(LPVOID);
DWORD nThreadId;
DWORD dwByte;
bool bSelectServer = true;
int nSwitch = 1;
char szRXBuffer[10240];
char szRX[128];
char szTXBuffer[10240];
char szTX[128];

serialConnInfo sConnInfo;
HANDLE hRecvThread;

void SerialConnect();

//Socket Port Initialize.
WSADATA wsaData;
SOCKET hServerSocket;
SOCKET hHandleSocket;
SOCKET hClientSocket;
char szRecvBuffer[10240] = "";

SOCKADDR_IN servAddr;
SOCKADDR hndlAddr;
SOCKADDR_IN dstAddr;
int szClientAddr;

char szServTotalBuffer[10240] = "";
char szClntTotalBuffer[10240] = "";

void InitservSocket(HWND);
void InitclientSocket(HWND);
void sendMessage(SOCKET, char*, char*);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpszCmdParam, int ncmdShow) {
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW , CW_USEDEFAULT, CW_USEDEFAULT, 700, 500, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd,ncmdShow);
	//DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), HWND_DESKTOP, NULL);
	

	while (GetMessage(&Message, NULL, 0, 0)) {
		if (IsDialogMessage(hWnd, &Message) == 0) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
	}

	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	switch (iMessage) {
	case WM_CREATE:
		AllocConsole();		//콘솔 창 띄우기

		hLabel1 = CreateWindow("static", "RX (Board To PC)", WS_CHILD | WS_VISIBLE, 25, 25, 150, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel2 = CreateWindow("static", "TX (PC To Board)", WS_CHILD | WS_VISIBLE, 250, 25, 150, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel3 = CreateWindow("static", "Recv", WS_CHILD | WS_VISIBLE, 25, 275, 50, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel4 = CreateWindow("static", "Send", WS_CHILD | WS_VISIBLE, 250, 275, 50, 20, hWnd, (HMENU)NULL, NULL, NULL);

		hLabel5 = CreateWindow("Button", "Serial Port", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 440, 25, 220, 235, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel6 = CreateWindow("static", "PortNumber", WS_CHILD | WS_VISIBLE, 450, 65, 90, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel7 = CreateWindow("static", "BaudRate", WS_CHILD | WS_VISIBLE, 450, 95, 90, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel8 = CreateWindow("static", "DataBits", WS_CHILD | WS_VISIBLE, 450, 125, 90, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel9 = CreateWindow("static", "ParityBits", WS_CHILD | WS_VISIBLE, 450, 155, 90, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel10 = CreateWindow("static", "StopBits", WS_CHILD | WS_VISIBLE, 450, 185, 90, 20, hWnd, (HMENU)NULL, NULL, NULL);

		hLabel11 = CreateWindow("Button", "Socket Port", WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 440, 275, 235, 175, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel12 = CreateWindow("static", "IP Address", WS_CHILD | WS_VISIBLE, 450, 335, 90, 20, hWnd, (HMENU)NULL, NULL, NULL);
		hLabel13 = CreateWindow("static", "PortNumber", WS_CHILD | WS_VISIBLE, 450, 365, 90, 20, hWnd, (HMENU)NULL, NULL, NULL);

		hTxtBox1 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | WS_TABSTOP, 25, 50, 150, 150, hWnd, (HMENU)0, g_hInst, NULL);
		hTxtBox2 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | WS_TABSTOP, 250, 50, 150, 150, hWnd, (HMENU)1, g_hInst, NULL);
		hTxtBox3 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | WS_TABSTOP, 25, 300, 150, 150, hWnd, (HMENU)2, g_hInst, NULL);
		hTxtBox4 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_VSCROLL | ES_AUTOVSCROLL | WS_TABSTOP, 250, 300, 150, 150, hWnd, (HMENU)3, g_hInst, NULL);
		hTxtBox5 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE, 550, 65, 90, 20, hWnd, (HMENU)4, g_hInst, NULL);		//PortNumber
		
		hCmbBox1 = CreateWindow("combobox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_TABSTOP, 550, 95, 90, 270, hWnd, (HMENU)5, g_hInst, NULL);		//BaudRate
		hCmbBox2 = CreateWindow("combobox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_TABSTOP, 550, 125, 90, 60, hWnd, (HMENU)6, g_hInst, NULL);		//DataBits	
		hCmbBox3 = CreateWindow("combobox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_TABSTOP, 550, 155, 90, 80, hWnd, (HMENU)7, g_hInst, NULL);		//ParityBits
		hCmbBox4 = CreateWindow("combobox", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | WS_TABSTOP, 550, 185, 90, 60, hWnd, (HMENU)8, g_hInst, NULL);		//StopBits

		for (int i = 0; i < sizeof(_baudItems) / sizeof(_baudItems[0]); i++) {
			SendMessage(hCmbBox1, CB_ADDSTRING, 0, (LPARAM)_baudItems[i]);
		}
		for (int i = 0; i < sizeof(_dataItems) / sizeof(_dataItems[0]) ; i++) {
			SendMessage(hCmbBox2, CB_ADDSTRING, 0, (LPARAM)_dataItems[i]);
		}
		for (int i = 0; i < sizeof(_parityItems) / sizeof(_parityItems[0]); i++) {
			SendMessage(hCmbBox3, CB_ADDSTRING, 0, (LPARAM)_parityItems[i]);
		}
		for (int i = 0; i < sizeof(_baudItems) / sizeof(_baudItems[0]); i++) {
			SendMessage(hCmbBox4, CB_ADDSTRING, 0, (LPARAM)_stopItems[i]);
		}

		SetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox1), DEFAULT_BAUD);
		SetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox2), DEFAULT_DATA);
		SetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox3), DEFAULT_PARITY);
		SetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox4), DEFAULT_STOP);
		//hTxtBox6 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE, 550, 95, 90, 20, hWnd, (HMENU)5, g_hInst, NULL);
		//hTxtBox7 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE, 550, 125, 90, 20, hWnd, (HMENU)6, g_hInst, NULL);
		//hTxtBox8 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE, 550, 155, 90, 20, hWnd, (HMENU)7, g_hInst, NULL);
		//hTxtBox9 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE, 550, 185, 90, 20, hWnd, (HMENU)8, g_hInst, NULL);
		hTxtBox10 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 550, 335, 120, 20, hWnd, (HMENU)9, g_hInst, NULL);		//IP Address
		hTxtBox11 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP, 550, 365, 90, 20, hWnd, (HMENU)10, g_hInst, NULL);	//Port Number
		hTxtBox12 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE, 10, 205, 90, 20, hWnd, (HMENU)17, g_hInst, NULL);	//Port Number
		hTxtBox13 = CreateWindowEx(WS_EX_CLIENTEDGE, "Edit", "", WS_CHILD | WS_VISIBLE, 120, 205, 90, 20, hWnd, (HMENU)19, g_hInst, NULL);	//Msg

		

		hBtn1 = CreateWindow("Button", "Connect", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 450, 220, 90, 30, hWnd, (HMENU)11, g_hInst, NULL);
		hBtn2 = CreateWindow("Button", "Disconnect", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 550, 220, 90, 30, hWnd, (HMENU)12, g_hInst, NULL);
		hBtn3 = CreateWindow("Button", "Connect", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 450, 405, 90, 30, hWnd, (HMENU)13, g_hInst, NULL);
		hBtn4 = CreateWindow("Button", "Disconnect", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 550, 405, 90, 30, hWnd, (HMENU)14, g_hInst, NULL);
		hBtn5 = CreateWindow("Button", "Server", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 550, 285, 90, 30, hWnd, (HMENU)15, g_hInst, NULL);
		hBtn6 = CreateWindow("Button", "Client", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON, 550, 305, 90, 30, hWnd, (HMENU)16, g_hInst, NULL);
		hBtn7 = CreateWindow("Button", "Board TX Conn.", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 10, 235, 110, 30, hWnd, (HMENU)18, g_hInst, NULL);
		hBtn8 = CreateWindow("Button", "Board TX Msg", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 120, 235, 110, 30, hWnd, (HMENU)20, g_hInst, NULL);

		SendMessage(hBtn5, BM_SETCHECK, BST_CHECKED, 0);
		EnableWindow(hTxtBox10, false);

		ZeroMemory(szRXBuffer, sizeof(szRXBuffer));
		ZeroMemory(szRX, sizeof(szRX));
		ZeroMemory(szTXBuffer, sizeof(szTXBuffer));
		ZeroMemory(szTX, sizeof(szTX));

		return 0;
	case WM_COMMAND:		//버튼 클릭 시 이벤트 발생 (HMENU)Number
		switch (LOWORD(wParam)) {
			//Button
		case 11:
			GetDlgItemText(hWnd, GetDlgCtrlID(hTxtBox5), szTemp, sizeof(szTemp));

			if (strlen(szTemp) >= 5) { strcpy(sConnInfo.szPortNumber, strcat(_szUpper9, szTemp)); }
			else _tcscpy(sConnInfo.szPortNumber, szTemp);

			printf("Serial Port Number : ");
			if (sConnInfo.szPortNumber != NULL) printf(sConnInfo.szPortNumber);
			printf("\r\n");

			GetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox1), szTemp, sizeof(szTemp));
			sConnInfo.nBaudRate = (int)szTemp;
			printf("BaudRate : ");
			if (sConnInfo.nBaudRate != NULL)	printf((TCHAR*)sConnInfo.nBaudRate);
			printf("\r\n");

			GetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox2), szTemp, sizeof(szTemp));
			sConnInfo.nDataBits = (int)szTemp;
			printf("DataBits : ");
			if (sConnInfo.nDataBits != NULL)	printf((TCHAR*)sConnInfo.nDataBits);
			printf("\r\n");

			GetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox3), szTemp, sizeof(szTemp));
			_tcscpy(sConnInfo.szParityBits, szTemp);
			printf("ParityBits : ");
			if (sConnInfo.szParityBits != NULL)	printf(sConnInfo.szParityBits);
			printf("\r\n");

			GetDlgItemText(hWnd, GetDlgCtrlID(hCmbBox4), szTemp, sizeof(szTemp));
			sConnInfo.nStopBits = (int)szTemp;
			printf("StopBits : ");
			if (sConnInfo.nStopBits)				printf((TCHAR*)sConnInfo.nStopBits);
			printf("\r\n");

			SerialConnect();
			EnableWindow(hBtn1, false);

			break;

		case 12:
			//printf("Serial Disconnect!\n");
			printf("%s 포트를 닫았습니다! - Serial \n", sConnInfo.szPortNumber);
			TerminateThread(hRecvThread, 0);
			CloseHandle(hRecvThread);
			CloseHandle(hSerial);

			EnableWindow(hBtn1, true);
			break;
		case 13:
			if (bSelectServer) {
				InitservSocket(hWnd);
				//printf("Port Open!\n");
			}
			else {
				InitclientSocket(hWnd);
				//printf("Socket Connect!\n");
			}
			EnableWindow(hBtn3, false);
			EnableWindow(hBtn5, false);
			EnableWindow(hBtn6, false);
			break;
		case 14:
			if (bSelectServer) {
				closesocket(hServerSocket);
				printf("%d 포트를 닫았습니다! - Server \n", ntohs(servAddr.sin_port));
				WSACleanup();

			}
			else {
				closesocket(hClientSocket);
				printf("%d 포트의 접속을 종료했습니다! - Client \n", ntohs(dstAddr.sin_port));
				WSACleanup();
			}
			EnableWindow(hBtn3, true);
			EnableWindow(hBtn5, true);
			EnableWindow(hBtn6, true);
			//printf("Socket DIsconnect!\n");
			break;

			//radio Button
		case 15:
			bSelectServer = true;
			EnableWindow(hTxtBox10, false);
			break;
		case 16:
			bSelectServer = false;
			EnableWindow(hTxtBox10, true);
			break;
		case 18:
			char _porttmp[20];
			GetWindowText(hTxtBox12, _porttmp, sizeof(_porttmp));
			hSerial2 = CreateFile(
				_porttmp, GENERIC_READ | GENERIC_WRITE, 0, NULL,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

			dcbSerialParams.BaudRate = 115200;//sConnInfo.nBaudRate;
			dcbSerialParams.ByteSize = 8;//sConnInfo.nDataBits;
			dcbSerialParams.Parity = NOPARITY;
			dcbSerialParams.StopBits = ONESTOPBIT;

			if (SetCommState(hSerial2, &dcbSerialParams) == 0) {
				printf("시리얼 포트 정보 입력 오류 - PC1.\n");
				CloseHandle(hSerial2);
			}

			timeouts.ReadIntervalTimeout = 50;
			timeouts.ReadTotalTimeoutConstant = 50;
			timeouts.ReadTotalTimeoutMultiplier = 10;
			timeouts.WriteTotalTimeoutConstant = 50;
			timeouts.WriteTotalTimeoutMultiplier = 10;
			if (SetCommTimeouts(hSerial2, &timeouts) == 0) {
				printf("시리얼 타임 아웃 정보 입력 오류. - PC1\n");
				CloseHandle(hSerial2);
			}
			else {
				printf(_porttmp);
				printf("포트가 연결되었습니다.\n");
			}
			break;
		case 20:
			char _msgtmp[128];
			DWORD dwByte2;
			GetDlgItemText(hWnd, GetDlgCtrlID(hTxtBox13), _msgtmp, sizeof(_msgtmp));
			if (hSerial2 != NULL) {
				WriteFile(hSerial2, _msgtmp, 8, &dwByte2, 0);
			}
			break;
		}


	case WM_SOCKET1:
		switch (WSAGETSELECTEVENT(lParam)) {
			case FD_READ:		//소켓 Recv
			{
								char szInComing[128];


								ZeroMemory(szTX, sizeof(szTX));
								ZeroMemory(szInComing, sizeof(szInComing));
								int inDataLength;
								if (bSelectServer) {
									inDataLength = recv(hHandleSocket, (char*)szInComing,
										sizeof(szInComing), 0);
								}
								else {
									inDataLength = recv(hClientSocket, (char*)szInComing,
										sizeof(szInComing), 0);
								}
								strcat(szRecvBuffer, szInComing);
								strcpy(szTX, szInComing);
								WriteFile(hSerial, szTX, (DWORD)sizeof(szTX), &dwByte, NULL);
								strcat(szTXBuffer, szTX);

								if (strlen(szRecvBuffer) >= 10000) { memset(&szRecvBuffer, 0, sizeof(szRecvBuffer)); }
								if (strlen(szTXBuffer) >= 10000) {	ZeroMemory(szTXBuffer, sizeof(szTXBuffer));	}

									SetWindowText(hTxtBox3, szRecvBuffer);
									SendMessageA(hTxtBox3, EM_SETSEL, 0, -1);
									SendMessageA(hTxtBox3, EM_SETSEL, -1, -1);
									SendMessageA(hTxtBox3, EM_SCROLLCARET, 0, 0);

									SetWindowText(hTxtBox2, szTXBuffer);
									SendMessageA(hTxtBox2, EM_SETSEL, 0, -1);
									SendMessageA(hTxtBox2, EM_SETSEL, -1, -1);
									SendMessageA(hTxtBox2, EM_SCROLLCARET, 0, 0);
								
			}
			break;
			case FD_CLOSE:
			{
							 printf("포트를 닫았습니다.\n");
			}
			break;
			case FD_ACCEPT:
			{
							  int nHndlSize = sizeof(sockaddr);
							  hHandleSocket = accept(wParam, &hndlAddr, &nHndlSize);
							  if (hHandleSocket == INVALID_SOCKET) {
								  int nret = WSAGetLastError();
								  printf("WSA를 치웠습니다!! \n");
								  WSACleanup();
							  }
							  printf("%d.%d.%d.%d 클라이언트가 접속했습니다.\n", hndlAddr.sa_data[2], hndlAddr.sa_data[3], hndlAddr.sa_data[4], hndlAddr.sa_data[5]);
			}
				break;
			case FD_CONNECT:
			{
							   printf("연결되었습니다! - FD CONNECT\n");
			}
				break;
		}
			break;
			return 0;
		case WM_DESTROY:
			nSwitch = 0;
			FreeConsole();
			PostQuitMessage(0);
			return 0;
		}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void SerialConnect() {

	hSerial = CreateFile(
		sConnInfo.szPortNumber, GENERIC_READ | GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	dcbSerialParams.BaudRate = sConnInfo.nBaudRate;
	dcbSerialParams.ByteSize = sConnInfo.nDataBits;
	if (_tccmp(sConnInfo.szParityBits, "none") == 0)
		dcbSerialParams.Parity = NOPARITY;
	else if (_tccmp(sConnInfo.szParityBits, "even") == 0)
		dcbSerialParams.Parity = EVENPARITY;
	else if (_tccmp(sConnInfo.szParityBits, "odd") == 0)
		dcbSerialParams.Parity = ODDPARITY;
	else
		dcbSerialParams.Parity = NOPARITY;
	
	if(sConnInfo.nStopBits == 1) dcbSerialParams.StopBits = ONESTOPBIT;
	else if (sConnInfo.nStopBits == 1.5f) dcbSerialParams.StopBits = ONE5STOPBITS;
	else if (sConnInfo.nStopBits == 2.0f) dcbSerialParams.StopBits = TWOSTOPBITS;

	if (SetCommState(hSerial, &dcbSerialParams) == 0) {
		printf("시리얼 포트 정보 입력 오류 - PC1.\n");
		CloseHandle(hSerial);
	}

	timeouts.ReadIntervalTimeout = 50;
	timeouts.ReadTotalTimeoutConstant = 50;
	timeouts.ReadTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 50;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	if (SetCommTimeouts(hSerial, &timeouts) == 0) {
		printf("시리얼 타임 아웃 정보 입력 오류. - PC1\n");
		CloseHandle(hSerial);
	}
	else {
		printf(sConnInfo.szPortNumber);
		printf("포트가 연결되었습니다.\n");
	}

	hRecvThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)RXData,NULL,0,&nThreadId);
}

DWORD RXData(VOID* dummy) {			//serial Port 로 데이터를 받는 경우
	char cRecv[10] = "";

	int nRet;

	while (nSwitch) {
		memset(&szRX[0], 0, sizeof(szRX));
		nRet = ReadFile(hSerial, szRX, (DWORD)sizeof(szRX), &dwByte, NULL);
		printf(szRX);
		strcat(szRXBuffer, szRX);

		if (strlen(szServTotalBuffer) >= 10000) {	memset(&szServTotalBuffer, 0, sizeof(szServTotalBuffer));	}
		if (strlen(szClntTotalBuffer) >= 10000) { memset(&szClntTotalBuffer, 0, sizeof(szClntTotalBuffer)); }

		if (strlen(szTXBuffer) >= 10000) {	memset(&szTXBuffer, 0, sizeof(szTXBuffer));	}
		if (strlen(szRXBuffer) >= 10000) {	memset(&szRXBuffer, 0, sizeof(szRXBuffer));	}

		if (bSelectServer) { sendMessage(hHandleSocket, szRX, szServTotalBuffer); }
		else				{ sendMessage(hClientSocket, szRX, szClntTotalBuffer); }

		SetWindowText(hTxtBox1,szRXBuffer);
		SendMessageA(hTxtBox1,EM_SETSEL, 0, -1);
		SendMessageA(hTxtBox1,EM_SETSEL, -1, -1);
		SendMessageA(hTxtBox1,EM_SCROLLCARET, 0, 0);
		
	}

	return 0;
}

void InitservSocket(HWND hWnd) {
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WsaData 생성 실패! - Server \n");
	}

	hServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hServerSocket == INVALID_SOCKET) {
		printf("소켓을 사용할 수 없습니다! - Server \n");
	}

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(GetDlgItemInt(hWnd,GetDlgCtrlID(hTxtBox11),NULL,NULL));

	if (bind(hServerSocket, (LPSOCKADDR)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
		printf("주소,포트 할당 오류! - Server \n");
	}
	if (WSAAsyncSelect(hServerSocket, hWnd, WM_SOCKET1, (FD_CLOSE | FD_ACCEPT | FD_READ | FD_WRITE ))) {
		printf("비동기 대기 중.. - Server \n");
	}

	if (listen(hServerSocket, 3) == SOCKET_ERROR) { // backlog - 동시 연결자 수
		printf("연결 요청 대기 오류! - Server \n");
	}
	szClientAddr = sizeof(hndlAddr);

	printf("%d 포트에서 서버가 대기중입니다.\n", GetDlgItemInt(hWnd, GetDlgCtrlID(hTxtBox11),NULL,NULL));
	//hHandleSocket = accept(hServerSocket, &hndlAddr, &szClientAddr);
	if (hHandleSocket == INVALID_SOCKET) {
		printf("연결 승인 오류! - Server \n");
	}
}

void InitclientSocket(HWND hWnd) {
	char strConnIP[20];
	int nConnPort;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("WsaData 생성 실패! - Server \n");
	}

	GetDlgItemText(hWnd, GetDlgCtrlID(hTxtBox10), strConnIP, sizeof(strConnIP));
	nConnPort = GetDlgItemInt(hWnd, GetDlgCtrlID(hTxtBox11), NULL, NULL);
	
	hClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hClientSocket == INVALID_SOCKET) {
		printf("소켓 초기화 오류! \n");
	}

	if (WSAAsyncSelect(hClientSocket, hWnd, WM_SOCKET1, (FD_CLOSE | FD_READ))) {
		printf("비동기 접속 실패! - Client \n");
	}

	struct hostent *host;
	if ((host = gethostbyname(strConnIP)) == NULL) {
		printf("호스트 로드 중 에러 발생! - Client \n");
	}

	memset(&dstAddr, 0, sizeof(dstAddr));
	dstAddr.sin_port = htons(nConnPort);
	dstAddr.sin_family = AF_INET;
	dstAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

	if (connect(hClientSocket, (LPSOCKADDR)(&dstAddr), sizeof(dstAddr)) == SOCKET_ERROR) {
		printf("%s, %d \n",strConnIP, ntohs(dstAddr.sin_port));
		//printf("소켓 연결 오류! - client \n");
	}
	else printf("Client Socket Init Complete.\n");
}

void sendMessage(SOCKET hSocket,char* Message, char* szSendTotalBuffer) {

	send(hSocket, Message, strlen(Message), 0);
	strcat(szSendTotalBuffer,Message);	
	
	SetWindowText(hTxtBox4, szSendTotalBuffer);

	SendMessageA(hTxtBox4, EM_SETSEL, 0, -1);
	SendMessageA(hTxtBox4, EM_SETSEL, -1, -1);
	SendMessageA(hTxtBox4, EM_SCROLLCARET, 0, 0);
}

