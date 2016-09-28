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
#define _USE_MATH_DEFINES
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
int SongTime;
int OsuWindowX, OsuWindowY;
bool songStarted;
float CircleSize;
float StackOffset;
float StackLeniency;
float SliderMultiplier;
float OverallDifficulty;
float XMultiplier, YMultiplier;
string cmdName;						//String to set Console Title.

//winAPI stuff
HWND OsuWindow;
DWORD OsuProcessID;
LPVOID TimeAdress;
HANDLE OsuProcessHandle;

//Keyboard things
INPUT keys1, keys2, keys3, keys4;

///Temporary warning Disable.
#pragma warning (disable:4312)
#pragma optimize("", on)

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
		this_thread::sleep_for(chrono::microseconds(100));
	}
}

//Function to press the keys.
void keyPresses(bool key1, bool key2, bool key3, bool key4) {

	bool k1 = key1, k2 = key2;
	bool k3 = key3, k4 = key4;
	
	//Key 1 Press
	if (k1 == true)
	{
		printf("Hit! Z \n");
		keys1.ki.wVk = 0x5A; // Z
		keys1.ki.dwFlags = 0;
		SendInput(1, &keys1, sizeof(INPUT));
		this_thread::sleep_for(chrono::microseconds(1000));
	}

	//Release the keyboard keys.
	keys1.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &keys1, sizeof(INPUT));

	//Key 2 Press
	if (k2 == true)
	{
		printf("Hit! X \n");
		keys2.ki.wVk = 0x58; // X
		keys2.ki.dwFlags = 0;
		SendInput(1, &keys2, sizeof(INPUT));
		this_thread::sleep_for(chrono::microseconds(1000));
	}

	keys2.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &keys2, sizeof(INPUT));

	//Key 3 Press
	if (k3 == true)
	{
		printf("Hit! C \n");
		keys3.ki.wVk = 0x43; // C
		keys3.ki.dwFlags = 0;
		SendInput(1, &keys3, sizeof(INPUT));
		this_thread::sleep_for(chrono::microseconds(1000));
	}

	keys3.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &keys3, sizeof(INPUT));

	//Key 4 press
	if (k4 == true)
	{
		printf("Hit! V \n");
		keys4.ki.wVk = 0x56; // V
		keys4.ki.dwFlags = 0;
		SendInput(1, &keys4, sizeof(INPUT));
		this_thread::sleep_for(chrono::microseconds(1000));
	}

	keys4.ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(1, &keys4, sizeof(INPUT));
}

