#ifndef __TERM_EXAMPLE__
#define __TERM_EXAMPLE__

#define ESC "\033"

//Format text
#define RESET 		0
#define BRIGHT 		1
#define DIM			2
#define UNDERSCORE	3
#define BLINK		4
#define REVERSE		5
#define HIDDEN		6

//Foreground Colours (text)

#define F_BLACK 	30
#define F_RED		31
#define F_GREEN		32
#define F_YELLOW	33
#define F_BLUE		34
#define F_MAGENTA 	35
#define F_CYAN		36
#define F_WHITE		37

//Background Colours
#define B_BLACK 	40
#define B_RED		41
#define B_GREEN		42
#define B_YELLOW	44
#define B_BLUE		44
#define B_MAGENTA 	45
#define B_CYAN		46
#define B_WHITE		47

/*
 * Set Display Attribute Mode	<ESC>[{attr1};...;{attrn}m
 */
#define resetcolor() printf(ESC "[0m")
#define set_display_atrib(color) printf(ESC "[%dm", color)

#define home() printf(ESC "[H")	   /* Move cursor to the indicated row, column (origin at 1,1) */
#define clrscr() printf(ESC "[2J") /* lear the screen, move to (1,1) */
#define gotoxy(x, y) printf(ESC "[%d;%dH", y, x);
#define visible_cursor() printf(ESC "[?251");

/*
 * Вывод в текстовые фреймы работы 'setun1958'
 */
/* Frame1(c1,r1,c2,r2) */
#define FRM1_ROW1 (2)
#define FRM1_ROW2 (32)
#define FRM1_ROW_MAX (FRM1_ROW2 - FRM1_ROW1 + 1)
#define FRM1_COL1 (2)
#define FRM1_COL2 (79)
#define FRM1_COL_MAX (FRM1_COL2 - FRM1_COL1 + 1)
/* Frame2 */
#define FRM2_ROW1 (35)
#define FRM2_ROW2 (35)
#define FRM2_ROW_MAX (FRM2_ROW2 - FRM2_ROW1 + 1)
#define FRM2_COL1 (3)
#define FRM2_COL2 (79)
#define FRM2_COL_MAX (FRM2_COL2 - FRM2_COL1 + 1)
/* Frame3 */
#define FRM3_ROW1 (35)
#define FRM3_ROW2 (35)
#define FRM3_ROW_MAX (FRM3_ROW2 - FRM3_ROW1 + 1)
#define FRM3_COL1 (14)
#define FRM3_COL2 (79)
#define FRM3_COL_MAX (FRM3_COL2 - FRM3_COL1 + 1)

/* Прототип функци установить цвет вывода исмволов */
void SetColorFrame(int setcolor);

/* Прототипы функций Frame 1 */
void initFrm1Buf(void);
void prnClearFrm1Buf(void);
int  setCharFrm1Buf(char ch);
void prnFrm1Buf(int action);

/* Прототипы функций Frame 2 */
void initFrm2Buf(void);
void prnClearFrm2Buf(void);
int  setCharFrm2Buf(char ch);
void prnClearFrm2Buf(void);
void prnFrm2Buf(int action);

/* Прототипы функций Frame 3 */
void initFrm3Buf(void);
void prnClearFrm3Buf(void);
int  setCharFrm3Buf(char ch);
void prnClearFrm3Buf(void);
void prnFrm3Buf(int action);

/* Нарисовать экран работы 'setun1958' */
void Frames_Draw(void);


void PrintFrm1(const char *format, ...);
void PrintFrm2(const char *format, ...);
void PrintFrm3(const char *format, ...);

void Init_TUI_Setun(void);

#endif /*__TERM_EXAMPLE__*/
