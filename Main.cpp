/************************************************************
* -- NIE USUWAJ TEJ INFORMACJI Z PROGRAMU ---------------- *
************************************************************
* -- Program powsta³ na bazie kodu Ÿród³owego ------------ *
* -- udostêpnionego studentom na potrzeby przedmiotu ----- *
* -- Programowanie Interfejsu U¿ytkownika ---------------- *
* -- Copyright (c) 2010 Politechnika Œl¹ska w Gliwicach -- *
* -- Rados³aw Sokó³, Wydzia³ Elektryczny ----------------- *
************************************************************/

#include <assert.h>
#include <windows.h>
#include <stdio.h>
#include <exception>
#include <time.h>
#include <omp.h>
#include <vector>
#include <iostream>
#include <string>
#include <conio.h>


TCHAR NazwaAplikacji[] = TEXT("Przetwarzanie obrazu");
TCHAR NazwaKlasy[] = TEXT("OKNOGLOWNE");

char *Rysunek = 0;
char *Filtrowany = 0;
DWORD RozmiarRysunku = 0;
unsigned char NrFiltru = 1;
double CzasFiltrowania = 0.0;

struct RGB { unsigned char B, G, R; };

void filtrSzary(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE);
void filtrPodkreslenie(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE);
void filtrRozmycie(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE);
void filtrProgujacy(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE);


inline unsigned int RoundUp4(const unsigned int i)
{
	return (i + 3)&(~3);
}

