/*
 * Стандартные заголовочные файлы
 */
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>

/*
 * Стандартные заголовочные файлы
 */
#include "term.h"

/*
 * ANSI/VT100 Terminal using example
 */

/* Объект "Линия фрейма" */
struct line_st
{
	char buf[FRM1_COL_MAX];
	int pos;
	int posmax;
};

/* Объект "Фрейм 1" */
struct frame1buf_st
{
	struct line_st line[FRM1_ROW_MAX]; /* строки фрейма */
	int colmin;						   /* текущая колонка во  фрейма */
	int rowmin;						   /* текущая строка во  фрейма */
	int col;						   /* текущая колонка во  фрейма */
	int row;						   /* текущая строка во  фрейма */
	int colmax;						   /* текущая колонка во  фрейма */
	int rowmax;						   /* текущая строка во  фрейма */
};

/* Объект "Фрейм 1" */
struct frame2buf_st
{
	struct line_st line[FRM2_ROW_MAX]; /* строки фрейма */
	int colmin;						   /* текущая колонка во  фрейма */
	int rowmin;						   /* текущая строка во  фрейма */
	int col;						   /* текущая колонка во  фрейма */
	int row;						   /* текущая строка во  фрейма */
	int colmax;						   /* текущая колонка во  фрейма */
	int rowmax;						   /* текущая строка во  фрейма */
};

/* Объект "Фрейм 1" */
struct frame3buf_st
{
	struct line_st line[FRM3_ROW_MAX]; /* строки фрейма */
	int colmin;						   /* текущая колонка во  фрейма */
	int rowmin;						   /* текущая строка во  фрейма */
	int col;						   /* текущая колонка во  фрейма */
	int row;						   /* текущая строка во  фрейма */
	int colmax;						   /* текущая колонка во  фрейма */
	int rowmax;						   /* текущая строка во  фрейма */
};

/* Создать объект "Фрейм 1" */
static struct frame1buf_st fb = {0};
/* Создать объект "Фрейм 2" */
static struct frame2buf_st frm2b = {0};
/* Создать объект "Фрейм 3" */
static struct frame3buf_st frm3b = {0};

/*
 * Работа 'setun1958'
 */
void Controller_Emulator(void);

/* Основной фрейм для вывода */
char ptty1_dbg[][80] =
	{
		" Пример  Example",
		" C: [---+0], ZW3, (-114)		-> zone[-1] : Z4Z30, (-3699)",
		"A*: [-++0-], Z4Z, (-46)		-> zone[-1] : 00000 00000, (0)",
		" k6..8[+00]: (A*)=>(S)",
		" ",
		"  K  : [-++0-+000], Z4Z30, (-3699)",
		"  F  : [00+-0], 01X, (6)",
		"  C  : [---++], ZW4, (-113)",
		"  W  : [0], 0, (0)",
		"  ph1: [0], 0, (0)",
		"  ph2: [0], 0, (0)",
		"  S  : [000000000000000000], 000000000, (0), {0.000000}",
		"  R  : [0+-++-+-+--+--+-+-], 1Y222YW22, (34673123), {0.805477}"};

