// ConInTest.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <thread>

#undef USE_THREAD

#define TEXT_STR L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ|||"

int ReadFromFileW()
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nRead = 0, nWrite = 0;
	DWORD nGood = 0, nFail = 0;
	wchar_t szLine[4096+1];

	SetConsoleMode(hIn, 1);

	GetConsoleMode(hIn, &nRead);
	GetConsoleMode(hOut, &nWrite);
	printf("Console modes: In=0x%02X, Out=0x%02X\n", nRead, nWrite);

	while (true)
	{
		SetConsoleTextAttribute(hOut, 0x07);
		nRead = 0;
		DWORD nBar = 0;
		for (size_t i = 0; i < (ARRAYSIZE(szLine)-1); i++)
		{
			DWORD nChar = 0;
			if (ReadConsole(hIn, szLine+i, 1, &nChar, NULL) && nChar)
			{
				if (szLine[i] == L'\n')
					break;
				nRead++;
				if (szLine[i] == L'|')
				{
					nBar++;
					if (nBar == 3)
						break;
				}
			}
		}
		szLine[nRead] = 0;
		bool bWriteInput = true;

		#ifdef USE_STATISTICS
		if (wcscmp(TEXT_STR, szLine) == 0)
		{
			nGood++;
			printf("\rGood: %u; Fail: %u;", nGood, nFail);
			bWriteInput = false;
		}
		else
		{
			nFail++;
			printf("\rGood: %u; Fail: %u;\n", nGood, nFail);
		}
		#endif

		if (bWriteInput)
		{
			for (DWORD i = 0; i < nRead; i++)
			{
				SetConsoleTextAttribute(hOut, szLine[i]<'0'||szLine[i]>'|' ? 0xC0 : 0x07);
				WriteConsoleW(hOut, szLine+i, 1, &nWrite, NULL);
			}
			WriteConsoleA(hOut, "\n", 1, &nWrite, NULL);
		}
	}

	return 1;
}

int WriteStream(int iSleep)
{
	#ifndef USE_THREAD
	TCHAR szExe[MAX_PATH+16] = {}; GetModuleFileName(NULL, szExe, MAX_PATH);
	TCHAR* pszName = _tcsrchr(szExe, _T('\\')); *(pszName++) = 0;
	_tcscat(pszName, L" /INLINE");
	STARTUPINFO si = {sizeof(si)}; PROCESS_INFORMATION pi = {};
	BOOL bCreate = CreateProcess(NULL, pszName, NULL, NULL, TRUE, 0, NULL, szExe, &si, &pi);
	#else
	std::thread reader(ReadFromFileW);
	#endif

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleMode(hIn, 1);

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
		if (ch == L'\n')
		{
			rc.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
		}
		prc[i*2] = rc;
		prc[i*2+1] = rc;
		prc[i*2+1].Event.KeyEvent.bKeyDown = FALSE;
	}

	int iAllWritten = 0;

	Sleep(25);

	while (true)
	{
		nWritten = 0;
		int nCount = nLen*2;
		int nStep = 200;
		for (int i = 0; i < nCount; i+=nStep)
		{
			nWrite = min(nCount-i,nStep);
			if (WriteConsoleInputW(hIn, prc+i, nWrite, &nWrite))
				nWritten+=nWrite;
			if (iSleep > 0)
				Sleep(iSleep);
		}
		iAllWritten += nWritten;

		Sleep(25);

		wchar_t szInfo[200];
		swprintf_s(szInfo, L"Writing to console input buffer: %i events, %i in last step", iAllWritten, nWritten);
		SetConsoleTitle(szInfo);

		int nBtn = IDRETRY;
		//nBtn = MessageBox(NULL, L"Press <Retry> to paste\n\n" TEXT_STR, L"ConInTest", MB_RETRYCANCEL|MB_SYSTEMMODAL);
		if (nBtn != IDRETRY)
			break;
	}

	#ifndef USE_THREAD
	TerminateProcess(pi.hProcess, 100);
	#endif

	SetConsoleTitle(_T("Done"));
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc >= 2 && _tcsicmp(argv[1], _T("/INLINE")) == 0)
		return ReadFromFileW();
	int iSleep = 50; TCHAR* pch;
	if (argc >= 2 && isdigit(argv[1][0]))
		iSleep = _tcstol(argv[1], &pch, 10);
	return WriteStream(iSleep);
}

