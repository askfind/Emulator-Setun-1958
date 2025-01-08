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

/* Прототип функци установить цвет вывода исмволов */
void SetColorFrame(int setcolor);

/* Прототипы функций Frame 1 */
void Frm1BufInit(void);
void prnClearFrm1Buf(void);
int setCharFrm1Buf(char ch);
void prnFrm1Buf(int action);

/* Прототипы функций Frame 2 */
void Frm2BufInit(void);
void prnClearFrm2Buf(void);
int setCharFrm2Buf(char ch);
void prnClearFrm2Buf(void);
void prnFrm2Buf(int action);

/* Прототипы функций Frame 3 */
void Frm3BufInit(void);
void prnClearFrm3Buf(void);
int setCharFrm3Buf(char ch);
void prnClearFrm3Buf(void);
void prnFrm3Buf(int action);

/* Нарисовать экран работы 'setun1958' */
void Frames_Draw(void);

#endif /*__TERM_EXAMPLE__*/