/* Основной фрейм для вывода */
char ptty1_fram[][80] =
	{
		"[ Dump FRAM Setun-1958: ]",
		"Zone = 1",
		" ",
		"WW WX  1 3X X4        02 03  Z 1Y 3X",
		"   WY  1 13 X4           04  1 1Y 10",
		"WZ W0  1 20 2W        1W 1X  0 10 2X",
		"   W1  0 00 00           1Y  Z 1X 30",
		"W2 W3  Z WY Z3        1Z 10  Z 40 3X",
		"   W4  1 XY 10           11  1 14 10",
		"XW XX  0 W0 2X        12 13  0 03 2X",
		"   XY  Z 1X Z0           14  Z 42 30",
		"XZ X0  Z 04 30        2W 2X  Z 10 Y0",
		"   X1  Z 01 33           2Y  Z 1X 33",
		"X2 X3  Z 03 3X        2Z 20  1 23 10",
		"   X4  1 YY 10           21  0 04 2X",
		"YW YX  0 0X 2X        22 23  1 YX ZX",
		"   YY  Z 22 30           24  1 X0 13",
		"YZ Y0  Z 2W 40        3W 3X  1 1X Z0",
		"   Y1  Z 3Z 3X           3Y  Z 4Y 0X",
		"Y2 Y3  Z 32 3X        3Z 30  Z 22 3Z",
		"   Y4  Z 3W 23           31  Z 4Z Y3",
		"ZW ZX  Z 22 43        32 33  Z 1X Z0",
		"   ZY  Z 22 4X           34  1 WW Y2",
		"ZZ Z0  Z 22 3X        4W 4X  1 YX ZX",
		"   Z1  1 Z4 10           4Y  1 34 13",
		"Z2 Z3  Z 43 2X        4Z 40  1 XX Z0",
		"   Z4  1 ZX 30           41  1 WW X4",
		"0W 0X  Z 04 20        42 43  Z 10 ZX",
		"   0Y  Z 1Y Y0           44  1 41 1X",
		"0Z 00  Z 40 03        KC     0 00 0Y",
		"   01  Z 42 YX               1 YY YX"};

/*
 * Нарисовать окно 'setun1958'
 */
void Frames_Draw(void)
{

	home();
	set_display_atrib(B_BLACK);
	//
	//        12345678901234567890123456789012345678901234567890123456789012345678901234567890
	//
	puts(																					  /* Frame1 */
		 "┌─────────────────────────── Setup 1958 ver 1.94 ──────────────────────────────┐\n" // 1
		 "│                                                                              │\n" // 2
		 "│                                                                              │\n" // 3
		 "│                                                                              │\n" // 4
		 "│                                                                              │\n" // 5
		 "│                                                                              │\n" // 6
		 "│                                                                              │\n" // 7
		 "│                                                                              │\n" // 8
		 "│                                                                              │\n" // 9
		 "│                                                                              │\n" // 0
		 "│                                                                              │\n" // 1
		 "│                                                                              │\n" // 2
		 "│                                                                              │\n" // 3
		 "│                                                                              │\n" // 4
		 "│                                                                              │\n" // 5
		 "│                                                                              │\n" // 6
		 "│                                                                              │\n" // 7
		 "│                                                                              │\n" // 8
		 "│                                                                              │\n" // 9
		 "│                                                                              │\n" // 0
		 "│                                                                              │\n" // 1
		 "│                                                                              │\n" // 2
		 "│                                                                              │\n" // 3
		 "│                                                                              │\n" // 4
		 "│                                                                              │\n" // 5
		 "│                                                                              │\n" // 6
		 "│                                                                              │\n" // 7
		 "│                                                                              │\n" // 8
		 "│                                                                              │\n" // 9
		 "│                                                                              │\n" // 0
		 "│                                                                              │\n" // 1
		 "│                                                                              │\n" // 2
		 "└──────────────────────────────────────────────────────────────────────────────┘\n" // 3
		 "┌──────────────────────────────────────────────────────────────────────────────┐\n" // 4
		 "│                                                                              │\n" // 5
		 "└──────────────────────────────────────────────────────────────────────────────┘"); // 6
																							  /*Frame2*/
	resetcolor();
}

/*
 * Установить цвет печати символа во Фрейм
 */
void SetColorFrame(int setcolor)
{
	if (setcolor == 0) /* Color default */
	{
		/* Атрибуты цвета */
		set_display_atrib(B_BLACK);
		set_display_atrib(BRIGHT);
		set_display_atrib(F_WHITE);
	}
	else if (setcolor == 1) /* GREEN */
	{
		set_display_atrib(BRIGHT);
		set_display_atrib(B_GREEN);
	}
	else if (setcolor == 2) /* RED */
	{
		set_display_atrib(BRIGHT);
		set_display_atrib(B_RED);
	}
	else
	{
		/* Атрибуты цвета */
		set_display_atrib(B_BLACK);
		set_display_atrib(BRIGHT);
		set_display_atrib(F_WHITE);
	}
}

/* ----------------
 * Функции Фрейма 1
 */
