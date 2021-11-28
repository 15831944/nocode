#pragma once

#pragma comment(lib,"dsound")

#include<windows.h>
//#include<dsound.h>

class sound
{
//	IDirectSound* ds;
//	IDirectSoundBuffer* dsb1, * dsb2;

public:
	sound(/*HWND hWnd*/)/*: ds(0)*/ {
		//DirectSoundCreate(NULL, &ds, NULL);
		//if (ds) {
		//	ds->SetCooperativeLevel(hWnd, DSSCL_NORMAL);
		//}
	}
	~sound() {
		//if (ds) {
		//	ds->Release();
		//}
		//ds = 0;
	}
	void play(LPCWSTR lpszFilePath) {

		PlaySound(lpszFilePath, NULL, SND_FILENAME | SND_ASYNC);

		//if (ds) {
		//	DSBUFFERDESC dsbd = { 0 };
		//	dsbd.dwSize = sizeof(DSBUFFERDESC);
		//	dsbd.dwFlags = DSBCAPS_STATIC | DSBCAPS_CTRLDEFAULT;
		//	...
		//}


	}
};