//Function that handles key data to keypresses. 
void ManiaKeys(const HitObject* object)
{
	//Output
	//cout << "Start Pos: " << object->getStartPosition().x << ", Start Time: " << object->getStartTime() << endl;
	
	while (SongTime < object->getStartTime() && songStarted)
	{
		auto a = static_cast<int>(object->getStartTime());

		//Bools for applicable keypresses
		bool key1 = false, key2 = false, key3 = false , key4 = false;

		//Key 1 Hit.
		if ((object->getStartPosition().x == 64) && (SongTime + 10 >= a)){
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

		//Sleep for 100 MicroSeconds.
		this_thread::sleep_for(chrono::microseconds(1));

		//Send keys off to another function.
		keyPresses(key1, key2, key3, key4);
		

	}

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

//Check game (Not fully understood yet).
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
		//int x = min(rect.right, GetSystemMetrics(SM_CXSCREEN));
		//int y = min(rect.bottom, GetSystemMetrics(SM_CYSCREEN));
		//int swidth = x;
		//int sheight = y;
		//if (swidth * 3 > sheight * 4) {
		//	swidth = sheight * 4 / 3;
		//}
		//else {
		//	sheight = swidth * 3 / 4;
		//}
		//XMultiplier = swidth / 640.0f;
		//YMultiplier = sheight / 480.0f;
		//auto xOffset = static_cast<int>(x - 512.0f * XMultiplier) / 2;
		//auto yOffset = static_cast<int>(y - 384.0f * YMultiplier) / 2;
		//
		//OsuWindowX = p.x + xOffset;
		//OsuWindowY = p.y + yOffset;
		const static auto Length = 256;
		char TitleC[256];
		GetWindowText(OsuWindow, TitleC, Length);
		auto Title = string(TitleC);
		if (Title != "osu!" && Title != "")
		{
			if (!songStarted)
			{
				songStarted = true;
				//thread Auto(AutoThread);
				//Auto.join();
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)AutoThread, 0, 0, NULL);



				
			}
		}
		else
		{
			if (Title == "")
			{
				TerminateProcess(GetCurrentProcess(), 0);
			}
			songStarted = false;
		}
		this_thread::sleep_for(chrono::microseconds(1));
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

		//Find [General]
		if (str.find("[General]") != string::npos)	{
			General = true;
		}
		else if (General){
			//Find StachLeniencey
			if (str.find("StackLeniency") != string::npos)
			{
				StackLeniency = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos)
			{
				General = false;
			}
		}

		//Find [Metadata]
		else if(str.find("[Metadata]") != string::npos)
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
			else if (str.find("CircleSize") != string::npos)
			{
				CircleSize = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find("SliderMultiplier") != string::npos)
			{
				SliderMultiplier = stof(str.substr(str.find(':') + 1));
			}
			else if (str.find(':') == string::npos)
			{
				Difficulty = false;
			}
		}

		//Find [TimingPoints]
		else if (str.find("[TimingPoints]") != string::npos) {
			Timing = true;
		}
		else if (Timing) {
			if (str.find(',') == string::npos){
				Timing = false;
			}
			else {
				TimingPoint TP = TimingPoint(str);
				TimingPoints.push_back(TP);
			}
		}
		
		//Find [HitObjects]
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
				HitObject HO = HitObject(str, &TimingPoints, SliderMultiplier);
				HitObjects.push_back(HO);
			}
		}
	}

	//Close file, we don't need it anymore.
	t.close();

	//Combine artist and title for a title.
	cmdName = artist + " - " + title;

	//float PreEmpt = mapDifficultyRange(OverallDifficulty, 1800.0f, 1200.0f, 450.0f);
	StackOffset = (512.0f / 16.0f) * (1.0f - 0.7f * (CircleSize - 5.0f) / 5.0f) / 10.0f;

	///Dont need to display this.
	//cout << StackOffset << endl;

	const int STACK_LENIENCE = 3;
	for (int i = HitObjects.size() - 1; i > 0; i--)
	{
		int n = i;
		/* We should check every note which has not yet got a stack.
		* Consider the case we have two interwound stacks and this will make sense.
		*
		* o <-1      o <-2
		*  o <-3      o <-4
		*
		* We first process starting from 4 and handle 2,
		* then we come backwards on the i loop iteration until we reach 3 and handle 1.
		* 2 and 1 will be ignored in the i loop because they already have a stack value.
		*/

		HitObject *objectI = &HitObjects[i];

		if (objectI->stackId != 0 || objectI->itSpinner()) continue;

		/* If this object is a hitcircle, then we enter this "special" case.
		* It either ends with a stack of hitcircles only, or a stack of hitcircles that are underneath a slider.
		* Any other case is handled by the "is Slider" code below this.
		*/
		if (objectI->endTime == 0)
		{
			while (--n >= 0)
			{
				HitObject* objectN = &HitObjects[n];

				if (objectN->itSpinner()) continue;

				//HitObjectSpannable spanN = objectN as HitObjectSpannable;
				//float timeI = objectI->startTime - PreEmpt * StackLeniency;
				float timeN = static_cast<float>(objectN->itSlider() ? objectN->endTime : objectN->startTime);
				//if (timeI > timeN)
				//	break;

				/* This is a special case where hticircles are moved DOWN and RIGHT (negative stacking) if they are under the *last* slider in a stacked pattern.
				*    o==o <- slider is at original location
				*        o <- hitCircle has stack of -1
				*         o <- hitCircle has stack of -2
				*/
				if (objectN->endTime != 0 && distance(objectN->getEndPos(), objectI->startPosition) < STACK_LENIENCE)
				{
					int offset = objectI->stackId - objectN->stackId + 1;
					for (int j = n + 1; j <= i; j++)
					{
						//For each object which was declared under this slider, we will offset it to appear *below* the slider end (rather than above).
						if (distance(objectN->getEndPos(), HitObjects[j].startPosition) < STACK_LENIENCE)
							HitObjects[j].stackId -= offset;
					}

					//We have hit a slider.  We should restart calculation using this as the new base.
					//Breaking here will mean that the slider still has StackCount of 0, so will be handled in the i-outer-loop.
					break;
				}

				if (distance(objectN->startPosition, objectI->startPosition) < STACK_LENIENCE)
				{
					//Keep processing as if there are no sliders.  If we come across a slider, this gets cancelled out.
					//NOTE: Sliders with start positions stacking are a special case that is also handled here.

					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
		else if (objectI->itSlider())
		{
			/* We have hit the first slider in a possible stack.
			* From this point on, we ALWAYS stack positive regardless.
			*/
			while (--n >= 0)
			{
				HitObject* objectN = &HitObjects[n];

				if (objectN->itSpinner()) continue;

				//if (objectI->startTime - (PreEmpt * StackLeniency) > objectN->startTime)
				//	break;

				if (distance((objectN->endTime != 0 ? objectN->getEndPos() : objectN->startPosition), objectI->startPosition) < STACK_LENIENCE)
				{
					objectN->stackId = objectI->stackId + 1;
					objectI = objectN;
				}
			}
		}
	}
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
		ParseSong(ofn.lpstrFile);
	}
}

//Attempt to flush variables, so that the program shouldn't be restarted.
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

//Main Function, the Begining.
int main()
{
redo:
	setKeyboard();
	OpenSong();		//Open Dialog to choose song.
	checkGame();	//Check for osu! application.
	getchar();		//Wait for user to press Enter to restart program.
	flushMe();		//Flush variables, soft restart.
	goto redo;
	return 0;
}

//TODO: 2. Give program more FPS, or find a way for more CPU. (In the Works)
//TODO: 3. Handles multiple keypresses better. 
//TODO: 4. Clean unused code for this version of bot. (50% done)