void initFrm1Buf(void)
{
	int i = 0;
	int n = sizeof(fb.line) / sizeof(fb.line[0]);
	for (i = 0; i < n; i++)
	{
		memset(fb.line[i].buf, ' ', FRM1_COL_MAX); // Заполнить space! 
		fb.line[i].pos = 0;
		fb.line[i].posmax = FRM1_COL_MAX;
	}
	fb.colmin = FRM1_COL1;
	fb.rowmin = FRM1_ROW1;
	fb.col = FRM1_COL1; /* текущая позиция */
	fb.row = FRM1_ROW1; /* текущая позиция */
	fb.colmax = FRM1_COL2;
	fb.rowmax = FRM1_ROW2;
}

/*
 * Очистить буфер с вывести пустые строки Фрейма 2
 */
void prnClearFrm1Buf(void)
{
	int i;
	int n = sizeof(fb.line) / sizeof(fb.line[0]);

	/* Переписать все строки строки */
	for (i = 0; i < n; i++)
	{
		memset(fb.line[i].buf, ' ', FRM1_COL_MAX); // Заполнить space! 
		fb.line[i].pos = 0;
		fb.line[i].posmax = FRM1_COL_MAX;
	}
	fb.colmin = FRM1_COL1;
	fb.rowmin = FRM1_ROW1;
	fb.col = FRM1_COL1; /* текущая позиция */
	fb.row = FRM1_ROW1; /* текущая позиция */
	fb.colmax = FRM1_COL2;
	fb.rowmax = FRM1_ROW2;

	for (i = 0; i < n; i++)
	{
		/* Установить позицию курсора */
		gotoxy(FRM1_COL1, FRM1_ROW1 + i);
		/* Вывод на консоль */
		printf("%s", fb.line[i].buf);
	}
}

/*
 *  Вставить символ во фрейм
 *
 *  Возврат: событие 0 - обновить текущую строку
 *           событие 1 - сдвинуть все строки
 */
int setCharFrm1Buf(char ch)
{
	int flg = 0;

	if (ch == 0)
	{
		return 0;
	}
	else if (ch == '\n')
	{
		fb.line[fb.row - fb.rowmin].pos = 0;
		fb.row++;
		fb.col = fb.colmin;

		if (fb.row > fb.rowmax)
		{
			fb.row = fb.rowmax;
			fb.line[fb.row - fb.rowmin].pos = 0;
			return 1;
		}
		else {
			fb.line[fb.row - fb.rowmin].pos = 0;
			return 0;
		}
	}

	if (ch < ' ')
	{
		ch = ' ';
	}

	fb.line[fb.row - fb.rowmin].buf[fb.line[fb.row - fb.rowmin].pos] = ch;
	fb.line[fb.row - fb.rowmin].pos++;

	if (fb.line[fb.row - fb.rowmin].pos > fb.line[fb.row - fb.rowmin].posmax)
	{

		fb.row++;
		if (fb.row > fb.rowmax)
		{
			fb.row = fb.rowmax;
		}

		fb.col = fb.colmin;
		fb.line[fb.row - fb.rowmin].pos = 0;

		flg += 1;
	}

	fb.col++;
	if (fb.col > fb.colmax)
	{
		if (fb.col > fb.colmax)
		{
			fb.col = fb.colmax;
			flg += 1;
		}
	}

	return flg > 0 ? 1 : 0;
}

/*
 *  Печать буфера во Фрейм 1
 *
 *  Возврат: событие 0 - обновить текущую строку
 *           событие 1 - сдвинуть все строки
 */
void prnFrm1Buf(int action)
{
	int i = 0;

	if (action == 0)
	{
		/* Установить позицию курсора */
		gotoxy(FRM1_COL1, fb.row);
		/* Вывод на консоль */
		printf("%s", fb.line[fb.row - fb.rowmin].buf);
	}
	else
	{ /* action > 0 */

		int n = sizeof(fb.line) / sizeof(fb.line[0]);

		/* Переписать все строки строки */
		for (i = 0; i < (n - 1); i++)
		{
			memcpy(fb.line[i].buf, fb.line[i + 1].buf, FRM1_COL_MAX);
			//fb.line[i].pos = fb.line[i + 1].pos;
		}
		memset(fb.line[n - 1].buf, ' ', FRM1_COL_MAX); // Заполнить space

		for (i = 0; i < n; i++)
		{
			/* Установить позицию курсора */
			gotoxy(FRM1_COL1, FRM1_ROW1 + i);
			/* Вывод на консоль */
			printf("%s", fb.line[i].buf);
		}
	}
}

