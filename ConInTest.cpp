// ConInTest.cpp : Defines the entry point for the console application.
//

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>

int help()
{
	return 1;
}

int ReadByChar()
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	INPUT_RECORD rc = {};
	DWORD nRead = 0, nWrite = 0;
	int iKeyDown = 0, iKeyUp = 0, iMouse = 0, iOther = 0;
	bool bLoop = true;

	while (bLoop)
	{
		if (ReadConsoleInputW(hIn, &rc, 1, &nRead) && nRead)
		{
			switch (rc.EventType)
			{
			case KEY_EVENT:
				if (rc.Event.KeyEvent.bKeyDown)
				{
					if (rc.Event.KeyEvent.uChar.UnicodeChar == 27)
					{
						bLoop = false;
						break;
					}
					iKeyDown++;
					if (rc.Event.KeyEvent.uChar.UnicodeChar == L'\r')
						rc.Event.KeyEvent.uChar.UnicodeChar = L'\n';
					WriteConsoleW(hOut, &rc.Event.KeyEvent.uChar.UnicodeChar, 1, &nWrite, NULL);
					//Sleep(1); // simulate small delay
				}
				else
				{
					iKeyUp++;
				}
				break;
			case MOUSE_EVENT:
				// Just skip them for now
				iMouse++;
				break;
			default:
				//wprintf(L"{{{%u}}}", rc.EventType);
				iOther++;
			}
		}
	}

	wprintf(L"\nStatistics: KeyDown %i, KeyUp %i, Mouse %i, Other %i\n", iKeyDown, iKeyUp, iMouse, iOther);

	return 1;
}

int ReadFromFile()
{
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD nRead = 0, nWrite = 0;
	char szLine[4096+1];

	SetConsoleMode(hIn, 1);

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
	}

	//wprintf(L"\nStatistics: KeyDown %i, KeyUp %i, Mouse %i, Other %i\n", iKeyDown, iKeyUp, iMouse, iOther);

	return 1;
}

int WriteStream(int iCharCount)
{
	TCHAR szExe[MAX_PATH+16] = {}; GetModuleFileName(NULL, szExe, MAX_PATH);
	TCHAR* pszName = _tcsrchr(szExe, _T('\\')); *(pszName++) = 0;
	//_tcscat(pszName, L" /INCHAR");
	_tcscat(pszName, L" /INLINE");
	STARTUPINFO si = {sizeof(si)}; PROCESS_INFORMATION pi = {};
	BOOL bCreate = CreateProcess(NULL, pszName, NULL, NULL, TRUE, 0, NULL, szExe, &si, &pi);

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	GetConsoleScreenBufferInfo(hOut, &csbi);
	int iWidth = csbi.dwSize.X; // csbi.srWindow.Right - csbi.srWindow.Left - 1;
	INPUT_RECORD* prc = new INPUT_RECORD[iWidth*2];
	DWORD nWritten = 0;

	SetConsoleTitle(_T("Writing to console input buffer"));

	int iAllWritten = 0;
	int iRow = 0;
	while (iCharCount > 0)
	{
		wchar_t ch = L'a' + ((iRow++) % 26);
		INPUT_RECORD rc = {KEY_EVENT};
		rc.Event.KeyEvent.bKeyDown = TRUE;
		rc.Event.KeyEvent.dwControlKeyState = 0;
		rc.Event.KeyEvent.uChar.UnicodeChar = ch;
		rc.Event.KeyEvent.wVirtualKeyCode = 0;
		rc.Event.KeyEvent.wVirtualScanCode = 0;
		for (int i = 0; i < iWidth; i++)
		{
			prc[i*2] = rc;
			prc[i*2+1] = rc;
			prc[i*2+1].Event.KeyEvent.bKeyDown = FALSE;			
		}
		//prc[iWidth*2] = rc;
		//prc[iWidth*2].Event.KeyEvent.uChar.UnicodeChar = L'\n';

		nWritten = 0;
		BOOL bWritten = WriteConsoleInputW(hIn, prc, iWidth*2+1, &nWritten);
		iAllWritten += nWritten;
		iCharCount -= nWritten;
		wsprintfW(szExe, L"Writing to console input buffer: %i events done, %i left", iAllWritten, iCharCount);
		SetConsoleTitleW(szExe);

		if (!bWritten || ((iWidth*2+1) != nWritten))
		{
			wsprintfW(szExe, L"<<<WriteConsoleInputW failed, was written %i events, code %u>>>", iAllWritten, GetLastError());
			SetConsoleTitleW(szExe);
			break;
		}
	}

	INPUT_RECORD rc = {KEY_EVENT};
	rc.Event.KeyEvent.bKeyDown = TRUE;
	rc.Event.KeyEvent.uChar.UnicodeChar = 27;
	WriteConsoleInputW(hIn, &rc, 1, &nWritten);

	SetConsoleTitle(_T("Done"));

	WaitForSingleObject(pi.hProcess, INFINITE);
	//wprintf(L"\nDone, press Enter to exit");
	//getchar();
	MessageBox(NULL, _T("Press OK to exit"), _T("ConInTest"), MB_OK);
	return 1;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int iRc = 1;
	TCHAR *pch;

	if (argc < 2 || _tcscmp(argv[1], _T("/?")) == 0)
		iRc = help();
	else if (_tcsicmp(argv[1], _T("/INCHAR")) == 0)
		iRc = ReadByChar();
	else if (_tcsicmp(argv[1], _T("/INLINE")) == 0)
		iRc = ReadFromFile();
	else if (_tcsicmp(argv[1], _T("/FLOOD")) == 0)
		iRc = WriteStream(argc>=3 ? _tcstol(argv[2], &pch, 10) : 8000);

	return iRc;
}