static void AplikujFiltr()
{
	assert(sizeof(RGB) == 3);
	BITMAPFILEHEADER *bfh = reinterpret_cast<BITMAPFILEHEADER*>(Filtrowany);
	BITMAPINFO *bi = reinterpret_cast<BITMAPINFO*>(Filtrowany + sizeof(BITMAPFILEHEADER));
	if ((bi->bmiHeader.biBitCount != 24) && (bi->bmiHeader.biBitCount != 32)) throw std::exception();
	RGB *Raster = reinterpret_cast<RGB*>(Filtrowany + bfh->bfOffBits);
	RGB *Punkt = NULL;
	register int Wiersz, Kolumna;
	switch (NrFiltru) {
		/*
	case 1:
		for (Wiersz = 0; Wiersz < bi->bmiHeader.biHeight; ++Wiersz) {
			Punkt = Raster + RoundUp4(Wiersz * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);
			for (Kolumna = 0; Kolumna < bi->bmiHeader.biWidth; ++Kolumna) {
				Punkt->R = 0;
				++Punkt;
			}
		}
		break;
		*/
	case 2:
		filtrSzary(Punkt, Raster, 0, bi->bmiHeader.biHeight, 0, bi->bmiHeader.biWidth);
		break;
	case 3:
		filtrPodkreslenie(Punkt, Raster, 0, bi->bmiHeader.biHeight, 0, bi->bmiHeader.biWidth);
		break;
	case 4:
		filtrProgujacy(Punkt, Raster, 0, bi->bmiHeader.biHeight, 0, bi->bmiHeader.biWidth);
		break;
	case 5:
		filtrRozmycie(Punkt, Raster, 0, bi->bmiHeader.biHeight, 0, bi->bmiHeader.biWidth);
		break;
	case 6:
#pragma omp parallel for  private(Wiersz, Kolumna,Punkt) shared(bi,Raster) num_threads(4)
		for (Wiersz = 0; Wiersz < bi->bmiHeader.biHeight; ++Wiersz) {
			Punkt = Raster + RoundUp4(Wiersz * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);
			for (Kolumna = 0; Kolumna < bi->bmiHeader.biWidth; ++Kolumna) {
				Punkt->R = 0;
				++Punkt;
			}
		}
		break;
	case 7:
		unsigned char gray;
#pragma omp parallel for private(Wiersz, Kolumna,Punkt, gray) shared(bi,Raster) num_threads(24) 
		for (Wiersz = 0; Wiersz < bi->bmiHeader.biHeight; ++Wiersz) {
			Punkt = Raster + RoundUp4(Wiersz * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);
			for (Kolumna = 0; Kolumna < bi->bmiHeader.biWidth; ++Kolumna) {
				gray = (Punkt->R + Punkt->G + Punkt->B) / 3;
				Punkt->R = gray;
				Punkt->G = gray;
				Punkt->B = gray;
				++Punkt;
			}
		}
		break;
	case 8:
		//unsigned char max;
#pragma omp parallel for private(Wiersz, Kolumna,Punkt) shared(bi,Raster) num_threads(4)
		for (Wiersz = 0; Wiersz < bi->bmiHeader.biHeight; ++Wiersz) {
			Punkt = Raster + RoundUp4(Wiersz * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);
			for (Kolumna = 0; Kolumna < bi->bmiHeader.biWidth; ++Kolumna) {
				if (Punkt->R >= Punkt->G && Punkt->R >= Punkt->B) {
					Punkt->R = 255;
				}
				else if (Punkt->B >= Punkt->G && Punkt->B >= Punkt->R) {
					Punkt->B = 255;
				}
				else if (Punkt->G >= Punkt->R && Punkt->G >= Punkt->B) {
					Punkt->G = 255;
				}
				++Punkt;
			}
		}
		break;
	case 9: {
		unsigned char sum = 0;
		unsigned char threshold = 136;
#pragma omp parallel for private(Wiersz, Kolumna,Punkt, sum) shared(bi,Raster) num_threads(4)
		for (Wiersz = 0; Wiersz < bi->bmiHeader.biHeight; ++Wiersz) {
			Punkt = Raster + RoundUp4(Wiersz * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);
			for (Kolumna = 0; Kolumna < bi->bmiHeader.biWidth; ++Kolumna) {
				sum = (Punkt->R + Punkt->G + Punkt->B) / 3;
				if (sum >= threshold) {
					Punkt->R = 0;
					Punkt->G = 0;
					Punkt->B = 0;
				}
				else {
					Punkt->R = 255;
					Punkt->G = 255;
					Punkt->B = 255;
				}
				++Punkt;
			}
		}

	}
			break;
	case 1: {
		unsigned int sumaR = 0, sumaG = 0, sumaB = 0;
		RGB *PunktPlusOne = NULL, *PunktMinusOne = NULL;
		RGB srednia;
		RGB *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
#pragma omp parallel for private(Wiersz, Kolumna,Punkt,PunktMinusOne,PunktPlusOne,p1,p2,p3,p4,p5,p6,p7,p8,sumaR,sumaG,sumaB,srednia) shared(bi,Raster) num_threads(4)
		for (Wiersz = 1; Wiersz < bi->bmiHeader.biHeight - 1; Wiersz += 2) {
			Punkt = Raster + RoundUp4(Wiersz * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);
			if (Wiersz > 0)
				PunktMinusOne = Raster + RoundUp4((Wiersz - 1) * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);
			if (Wiersz < bi->bmiHeader.biHeight)
				PunktPlusOne = Raster + RoundUp4((Wiersz + 1) * bi->bmiHeader.biWidth * sizeof(RGB)) / sizeof(RGB);

			for (Kolumna = 1; Kolumna < bi->bmiHeader.biWidth - 1; Kolumna += 2) {

				p1 = Punkt + 1;
				p2 = Punkt - 1;
				p3 = PunktMinusOne + 1;
				p4 = PunktMinusOne - 1;
				p5 = PunktPlusOne + 1;
				p6 = PunktPlusOne - 1;
				p7 = PunktPlusOne;
				p8 = PunktMinusOne;

				sumaR = (p1->R + p2->R + p3->R + p4->R + p5->R + p6->R + p7->R + p8->R);
				sumaG = (p1->G + p2->G + p3->G + p4->G + p5->G + p6->G + p7->G + p8->G);
				sumaB = (p1->B + p2->B + p3->B + p4->B + p5->B + p6->B + p7->B + p8->B);

				srednia.R = sumaR / 8;
				srednia.G = sumaG / 8;
				srednia.B = sumaB / 8;

				Punkt->R = srednia.R;
				Punkt->G = srednia.G;
				Punkt->B = srednia.B;
				PunktMinusOne->R = srednia.R;
				PunktMinusOne->G = srednia.G;
				PunktMinusOne->B = srednia.B;
				PunktPlusOne->R = srednia.R;
				PunktPlusOne->G = srednia.G;
				PunktPlusOne->B = srednia.B;
				Punkt += 2;
				if (PunktMinusOne != NULL)
					PunktMinusOne += 2;
				if (PunktPlusOne != NULL)
					PunktPlusOne += 2;
				sumaR = 0;
				sumaG = 0;
				sumaB = 0;
			}
		}
	}
			break;
	}
}
void filtrSzary(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE) {
	register int Wiersz, Kolumna;
	unsigned char gray;
	for (Wiersz = hS; Wiersz < hE; ++Wiersz) {
		Punkt = Raster + RoundUp4(Wiersz * wE * sizeof(RGB)) / sizeof(RGB);
		for (Kolumna = wS; Kolumna < wE; ++Kolumna) {
			gray = (Punkt->R + Punkt->G + Punkt->B) / 3;
			Punkt->R = gray;
			Punkt->G = gray;
			Punkt->B = gray;
			++Punkt;
		}
	}
}
void filtrPodkreslenie(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE) {
	register int Wiersz, Kolumna;
	unsigned char max = 0;
	for (Wiersz = hS; Wiersz < hE; ++Wiersz) {
		Punkt = Raster + RoundUp4(Wiersz * wE * sizeof(RGB)) / sizeof(RGB);
		for (Kolumna = wS; Kolumna < wE; ++Kolumna) {
			if (Punkt->R >= Punkt->G && Punkt->R >= Punkt->B) {
				Punkt->R = 255;
			}
			else if (Punkt->B >= Punkt->G && Punkt->B >= Punkt->R) {
				Punkt->B = 255;
			}
			else if (Punkt->G >= Punkt->R && Punkt->G >= Punkt->B) {
				Punkt->G = 255;
			}
			++Punkt;
		}
	}
}