/* ----------------
 * Функции Фрейма 2
 */
void initFrm2Buf(void)
{
	int i = 0;
	int n = sizeof(frm2b.line) / sizeof(frm2b.line[0]);
	for (i = 0; i < n; i++)
	{
		memset(frm2b.line[i].buf, ' ', FRM2_COL_MAX); // Заполнить space! 
		frm2b.line[i].pos = 0;
		frm2b.line[i].posmax = FRM2_COL_MAX;
	}
	frm2b.colmin = FRM2_COL1;
	frm2b.rowmin = FRM2_ROW1;
	frm2b.col = FRM2_COL1; /* текущая позиция */
	frm2b.row = FRM2_ROW1; /* текущая позиция */
	frm2b.colmax = FRM2_COL2;
	frm2b.rowmax = FRM2_ROW2;
}


/*
 *  Вставить символ во Фрейм
 *
 *  Возврат: событие 0 - обновить текущую строку
 *           событие 1 - сдвинуть все строки
 */
int setCharFrm2Buf(char ch)
{
	if (ch == 0)
	{
		return 0;
	}	
	else if (ch == '\n')
	{
		frm2b.line[frm2b.row - frm2b.rowmin].pos = 0;
		frm2b.row++;
		frm2b.col = frm2b.colmin;
		frm2b.line[frm2b.row - frm2b.rowmin].pos = 0;

		if (frm2b.row > frm2b.rowmax)
		{
			frm2b.row = frm2b.rowmax;
			return 1;
		}
		else {
			return 0;

		}		
	}

	if (ch < ' ')
	{
		ch = ' ';
	}

	frm2b.line[frm2b.row - frm2b.rowmin].buf[frm2b.line[frm2b.row - frm2b.rowmin].pos] = ch;
	frm2b.line[frm2b.row - frm2b.rowmin].pos++;

	frm2b.col++;

	if (frm2b.line[frm2b.row - frm2b.rowmin].pos > frm2b.line[frm2b.row - frm2b.rowmin].posmax)
	{

		frm2b.row++;
		if (frm2b.row > frm2b.rowmax)
		{
			frm2b.row = frm2b.rowmax;
		}

		frm2b.col = frm2b.colmin;
		frm2b.line[frm2b.row - frm2b.rowmin].pos = 0;

		return 1;
	}
	else
	{
		if (frm2b.col > frm2b.colmax)
		{
			frm2b.col = frm2b.colmax;
			frm2b.line[frm2b.row - frm2b.rowmin].pos = 0;
			return 1;
		}
	}

	return 0;
}

/*
 * Очистить буфер с вывести пустые строки Фрейма 2
 */
void prnClearFrm2Buf(void)
{
	int i;
	int n = sizeof(frm2b.line) / sizeof(frm2b.line[0]);

	/* Переписать все строки строки */
	for (i = 0; i < n; i++)
	{
		memset(frm2b.line[i].buf, ' ', FRM2_COL_MAX); // Заполнить space
		frm2b.line[i].pos = 0;
		frm2b.line[i].posmax = FRM2_COL_MAX;
	}
	frm2b.colmin = FRM2_COL1;
	frm2b.rowmin = FRM2_ROW1;
	frm2b.col = FRM2_COL1; /* текущая позиция */
	frm2b.row = FRM2_ROW1; /* текущая позиция */
	frm2b.colmax = FRM2_COL2;
	frm2b.rowmax = FRM2_ROW2;

	for (i = 0; i < n; i++)
	{
		/* Установить позицию курсора */
		gotoxy(FRM2_COL1, FRM2_ROW1 + i);
		/* Вывод на консоль */
		printf("%s", frm2b.line[i].buf);
	}
}

