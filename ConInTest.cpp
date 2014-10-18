// ConInTest.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

#define TEXT_STR L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ|||"

int ReadFromFile()
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nRead = 0, nWrite = 0;
	char szLine[4096+1];

	SetConsoleMode(hIn, 1);

	GetConsoleMode(hIn, &nRead);
	GetConsoleMode(hOut, &nWrite);
	printf("Console modes: In=0x%02X, Out=0x%02X\n", nRead, nWrite);

	while (true)
	{
		SetConsoleTextAttribute(hOut, 0x07);
		BOOL bRead = ReadFile(hIn, szLine, ARRAYSIZE(szLine)-1, &nRead, NULL);
		szLine[nRead] = 0;
		for (DWORD i = 0; i < nRead; i++)
		{
			SetConsoleTextAttribute(hOut, szLine[i]==L'?' ? 0x0C : 0x07);
			WriteConsoleA(hOut, szLine+i, 1, &nWrite, NULL);
		}
		WriteConsoleA(hOut, "\n", 1, &nWrite, NULL);
	}

	return 1;
}

int ReadFromFileW()
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nRead = 0, nWrite = 0;
	wchar_t szLine[4096+1];

	SetConsoleMode(hIn, 1);

	GetConsoleMode(hIn, &nRead);
	GetConsoleMode(hOut, &nWrite);
	printf("Console modes: In=0x%02X, Out=0x%02X\n", nRead, nWrite);

	while (true)
	{
		SetConsoleTextAttribute(hOut, 0x07);
		BOOL bRead = ReadConsole(hIn, szLine, ARRAYSIZE(szLine)-1, &nRead, NULL);
		szLine[nRead] = 0;
		for (DWORD i = 0; i < nRead; i++)
		{
			SetConsoleTextAttribute(hOut, szLine[i]==L'?' ? 0x0C : 0x07);
			WriteConsoleW(hOut, szLine+i, 1, &nWrite, NULL);
		}
		WriteConsoleA(hOut, "\n", 1, &nWrite, NULL);
	}

	return 1;
}

int WriteStream()
{
	TCHAR szExe[MAX_PATH+16] = {}; GetModuleFileName(NULL, szExe, MAX_PATH);
	TCHAR* pszName = _tcsrchr(szExe, _T('\\')); *(pszName++) = 0;
	_tcscat(pszName, L" /INLINE");
	STARTUPINFO si = {sizeof(si)}; PROCESS_INFORMATION pi = {};
	BOOL bCreate = CreateProcess(NULL, pszName, NULL, NULL, TRUE, 0, NULL, szExe, &si, &pi);

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	wchar_t szText[] = TEXT_STR;
	const int nLen = lstrlen(szText);
	INPUT_RECORD* prc = new INPUT_RECORD[nLen*2];
	DWORD nWritten = 0, nWrite;

	SetConsoleTitle(_T("Writing to console input buffer"));

	for (int i = 0; i < nLen; i++)
	{
		wchar_t ch = szText[i];
		INPUT_RECORD rc = {KEY_EVENT};
		rc.Event.KeyEvent.bKeyDown = TRUE;
		rc.Event.KeyEvent.dwControlKeyState = 0;
		rc.Event.KeyEvent.uChar.UnicodeChar = ch;
		rc.Event.KeyEvent.wVirtualKeyCode = 0;
		rc.Event.KeyEvent.wVirtualScanCode = 0;
		prc[i*2] = rc;
		prc[i*2+1] = rc;
		prc[i*2+1].Event.KeyEvent.bKeyDown = FALSE;
	}

	int iAllWritten = 0;

	while (MessageBox(NULL, L"Press <Retry> to paste\n\n" TEXT_STR, L"ConInTest", MB_RETRYCANCEL|MB_SYSTEMMODAL) == IDRETRY)
	{
		nWritten = 0;
		//BOOL bWritten = WriteConsoleInputW(hIn, prc, nLen*2, &nWritten);
		for (int i = 0; i < nLen*2; i++)
		{
			nWrite = 0;
			if (WriteConsoleInputW(hIn, prc+i, 1, &nWrite))
				nWritten+=nWrite;
		}
		iAllWritten += nWritten;
		wsprintfW(szExe, L"Writing to console input buffer: %i events, %i in last step", iAllWritten, nWritten);
		SetConsoleTitle(szExe);
	}

	SetConsoleTitle(_T("Done"));
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int iRc = 1;
	if (argc < 2 || _tcsicmp(argv[1], _T("/FLOOD")) == 0)
		iRc = WriteStream();
	else if (_tcsicmp(argv[1], _T("/INLINE")) == 0)
		iRc = ReadFromFileW();
	return iRc;
}