void filtrProgujacy(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE) {
	register int Wiersz, Kolumna;
	unsigned char sum = 0;
	unsigned char threshold = 136;
	for (Wiersz = hS; Wiersz < hE; ++Wiersz) {
		Punkt = Raster + RoundUp4(Wiersz * wE * sizeof(RGB)) / sizeof(RGB);
		for (Kolumna = wS; Kolumna < wE; ++Kolumna) {
			sum = (Punkt->R + Punkt->G + Punkt->B) / 3;
			if (sum >= threshold) {
				Punkt->R = 0;
				Punkt->G = 0;
				Punkt->B = 0;
			}
			else {
				Punkt->R = 255;
				Punkt->G = 255;
				Punkt->B = 255;
			}
			++Punkt;
		}

	}
}

void filtrRozmycie(RGB *Punkt, RGB *Raster, int hS, int hE, int wS, int wE) {
	int Wiersz, Kolumna;
	unsigned int sumaR = 0, sumaG = 0, sumaB = 0;
	RGB *PunktPlusOne = NULL, *PunktMinusOne = NULL;
	RGB srednia;
	RGB *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8;
	for (Wiersz = hS + 1; Wiersz < hE - 1; Wiersz += 2) {
		Punkt = Raster + RoundUp4(Wiersz * wE * sizeof(RGB)) / sizeof(RGB);
		if (Wiersz > 0)
			PunktMinusOne = Raster + RoundUp4((Wiersz - 1) * wE * sizeof(RGB)) / sizeof(RGB);
		if (Wiersz < hE)
			PunktPlusOne = Raster + RoundUp4((Wiersz + 1) * wE * sizeof(RGB)) / sizeof(RGB);

		for (Kolumna = wS + 1; Kolumna < wE - 1; Kolumna += 2) {

			p1 = Punkt + 1;
			p2 = Punkt - 1;
			p3 = PunktMinusOne + 1;
			p4 = PunktMinusOne - 1;
			p5 = PunktPlusOne + 1;
			p6 = PunktPlusOne - 1;
			p7 = PunktPlusOne;
			p8 = PunktMinusOne;

			sumaR = (p1->R + p2->R + p3->R + p4->R + p5->R + p6->R + p7->R + p8->R);
			sumaG = (p1->G + p2->G + p3->G + p4->G + p5->G + p6->G + p7->G + p8->G);
			sumaB = (p1->B + p2->B + p3->B + p4->B + p5->B + p6->B + p7->B + p8->B);

			srednia.R = sumaR / 8;
			srednia.G = sumaG / 8;
			srednia.B = sumaB / 8;

			Punkt->R = srednia.R;
			Punkt->G = srednia.G;
			Punkt->B = srednia.B;
			PunktMinusOne->R = srednia.R;
			PunktMinusOne->G = srednia.G;
			PunktMinusOne->B = srednia.B;
			PunktPlusOne->R = srednia.R;
			PunktPlusOne->G = srednia.G;
			PunktPlusOne->B = srednia.B;
			Punkt += 2;
			if (PunktMinusOne != NULL)
				PunktMinusOne += 2;
			if (PunktPlusOne != NULL)
				PunktPlusOne += 2;
			sumaR = 0;
			sumaG = 0;
			sumaB = 0;
		}
	}
}

static void KopiujFiltrowany()
{
	if (Filtrowany == 0) Filtrowany = new char[RozmiarRysunku];
	memcpy(Filtrowany, Rysunek, RozmiarRysunku);
}