/*
 *  Вывод буфера во Фрейм 2
 *
 *  Возврат: событие 0 - обновить текущую строку
 *           событие 1 - сдвинуть все строки
 */
void prnFrm2Buf(int action)
{
	int i = 0;


	if (action == 0)
	{
		/* Установить позицию курсора */
		gotoxy( frm2b.colmin, frm2b.rowmin);
		/* Вывод на консоль */
		printf("%s", frm2b.line[frm2b.row - frm2b.rowmin].buf);
	}
	else
	{ /* action > 0 */
		
		int n = sizeof(frm2b.line) / sizeof(frm2b.line[0]);

		/* Переписать все строки строки */
		for (i = 0; i < (n - 1); i++)
		{
			memcpy(frm2b.line[i].buf, frm2b.line[i + 1].buf, FRM2_COL_MAX);
			frm2b.line[i].pos = frm2b.line[i + 1].pos;
		}

		memset(frm2b.line[n - 1].buf, ' ', FRM2_COL_MAX);

		for (i = 0; i < n; i++)
		{
			/* Установить позицию курсора */
			gotoxy(FRM2_COL1, FRM2_ROW1 + i);
			/* Вывод на консоль */
			printf("%s", frm2b.line[i].buf);			
		}
	}
}

/* ----------------
 * Функции Фрейма 3
 */
void initFrm3Buf(void)
{
	int i = 0;
	int n = sizeof(frm3b.line) / sizeof(frm3b.line[0]);
	for (i = 0; i < n; i++)
	{
		memset(frm3b.line[i].buf, ' ', FRM3_COL_MAX); // Заполнить space! 
		frm3b.line[i].pos = 0;
		frm3b.line[i].posmax = FRM3_COL_MAX;
	}
	frm3b.colmin = FRM3_COL1;
	frm3b.rowmin = FRM3_ROW1;
	frm3b.col = FRM3_COL1; /* текущая позиция */
	frm3b.row = FRM3_ROW1; /* текущая позиция */
	frm3b.colmax = FRM3_COL2;
	frm3b.rowmax = FRM3_ROW2;
}

/*
 *  Вставить символ во фрайм
 *
 *  Возврат: событие 0 - обновить текущую строку
 *           событие 1 - сдвинуть все строки
 */
int setCharFrm3Buf(char ch)
{

	if (ch == '\0')
	{
		return 0;
	}
	else if (ch == '\n')
	{
		frm3b.line[frm3b.row - frm3b.rowmin].pos = 0;
		frm3b.row++;
		frm3b.col = frm3b.colmin;

		frm3b.line[frm3b.row - frm3b.rowmin].pos = 0;

		if (frm3b.row > frm3b.rowmax)
		{
			frm3b.row = frm3b.rowmax;
			return 1;
		}
		else {
			return 0;
		}		
	}

	if (ch < ' ')
	{
		ch = ' ';
	}

	frm3b.line[frm3b.row - frm3b.rowmin].buf[frm3b.line[frm3b.row - frm3b.rowmin].pos] = ch;
	frm3b.line[frm3b.row - frm3b.rowmin].pos++;

	frm3b.col++;

	if (frm3b.line[frm3b.row - frm3b.rowmin].pos > frm3b.line[frm3b.row - frm3b.rowmin].posmax)
	{

		frm3b.row++;
		if (frm3b.row > frm3b.rowmax)
		{
			frm3b.row = frm3b.rowmax;
		}

		frm3b.col = frm3b.colmin;
		frm3b.line[frm3b.row - frm3b.rowmin].pos = 0;

		return 1;
	}
	else
	{
		if (frm3b.col > frm3b.colmax)
		{
			frm3b.col = frm3b.colmax;
			return 1;
		}
	}

	return 0;
}

