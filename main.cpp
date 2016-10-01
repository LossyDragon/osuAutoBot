/*
osu!AutoBot
Copyright (C) 2016  Andrey Tokarev

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

//Includes.
#include <cmath>
#include <iostream>
#include "HitObject.h"
#include <Windows.h>
#include <fstream>
#include <thread>
#include <TlHelp32.h>
#include <psapi.h>
#include <limits>

//Vectors.
vector<TimingPoint> TimingPoints;
vector<HitObject> HitObjects;

//Variables.
int timerr = NULL;
int SongTime;
int OsuWindowX, OsuWindowY;
bool songStarted;
float CircleSize;
float StackOffset;
float StackLeniency;
float SliderMultiplier;
float OverallDifficulty;
float XMultiplier, YMultiplier;
string cmdName;

//winAPI stuff
HWND OsuWindow;
DWORD OsuProcessID;
LPVOID TimeAdress;
HANDLE OsuProcessHandle, hConsole;

//Keyboard things
INPUT keys1, keys2, keys3, keys4;

///Temporary warning Disable.
#pragma warning (disable:4312)

//Get Base Address.
LPVOID GetBaseAddress(HANDLE hProc)
{
	MODULEINFO miInfo;
	if (GetModuleInformation(hProc, nullptr, &miInfo, sizeof(miInfo)))
		return miInfo.EntryPoint;
	return nullptr;
}

//Get Memory Size.
DWORD GetMemorySize(HANDLE hProc)
{
	PROCESS_MEMORY_COUNTERS pmcInfo;
	if (GetProcessMemoryInfo(hProc, &pmcInfo, sizeof(pmcInfo)))
		return static_cast<DWORD>(pmcInfo.WorkingSetSize);
	return 0;
}

//Find Pattern.
DWORD FindPattern(HANDLE ProcessHandle, const byte Signature[], unsigned const int ByteCount) {
	const static unsigned int Mult = 4096;
	const static unsigned int StartAdress = reinterpret_cast<LONG_PTR>(GetBaseAddress(OsuProcessHandle));
	const static unsigned int EndAdress = StartAdress + GetMemorySize(OsuProcessHandle);
	static bool Hit;

	byte ByteInMemory[Mult];

	for (auto i = StartAdress; i < EndAdress; i += Mult - ByteCount) {
		ReadProcessMemory(ProcessHandle, reinterpret_cast<LPVOID>(i), &ByteInMemory, Mult, nullptr);
		for (unsigned int a = 0; a < Mult; a++) {
			Hit = true;

			for (unsigned int j = 0; j < ByteCount && Hit; j++) {
				if (ByteInMemory[a + j] != Signature[j] && Signature[j] != 0x00) Hit = false;
			}
			if (Hit) {
				return i + a;
			}
		}
	}
	return 0;
}

//Function to get P.I.D. for osu.exe
DWORD getProcessID()
{
	auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
		return 0;
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof pe;
	if (Process32First(hSnapshot, &pe))
	{
		do
		{
			if (string(pe.szExeFile) == "osu!.exe")
			{
				CloseHandle(hSnapshot);
				return pe.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &pe));
	}
	CloseHandle(hSnapshot);
	return 0;
}

//Time Thread.
void timeThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while (true)
	{
		ReadProcessMemory(OsuProcessHandle, TimeAdress, &SongTime, 4, nullptr);
		this_thread::sleep_for(chrono::microseconds(100));	//Micronaps
	}
}

//Keys 1&2, 3&4 threads.
//void k1_2(bool k1, bool k2) {
//	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
//	if (k1) {
//		//printf("Hit! Z \n");
//		keys1.ki.wVk = 0x5A; // Z
//		keys1.ki.dwFlags = 0;
//		SendInput(1, &keys1, sizeof(INPUT));
//		this_thread::sleep_for(chrono::microseconds(600));	//Micronaps
//		keys1.ki.dwFlags = KEYEVENTF_KEYUP;
//		SendInput(1, &keys1, sizeof(INPUT));
//	}
//	if (k2) {
//		//printf("Hit! X \n");
//		keys2.ki.wVk = 0x58; // X
//		keys2.ki.dwFlags = 0;
//		SendInput(1, &keys2, sizeof(INPUT));
//		this_thread::sleep_for(chrono::microseconds(600));	//Micronaps
//		keys2.ki.dwFlags = KEYEVENTF_KEYUP;
//		SendInput(1, &keys2, sizeof(INPUT));
//	}
//}


void k1_2(bool k1, bool k2) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	INPUT in;
	vector<WORD> inputs;

	if (k1)
		inputs.emplace_back('Z');
	if (k2)
		inputs.emplace_back('X');

	for (auto input : inputs) {
		//Debug output
		in.type = INPUT_KEYBOARD;
		in.ki.wScan = 0;
		in.ki.wVk = input;
		in.ki.time = 0;
		in.ki.dwExtraInfo = 0;
		in.ki.dwFlags = 0;
		SendInput(1, &in, sizeof(in));
	}
	this_thread::sleep_for(chrono::microseconds(600));

	for (auto input : inputs) {
		in.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &in, sizeof(in));
	}
}

void k3_4(bool k3, bool k4) {
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	if (k3) {
		//printf("Hit! C \n");
		keys3.ki.wVk = 0x43; // C
		keys3.ki.dwFlags = 0;
		SendInput(1, &keys3, sizeof(INPUT));
		this_thread::sleep_for(chrono::microseconds(600));	//Micronaps
		keys3.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &keys3, sizeof(INPUT));
	}
	if (k4) {
		//printf("Hit! V \n");
		keys4.ki.wVk = 0x56; // V
		keys4.ki.dwFlags = 0;
		SendInput(1, &keys4, sizeof(INPUT));
		this_thread::sleep_for(chrono::microseconds(600));	//Micronaps
		keys4.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &keys4, sizeof(INPUT));
	}
}

//Function that handles key data to keypresses. 
void ManiaKeys(const HitObject* object)
{
	//Bools for applicable keypresses
	bool key1 = false, key2 = false, key3 = false, key4 = false;
	
	while (SongTime < object->getStartTime() && songStarted)
	{
		auto a = static_cast<int>(object->getStartTime());
		//Key 1 Hit.
		if ((object->getStartPosition().x == 64) && (SongTime + 10 >= a)) {
			key1 = true;
		}
		//Key 2 Hit.
		if ((object->getStartPosition().x == 192) && (SongTime + 10 >= a)) {
			key2 = true;
		}
		//Key 3 Hit.
		if ((object->getStartPosition().x == 320) && (SongTime + 10 >= a)) {
			key3 = true;
		}
		//Key 4 Hit.
		if ((object->getStartPosition().x == 448) && (SongTime + 10 >= a)) {
			key4 = true;
		}

		//Micronaps
		this_thread::sleep_for(chrono::microseconds(250));

		//TODO: Better simultaneous keypresses.
		//TODO: Single keys actually hit 4-times. Fix
		if (key1 || key2) {
			thread keys1_2(k1_2, key1, key2);
			keys1_2.detach();
		}
		if (key3 || key4) {
			thread keys3_4(k3_4,key3, key4);
			keys3_4.detach();
		}
	}
	//Output
	///printf("K1 %s\n", key1 ? "true" : "false");
	///printf("K2 %s\n", key2 ? "true" : "false");
	///printf("K3 %s\n", key3 ? "true" : "false");
	///printf("K4 %s\n", key3 ? "true" : "false");
}

//Auto Thread.
void AutoThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	for (auto Hit : HitObjects)
	{
		ManiaKeys(&Hit);

		if (!songStarted)
		{
			printf("Song not started or was terminated!\n");
			return;
		}
	}
}

//Game checker.
void gameCheckerThread()
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	LPSTR s = const_cast<char *>(cmdName.c_str());	//Convert string to LPSTR for SetConsoleTitle().
	SetConsoleTitle(s);	//Set window title of the song's name

	while (true)
	{
		RECT rect;
		POINT p; p.x = 0; p.y = 8;
		GetClientRect(OsuWindow, &rect);
		ClientToScreen(OsuWindow, &p);
		const static auto Length = 256;
		char TitleC[256];
		GetWindowText(OsuWindow, TitleC, Length);
		auto Title = string(TitleC);
		if (Title != "osu!" && Title != "")
		{
			if (!songStarted)
			{
				songStarted = true;
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AutoThread, 0, 0, NULL);		
			}
		}
		else
		{
			if (Title == "")
				TerminateProcess(GetCurrentProcess(), 0);

			songStarted = false;
		}
		this_thread::sleep_for(chrono::microseconds(1));	//Micronaps
	}
}

//Check game
void checkGame()
{
	printf("Searching \"osu!\" window\n");

	OsuWindow = FindWindowA(nullptr, TEXT("osu!"));

	if (OsuWindow == nullptr) {
		printf("Please run osu!\n");
		while (OsuWindow == nullptr) {
			OsuWindow = FindWindowA(nullptr, TEXT("osu!"));
			Sleep(100);
		}
	}

	//Output
	printf("osu! found\n");

	OsuProcessID = getProcessID();
	OsuProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, OsuProcessID);
	BYTE Pattern[] = { 0xA3, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x00, 0xA1, 0x00, 0x00, 0x00, 0x00, 0xA3 };
	const int PatternLength = sizeof Pattern / sizeof BYTE;
	DWORD ScanAdress = FindPattern(OsuProcessHandle, Pattern, PatternLength) + 1;

	if (ScanAdress == 1) {
		TimeAdress = nullptr;
		printf("Error in \"Timer Find\"\n");
		CloseHandle(OsuProcessHandle);
		Sleep(1500); //1.5 sec delay to recursion.
		checkGame();
	}

	int timerr;
	ReadProcessMemory(OsuProcessHandle, reinterpret_cast<LPCVOID>(ScanAdress), &timerr, 4, nullptr);
	TimeAdress = reinterpret_cast<LPVOID>(timerr + 0xC);

	//Output
	printf("Timer address: %p \n", TimeAdress);
	printf("Starting threads.\n");

	//Send off to threads.
	thread game(gameCheckerThread);
	game.detach();
	thread Timer(timeThread);
	Timer.detach();
}

//Load song data into variables/vectors.
void ParseSong(string path)
{
	ifstream t(path);
	string artist, title;
	bool General = false, Difficulty = false;
	bool Timing = false, Hits = false;
	bool Metadata = false;

	while (t)
	{
		string str;
		getline(t, str);

		//Find [Metadata]
		if (str.find("[Metadata]") != string::npos)
		{
			Metadata = true;
		}
		else if (Metadata) {
			//Find Artist Name.
			if (str.find("Artist") != string::npos) {
				artist = str.substr(str.find(':') + 1);
			}
			//Find Title Name.
			else if (str.find("Title") != string::npos) {
				title = str.substr(str.find(':') + 1);
			}
			else if (str.find(':') == string::npos)
			{
				Metadata = false;
			}
		}

		//Find [Difficulty]
		else if (str.find("[Difficulty]") != string::npos){
			Difficulty = true;
		}
		else if (Difficulty){
			if (str.find("OverallDifficulty:") != string::npos) {
				OverallDifficulty = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos)
			{
				Difficulty = false;
			}
		}

		//Find [TimingPoints]
		//else if (str.find("[TimingPoints]") != string::npos) {
		//	Timing = true;
		//}
		//else if (Timing) {
		//	if (str.find(',') == string::npos) {
		//		Timing = false;
		//	}
		//	else {
		//		TimingPoint TP = TimingPoint(str);
		//		TimingPoints.push_back(TP);
		//	}
		//}

		//Find [HitObjects], we mostly need this.
		else if (str.find("[HitObjects]") != string::npos)
		{
			Hits = true;
		}
		else if (Hits)
		{
			if (str.find(',') == string::npos)
			{
				Hits = false;
			}
			else
			{
				HitObject HO = HitObject(str, &TimingPoints);
				HitObjects.push_back(HO);
			}
		}
	}

	//Close file, we don't need it anymore.
	t.close();

	//Combine artist and title for a title.
	cmdName = artist + " - " + title;
}

//Opens Dialog to choose .osu file.
void OpenSong()
{
	OPENFILENAME ofn;				// common dialog box structure
	char szFile[260];				// buffer for file name
	HWND hwnd = nullptr;            // owner window
	ZeroMemory(&ofn, sizeof ofn);
	ofn.lStructSize = sizeof ofn;
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "osu! map\0*.osu\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = nullptr;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = nullptr;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn)) {
		ParseSong(ofn.lpstrFile); //Parse song data.
	}
}

//Flush variables, so that the program shouldn't be restarted.
void flushMe()
{
	TimingPoints.clear();				//Vector Clear.
	HitObjects.clear();					//Vector Clear. 
	SongTime = NULL;					//
	CircleSize = NULL;					//
	StackOffset = NULL;					//
	songStarted = NULL;					//
	StackLeniency = NULL;				//
	SliderMultiplier = NULL;			//
	OverallDifficulty = NULL;			//
	OsuWindowX, OsuWindowY = NULL;		//
	XMultiplier, YMultiplier = NULL;	//
}

//Setup keyboard thingies before song execution. 
void setKeyboard() {
	keys1.type = INPUT_KEYBOARD;
	keys1.ki.wScan = 0;
	keys1.ki.time = 0;
	keys1.ki.dwExtraInfo = 0;

	keys2.type = INPUT_KEYBOARD;
	keys2.ki.wScan = 0;
	keys2.ki.time = 0;
	keys2.ki.dwExtraInfo = 0;

	keys3.type = INPUT_KEYBOARD;
	keys3.ki.wScan = 0;
	keys3.ki.time = 0;
	keys3.ki.dwExtraInfo = 0;

	keys4.type = INPUT_KEYBOARD;
	keys4.ki.wScan = 0;
	keys4.ki.time = 0;
	keys4.ki.dwExtraInfo = 0;
}

//Function for prerequisites.
void description(){
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE); //Change color of text.
	SetConsoleTextAttribute(hConsole, 12); //RED
	printf("4K Mania only!\n");
	printf("ScreenRecording may lower accuracy.\n");
	printf("Use in offline mode. Do not use to Cheat!\n");
	SetConsoleTextAttribute(hConsole, 7); //Back to white.
	Sleep(1500);
}

//Main Function, the Begining.
int main()
{
	description();	//Prerequisite.
	redo:			//Lazy Recursion.
		setKeyboard();	//Initialize keyboard control.
		OpenSong();		//Open Dialog, choose song.
		system("CLS");	//Cleanup description();
		checkGame();	//Check for osu! application.
		getchar();		//Wait for user to press Enter to restart program.
		flushMe();		//Flush variables, soft restart.
	goto redo;		//Goto.
	return 0;
}

//TODO: 2. Give program more FPS, or find a way for more CPU. (In the Works)
//TODO: 4. Clean unused code for this version of bot. (Yeah, yeah~)
//TODO: LongNotes...