static void Tworz(const HWND Okno)
{
	register const HANDLE Plik = CreateFile(TEXT("Obraz.bmp"), GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, 0, NULL);
	if (Plik == INVALID_HANDLE_VALUE) {
		MessageBox(Okno, TEXT("B³¹d w trakcie otwierania pliku Obraz.bmp"),
			NazwaAplikacji, MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	RozmiarRysunku = GetFileSize(Plik, NULL);
	Rysunek = new char[RozmiarRysunku];
	DWORD Odczytano;
	ReadFile(Plik, Rysunek, RozmiarRysunku, &Odczytano, NULL);
	CloseHandle(Plik);
	KopiujFiltrowany();
}

static void Rysuj(const HWND Okno)
{
	RECT Rozmiar;
	TCHAR Tekst[64];
	PAINTSTRUCT PS;
	unsigned int dl;
	GetClientRect(Okno, &Rozmiar);
	const HDC DC = BeginPaint(Okno, &PS);
	if (Rysunek != 0) {
		BITMAPFILEHEADER *bfh = reinterpret_cast<BITMAPFILEHEADER*>(Filtrowany);
		BITMAPINFO *bi = reinterpret_cast<BITMAPINFO*>(Filtrowany + sizeof(BITMAPFILEHEADER));
		SetStretchBltMode(DC, HALFTONE);
		StretchDIBits(DC, 0, 0, Rozmiar.right, Rozmiar.bottom, 0, 0, bi->bmiHeader.biWidth, bi->bmiHeader.biHeight,
			static_cast<void*>(Filtrowany + bfh->bfOffBits), bi, DIB_RGB_COLORS, SRCCOPY);
		dl = swprintf(Tekst, TEXT("Czas filtrowania: %.2lf ms"), CzasFiltrowania);
		TextOut(DC, 5, 5, Tekst, dl);
	}
	EndPaint(Okno, &PS);
}

static void Znak(const HWND Okno, const TCHAR Znak)
{
	if (Znak >= TEXT('0') && Znak <= TEXT('9')) {
		SYSTEMTIME S1, S2;
		NrFiltru = Znak - TEXT('0');
		SetWindowText(Okno, TEXT("TRWA FILTROWANIE..."));
		KopiujFiltrowany();
		GetSystemTime(&S1);
		if (NrFiltru > 0 && NrFiltru <= 9)
			for (unsigned int i = 0; i < 500; ++i) AplikujFiltr();
		GetSystemTime(&S2);
		CzasFiltrowania = 3600.0 * (S2.wHour - S1.wHour) + 60.0 * (S2.wMinute - S1.wMinute)
			+ (S2.wSecond - S1.wSecond) + 0.001 * (S2.wMilliseconds - S1.wMilliseconds);
		CzasFiltrowania *= 2.0;
		InvalidateRect(Okno, NULL, FALSE);
		SetWindowText(Okno, NazwaAplikacji);
		UpdateWindow(Okno);
	}
}

static LRESULT CALLBACK FunkcjaOkienkowa(HWND Okno, UINT Komunikat, WPARAM wParam, LPARAM lParam)
{
	switch (Komunikat) {
	case WM_CREATE:
		Tworz(Okno);
		break;
	case WM_CHAR:
		Znak(Okno, static_cast<TCHAR>(wParam));
		break;
	case WM_PAINT:
		Rysuj(Okno);
		break;
	case WM_DESTROY:
		delete[] Rysunek;
		delete[] Filtrowany;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(Okno, Komunikat, wParam, lParam);
	}
	return 0;
}

static bool RejestrujKlasy()
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)(1 + COLOR_WINDOW);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpfnWndProc = &FunkcjaOkienkowa;
	wc.lpszClassName = NazwaKlasy;
	wc.lpszMenuName = NULL;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	return (RegisterClassEx(&wc) != 0);
}

static void WyrejestrujKlasy()
{
	UnregisterClass(NazwaKlasy, GetModuleHandle(NULL));
}

int WINAPI WinMain(HINSTANCE Instancja, HINSTANCE Poprzednia, LPSTR Parametry, int Widocznosc)
{
	// Zarejestruj klasê. Protestuj, je¿eli wyst¹pi³ b³¹d.
	if (!RejestrujKlasy()) {
		MessageBox(NULL, TEXT("Nie uda³o siê zarejestrowaæ klasy okna!"),
			NazwaAplikacji, MB_ICONSTOP | MB_OK);
		return 1;
	}
	// Stwórz g³ówne okno. Równie¿ protestuj, je¿eli wyst¹pi³ b³¹d.
	HWND GlowneOkno = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_CLIENTEDGE,
		NazwaKlasy, NazwaAplikacji, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, Instancja, NULL);
	if (GlowneOkno == NULL) {
		MessageBox(NULL, TEXT("Nie uda³o siê stworzyæ g³ównego okna!"),
			NazwaAplikacji, MB_ICONSTOP | MB_OK);
		return 2;
	}
	// Wyœwietl i uaktualnij nowo stworzone okno.
	ShowWindow(GlowneOkno, Widocznosc);
	UpdateWindow(GlowneOkno);
	// G³ówna pêtla komunikatów w¹tku.
	MSG Komunikat;
	while (GetMessage(&Komunikat, NULL, 0, 0) > 0) {
		TranslateMessage(&Komunikat);
		DispatchMessage(&Komunikat);
	}
	// Zwolnij pamiêæ klas i zakoñcz proces.
	WyrejestrujKlasy();
	return static_cast<int>(Komunikat.wParam);
}