void prnClearFrm3Buf(void)
{
	int i;
	int n = sizeof(frm3b.line) / sizeof(frm3b.line[0]);

	/* Переписать все строки строки */
	for (i = 0; i < n; i++)
	{
		memset(frm3b.line[i].buf, ' ', FRM3_COL_MAX); // Заполнить space
		frm3b.line[i].pos = 0;
		frm3b.line[i].posmax = FRM3_COL_MAX;
	}
	frm3b.colmin = FRM3_COL1;
	frm3b.rowmin = FRM3_ROW1;
	frm3b.col = FRM3_COL1; /* текущая позиция */
	frm3b.row = FRM3_ROW1; /* текущая позиция */
	frm3b.colmax = FRM3_COL2;
	frm3b.rowmax = FRM3_ROW2;

	for (i = 0; i < n; i++)
	{
		/* Установить позицию курсора */
		gotoxy(FRM3_COL1, FRM3_ROW1 + i);
		/* Вывод на консоль */
		printf("%s", frm3b.line[i].buf);		
	}
}

/*
 *  Отправить фрейм на консоль
 *
 *  Возврат: событие 0 - обновить текущую строку
 *           событие 1 - сдвинуть все строки
 */
void prnFrm3Buf(int action)
{
	int i = 0;

	if (action == 0)
	{
		/* Установить позицию курсора */
		gotoxy(FRM3_COL1, FRM3_ROW1);
		/* Вывод на консоль */
		printf("%s", frm3b.line[frm3b.row - frm3b.rowmin].buf);
	}
	else
	{ /* action > 0 */

		int n = sizeof(frm3b.line) / sizeof(frm3b.line[0]);

		/* Переписать все строки строки */
		for (i = 0; i < (n - 1); i++)
		{
			memcpy(frm3b.line[i].buf, frm3b.line[i + 1].buf, FRM3_COL_MAX);
			frm3b.line[i].pos = frm3b.line[i + 1].pos;
		}
		memset(frm3b.line[n - 1].buf, ' ', FRM3_COL_MAX);

		/* Атрибуты цвета */
		set_display_atrib(B_BLACK);
		set_display_atrib(BRIGHT);
		set_display_atrib(F_WHITE);

		for (i = 0; i < n; i++)
		{
			/* Установить позицию курсора */
			gotoxy(FRM3_COL1, FRM3_ROW1 + i);
			/* Вывод на консоль */
			printf("%s", frm3b.line[i].buf);
		}
	}
}

void PrintFrm1(const char *format, ...)
{
	int action = 0;
	int w = 0;
	char buffer[FRM1_COL_MAX]={0};
	
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);

	prnClearFrm2Buf();

	gotoxy(fb.col, fb.row);

	for (w = 0; w < strlen(buffer); w++)
	{
		action = setCharFrm1Buf(buffer[w]);
		prnFrm1Buf(action);
	}

	va_end(args);
}

void PrintFrm2(const char *format, ...)
{
	int action = 0;
	int w = 0;
	char buffer[FRM2_COL_MAX]={0};
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	for (w = 0; w < strlen(buffer); w++)
	{
		action = setCharFrm2Buf(buffer[w]);
		prnFrm2Buf(action);
	}
}

void PrintFrm3(const char *format, ...)
{
	int action = 0;
	int w = 0;
	char buffer[FRM3_COL_MAX]={0};
	va_list args;
	va_start(args, format);
	vsprintf(buffer, format, args);

	prnClearFrm3Buf();

	gotoxy(frm3b.colmin, frm3b.rowmin);

	for (w = 0; w < strlen(buffer); w++)
	{
		action = setCharFrm3Buf(buffer[w]);
		prnFrm3Buf(action);
	}

	va_end(args);

}

/*
 * Главный цикл работы 'setun1958'
 */
void Init_TUI_Setun(void)
{

	/* Подготовить вывод консоли */
	home();
	clrscr();
	Frames_Draw();

	/* Инициализация Фрейма 1 */
	initFrm1Buf();
	/* Инициализация Фрейма 2 */
	initFrm2Buf();
	/* Инициализация Фрейма 3 */
	initFrm3Buf();

	/* Установить цвет вывода */
	SetColorFrame(0);
}

#if 1 // viv+ Term_test

/* --------------------------------
 * Основной цикл работы 'setun1958'
 */
void Controller_Emulator(void)
{
	int j = 0;
	unsigned char port = 0x01;
	int alarm_counter = 0;
	time_t timer;
	struct tm *tm_info;

	srand(time(NULL));

	while (1)
	{
		int action = 0;
		alarm_counter++;
		time(&timer);
		tm_info = localtime(&timer);

		/* Атрибуты цвета */
		SetColorFrame(0);

		/*
		 * Вывод во фреймы
		 */

		{ /* Вывод вз Фрейм 1 */

			static int flg = 0; /* для вывода примеров */

			if (flg > 0)
			{

				for (j = 0; j < sizeof(ptty1_fram) / sizeof(ptty1_fram[0]); j++)
				{
					int w;
					for (w = 0; w < strlen(ptty1_fram[j]); w++)
					{
						action = setCharFrm1Buf(ptty1_fram[j][w]);
						prnFrm1Buf(action);
					}

					action = setCharFrm1Buf('\n');
					prnFrm1Buf(action);
				}

				flg = 0;
			}
			else
			{

				for (j = 0; j < sizeof(ptty1_dbg) / sizeof(ptty1_dbg[0]); j++)
				{
					int w;
					for (w = 0; w < strlen(ptty1_dbg[j]); w++)
					{
						action = setCharFrm1Buf(ptty1_dbg[j][w]);
						prnFrm1Buf(action);
					}

					action = setCharFrm1Buf('\n');
					prnFrm1Buf(action);
				}

				flg = 1;
			}
		}

		{ /* Вывод в продолжение командной строки во Фрейм 2 */

			char buf[FRM2_COL_MAX] = {0};
			int w;

			SetColorFrame(0);
			prnClearFrm2Buf();

			SetColorFrame(0);
			sprintf(buf, "setun1958:");

			for (w = 0; w < strlen(buf); w++)
			{
				action = setCharFrm2Buf(buf[w]);
				prnFrm2Buf(action);
			}
		}

		{ /* Вывод в продолжение командной строки во Фрейм 3 */

			char buf[FRM3_COL_MAX] = {0};
			int w;

			SetColorFrame(0);
			prnClearFrm3Buf();

			SetColorFrame(1);
			sprintf(buf, "STOP");

			for (w = 0; w < strlen(buf); w++)
			{
				action = setCharFrm3Buf(buf[w]);
				prnFrm3Buf(action);
			}

			gotoxy(frm3b.col, frm3b.row);
		}

		fflush(stdout);
		usleep(100000);
	}
}

/*
 * Главный цикл работы 'setun1958'
 */
int Term_test(void)
{

	home();
	clrscr();
	Frames_Draw();

	//
	// viv+ тестирование и отладка
	//
	// initFrm1Buf();
	// int cnt = 2000;
	// int flg;
	// while (cnt--)
	//{
	//	flg = setCharFrm1Buf( (uint8_t)(cnt) );
	//	prnFrm1Buf(flg);
	//}
	// exit(1);
	// -------------

	/* Инициализация Фрейма 1 */
	initFrm1Buf();
	/* Инициализация Фрейма 2 */
	initFrm2Buf();
	/* Инициализация Фрейма 3 */
	initFrm3Buf();

	/* Установить цвет */
	SetColorFrame(0);

	/* Непрерывный вывод */
	Controller_Emulator();

	return 0;
}

void TermTest1(void) {

        char buf[10];

        home();
		clrscr();

        memset(buf,0,10);
        gotoxy(1,1);
        printf("t1.1 %s\n",buf);
        sprintf(buf,"%s","123456");
        
		gotoxy(1,1);
        printf("t1.2 %s\n",buf);

        memset(buf,'.',10);
        gotoxy(1,2);
        printf("t1.3 %s\n",buf);
        sprintf(buf,"%s","7890");
        gotoxy(1,2);
        printf("t1.4 %s\n",buf);

        memset(buf,'_',10);
        gotoxy(1,3);
        printf("t1.5 %s\n",buf);
        sprintf(buf,"%s","7890\0");
        gotoxy(1,3);printf("t1.6 %s\n",buf);

		//visible_cursor(); // error
		//visible_cursor(); // error 				
		//printf("\033[?12l"); //error
		//printf("\033[25m"); 
		//visible_cursor(); // error
		printf("\033[?25l");		
}

#endif



/* EOF 'term.c' */
