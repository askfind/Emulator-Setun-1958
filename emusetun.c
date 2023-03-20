/**
 * Filename: "emusetun.c "
 *
 * Project: Виртуальная машина МЦВМ "Сетунь" 1958 года на языке Си
 *
 * Create date: 01.11.2018
 * Edit date:   20.03.2023
 *
 * Version: 1.85
 */

/**
 *  Заголовочные файла
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

#include "emusetun.h"

#define debug_print(...)                  \
	do                                    \
	{                                     \
		if (DEBUG)                        \
			fprintf(stdout, __VA_ARGS__); \
	} while (0)

//  - [ ] исправить вывод в 9-ном виде.

/* Макросы максимальное значения тритов */
#define TRIT1_MAX (+1)
#define TRIT1_MIN (-1)
#define TRIT3_MAX (+13)
#define TRIT3_MIN (-13)
#define TRIT4_MAX (+40)
#define TRIT4_MIN (-40)
#define TRIT5_MAX (+121)
#define TRIT5_MIN (-121)
#define TRIT9_MAX (+9841)
#define TRIT9_MIN (-9841)
#define TRIT18_MAX (+193710244L)
#define TRIT18_MIN (-193710244L)

#define SIZE_TRITS_MAX (32)		 /* максимальная количество тритов троичного числа */
#define SIZE_LONG_TRITS_MAX (64) /* максимальная количество тритов в длинном троичном числе */

/* *******************************************
 * Реализация виртуальной машины "Сетунь-1958"
 * --------------------------------------------
 */

/**
 *  Троичные типы данных памяти "Сетунь-1958"
 */

/**
 *  Описание тритов в троичном числе в поле бит
 *
 *  W(1)	 	W, {t1,t0} = {t1:[b0],t0:[b0]}
 *  A(1...5) 	A, {t1,t0} = {t1:[b4...b0],t0:[b4...b0]}
 *  K(1...9)	K, {t1,t0} = {t1:[b8...b0],t0:[b8...b0]}
 *  S(1...18)	S, {t1,t0} = {t1:[b17...b0],t0:[b17...b0]}
 *
 */
#define SIZE_WORD_SHORT (9)		 /* короткое слово 9-трит  */
#define SIZE_WORD_LONG (18)		 /* длинное слово  18-трит */
#define SIZE_WORD_LONG_LONG (36) /* двойное длинное слово  36-трит */

/**
 * Описание ферритовой памяти FRAM
 */
#define NUMBER_ZONE_FRAM (3)	 /* количество зон ферритовой памяти */
#define SIZE_ZONE_TRIT_FRAM (54) /* количнество коротких 9-тритных слов в зоне */
#define SIZE_ALL_TRIT_FRAM (162) /* всего количество коротких 9-тритных слов */

#define SIZE_GRFRAM (2)		   /* количнество груп (0...2) в FRAM */
#define SIZE_GR_TRIT_FRAM (81) /* количнество коротких 9-тритных слов в зоне */

/**
 * Адреса зон ферритовой памяти FRAM
 */
#define ZONE_M_FRAM_BEG (-120) /* ----0 */
#define ZONE_M_FRAM_END (-41)  /* -++++ */
#define ZONE_0_FRAM_BEG (-40)  /* 0---0 */
#define ZONE_0_FRAM_END (40)   /* 0++++ */
#define ZONE_P_FRAM_BEG (42)   /* +---0 */
#define ZONE_P_FRAM_END (121)  /* +++++ */

/**
 * Описание магнитного барабана DRUM
 */
#define SIZE_TRIT_DRUM (1944)	 /* количество хранения коротких слов из 9-тритов */
#define SIZE_ZONE_TRIT_DRUM (54) /* количество 9-тритных слов в зоне */
#define NUMBER_ZONE_DRUM (36)	 /* количество зон на магнитном барабане */
#define ZONE_DRUM_BEG (5)		 /* 01-- */
#define ZONE_DRUM_END (40)		 /* ++++ */

/**
 * Типы данных для виртуальной троичной машины "Сетунь-1958"
 */
typedef uintptr_t addr;

typedef struct trs
{
	uint8_t l;	 /* длина троичного числа в тритах */
	uint32_t t1; /* троичное число FALSE,TRUE */
	uint32_t t0; /* троичное число NIL */
} trs_t;

typedef struct long_trs
{
	uint8_t l;	 /* длина троичного числа в тритах			*/
	uint64_t t1; /* троичное число FALSE,TRUE */
	uint64_t t0; /* троичное число NIL */
} long_trs_t;

/**
 * Для обработки клавиатуры
 */
struct termios orig_termios;

/**
 * Статус выполнения операции  "Сетунь-1958"
 */
enum
{
	OK = 0,					 /* Успешное выполнение операции */
	WORK = 1,				 /* Выполнение операций виртуальной машины */
	STOP = 2,				 /* Успешный останов машины */
	STOP_OVER = 3,			 /* Останов по переполнению результата операции машины */
	STOP_ERROR = 4,			 /* Аварийный останов машины */
	STOP_ERROR_MB_NUMBER = 5 /* Номер зоны DRUM не поддерживается */
};

/**
 * Вывод отладочной информации памяти машины "Сетунь-1958"
 */
static uint8_t DEBUG = 1;
static uint32_t STEP = 0;
static int32_t BREAKPOINT = INT32_MAX;

/* Фотосчитыватель ФТ1 */
FILE *ptr1;
/* Фотосчитыватель ФТ2 */
FILE *ptr2;
/* Перфоратор ПЛ */
FILE *ptp1;
/* Печать Телетайп ТП, Пишущая машинка ПМ */
FILE *tty1;

/**
 * Определение памяти машины "Сетунь-1958"
 */
trs_t mem_fram[SIZE_GR_TRIT_FRAM][SIZE_GRFRAM];						   /* оперативное запоминающее устройство на ферритовых сердечниках */
trs_t mem_drum[NUMBER_ZONE_DRUM + ZONE_DRUM_BEG][SIZE_ZONE_TRIT_DRUM]; /* запоминающее устройство на магнитном барабане */

/** ***********************************
 *  Определение регистров "Сетунь-1958"
 *  -----------------------------------
 */
/* Основные регистры в порядке пульта управления */
trs_t K = {.l = 9, .t0 = 0, .t1 = 0}; /* K(1:9)  код команды (адрес ячейки оперативной памяти) */
trs_t F = {.l = 5, .t0 = 0, .t1 = 0}; /* F(1:5)  индекс регистр  */
trs_t C = {.l = 5, .t0 = 0, .t1 = 0}; /* C(1:5)  программный счетчик  */
trs_t W = {.l = 1, .t0 = 0, .t1 = 0}; /* W(1:1)  знак троичного числа */
//
trs_t ph1 = {.l = 1, .t0 = 0, .t1 = 0}; /* ph1(1:1) 1 разряд переполнения */
trs_t ph2 = {.l = 2, .t0 = 0, .t1 = 0}; /* ph2(1:1) 2 разряд переполнения */
trs_t S = {.l = 18, .t0 = 0, .t1 = 0};	/* S(1:18) аккумулятор */
trs_t R = {.l = 18, .t0 = 0, .t1 = 0};	/* R(1:18) регистр множителя */
trs_t MB = {.l = 4, .t0 = 0, .t1 = 0};	/* MB(1:4) троичное число зоны магнитного барабана */

/* Дополнительные */
trs_t MR = {.l = 9, .t0 = 0, .t1 = 0};		  /* временный регистр для обмена с FRAM */
long_trs_t TMP = {.l = 36, .t0 = 0, .t1 = 0}; /* временная переменная для троичного числа */

/** ------------------------------------------------------
 *  Прототипы функций для виртуальной машины "Сетунь-1958"
 *  ------------------------------------------------------
 */
int32_t pow3(int8_t x);
int8_t trit2int(trs_t t);
trs_t bit2trit(int8_t b);

/**  ---------------------------------------
 *   Операции с тритами
 *   trit = {-1,0,1}  - трит значение трита
 */
void and_t(int8_t *a, int8_t *b, int8_t *s);
void xor_t(int8_t *a, int8_t *b, int8_t *s);
void or_t(int8_t *a, int8_t *b, int8_t *s);
//  - [ ] исправить вывод в 9-ном виде.
void not_t(int8_t *a, int8_t *s);
void sum_t(int8_t *a, int8_t *b, int8_t *p0, int8_t *s, int8_t *p1);

/**  -------------------------------------------------------------
 *   Троичные числа
 *
 *   TRITS-1  = [t0]       - обозначение позиции тритов в числе
 *   TRITS-32 = [t31...t0] - обозначение позиции тритов в числе
 *
 */
int8_t get_trit(trs_t t, uint8_t pos);
trs_t set_trit(trs_t t, uint8_t pos, int8_t trit);
trs_t slice_trs(trs_t t, int8_t p1, int8_t p2);
void copy_trs(trs_t *src, trs_t *dst);

/**  -------------------------------------------------------------
 *   Троичные числа
 *   TRITS-64 = [t63...t0] - обозначение позиции тритов в числе
 */
void clear_long(long_trs_t *t);
void clear_full_long(long_trs_t *t);
int8_t get_long_trit(long_trs_t t, uint8_t pos);
long_trs_t set_long_trit(long_trs_t t, uint8_t pos, int8_t trit);
int8_t sgn_long_trs(long_trs_t x);
long_trs_t shift_long_trs(long_trs_t t, int8_t s);
void copy_long_trs(long_trs_t *src, long_trs_t *dst);

/**  -------------------------------------------------------------
 *   Троичные числа регистров Setun-1958
 *
 *   SETUN-T1  = [s1]      - обозначение позиции тритов в регистре
 *   SETUN-T5  = [s1...s5]
 *   SETUN-T9  = [s1...s9]
 *   SETUN-T18 = [s1...s18]
 */
int8_t get_trit_setun(trs_t t, uint8_t pos);
trs_t set_trit_setun(trs_t t, uint8_t pos, int8_t trit);
trs_t slice_trs_setun(trs_t t, int8_t p1, int8_t p2);
void copy_trs_setun(trs_t *src, trs_t *dst);

/**
 * Общие функции для троичных чисел из тритов
 */
void clear(trs_t *t);
void mod_3_n(trs_t *t, uint8_t n);
void clear_full(trs_t *t);

void inc_trs(trs_t *tr);
void dec_trs(trs_t *tr);

trs_t and_trs(trs_t a, trs_t b);
trs_t or_trs(trs_t a, trs_t b);
trs_t xor_trs(trs_t a, trs_t b);
trs_t add_trs(trs_t a, trs_t b);
trs_t sub_trs(trs_t a, trs_t b);
trs_t mul_trs(trs_t a, trs_t b);
trs_t div_trs(trs_t a, trs_t b);
trs_t shift_trs(trs_t t, int8_t s);

/* Long trits */
long_trs_t add_long_trs(long_trs_t a, long_trs_t b);

/* Преобразование тритов в другие типы данных */
int32_t trs2digit(trs_t t);
uint8_t trit2lt(int8_t v);
int8_t symtrs2numb(uint8_t c);
int8_t str_symtrs2numb(uint8_t *s);
trs_t smtr(uint8_t *s);

/* Операции с ферритовой памятью машины FRAM */
void clean_fram_zone(trs_t z);
void clean_fram(void);
trs_t ld_fram(trs_t ea);
void st_fram(trs_t ea, trs_t v);

/* Операции ввода и вывода "Сетунь-1958" */

/* Регист переключения Русский/Латинский */
static uint8_t russian_latin_sw = 1;
/* Регист переключения Буквенный/Цифровой */
static uint8_t letter_number_sw = 0;
/* Регист переключения цвета печатающей ленты */
static uint8_t color_sw = 0;

uint8_t pl_to_ind(uint8_t *perline);
int ConvertSWtoPaper(char *path_lst, char *path_txt);
int DumpFileTxs(char *pathfile);
trs_t Decoder_Command_Paper_Line(char *paperline, uint8_t *err);
trs_t Decoder_Symbol_Paper_Line(char *paperline, uint8_t *err);
uint8_t Read_Commands_from_FT1(FILE *file, trs_t fa);
uint8_t Read_Symbols_from_FT1(FILE *file, trs_t fa);
uint8_t Read_Commands_from_FT2(FILE *file, trs_t fa);
uint8_t Read_Symbols_from_FT1(FILE *file, trs_t fa);
uint8_t Read_Symbols_from_FT2(FILE *file, trs_t fa);
uint8_t Write_Commands_to_TTY1(FILE *file, trs_t fa);
uint8_t Write_Symbols_to_TTY1(FILE *file, trs_t fa);
uint8_t Perforation_Commands_to_PTP1(FILE *file, trs_t fa);
uint8_t Perforation_Symbols_to_PTP1(FILE *file, trs_t fa);
void init_tab4(void);

/* Очистить память магнитного барабана DRUM */
void clean_drum(void);
trs_t ld_drum(trs_t ea, uint8_t ind);
void st_drum(trs_t ea, uint8_t ind, trs_t v);

/* Операции копирования */
void fram_to_drum(trs_t ea);
void drum_to_fram(trs_t ea);

/* Функции троичной машины Сетунь-1958 */
void reset_setun_1958(void);				/* Сброс машины */
trs_t control_trs(trs_t a);					/* Устройство управления */
trs_t next_address(trs_t c);				/* Определить следующий адрес */
int8_t execute_trs(trs_t addr, trs_t oper); /* Выполнение кодов операций */

/* Функции вывода отладочной информации */
void view_short_reg(trs_t *t, uint8_t *ch);
void view_short_regs(void);


/** -----------------------------------------------
 *  Функционал терминала и обработки событий клавиш
 *  -----------------------------------------------
 */
void reset_terminal_mode()
{
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
}

/* Получение символа клавиши клавиатуры */
int getch()
{
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}
/** ---------------------------------------------------
 *  Реализации функций виртуальной машины "Сетунь-1958"
 *  ---------------------------------------------------
 */

/**
 * Возведение в степень по модулю 3
 */
int32_t pow3(int8_t x)
{
	int8_t i;
	int32_t r = 1;
	for (i = 0; i < x; i++)
	{
		r *= 3;
	}
	return r;
}

/**
 * Очистить поле битов троичного числа
 */
void clear(trs_t *t)
{
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Операция Mod 3^N
 */
void mod_3_n(trs_t *t, uint8_t n)
{
	t->t1 &= (0xFFFFFFFF >> (SIZE_TRITS_MAX - n));
	t->t0 &= (0xFFFFFFFF >> (SIZE_TRITS_MAX - n));
}

/**
 * Очистить длину троичного числа и поле битов троичного числа
 */
void clear_full(trs_t *t)
{
	t->l = 0;
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Очистить поле битов троичного числа
 */
void clear_long(long_trs_t *t)
{
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Очистить длину троичного числа и поле битов троичного числа
 */
void clear_full_long(long_trs_t *t)
{
	t->l = 0;
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Преобразование младшего трита в int8
 */
int8_t trit2int(trs_t t)
{
	if ((t.t0 & 1) > 0)
	{
		if ((t.t1 & 1) > 0)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

/**  ---------------------------------------
 *   Операции с тритами
 *   trit = {-1,0,1}  - трит значение трита
 */

/* Троичное сложение двух тритов с переносом */
void sum_t(int8_t *a, int8_t *b, int8_t *p0, int8_t *s, int8_t *p1)
{
	switch (*a + *b + *p0)
	{
	case -3:
	{
		*s = 0;
		*p1 = -1;
	}
	break;
	case -2:
	{
		*s = 1;
		*p1 = -1;
	}
	break;
	case -1:
	{
		*s = -1;
		*p1 = 0;
	}
	break;
	case 0:
	{
		*s = 0;
		*p1 = 0;
	}
	break;
	case 1:
	{
		*s = 1;
		*p1 = 0;
	}
	break;
	case 2:
	{
		*s = -1;
		*p1 = 1;
	}
	break;
	case 3:
	{
		*s = 0;
		*p1 = 1;
	}
	break;
	default:
	{
		*s = 0;
		*p1 = 0;
	}
	break;
	}
}

/* Троичное умножение тритов */
void and_t(int8_t *a, int8_t *b, int8_t *s)
{
	if ((*a * *b) > 0)
	{
		*s = 1;
	}
	else if ((*a * *b) < 0)
	{
		*s = -1;
	}
	else
	{
		*s = 0;
	}
}

/* Троичное отрицание трита */
void not_t(int8_t *a, int8_t *s)
{
	int8_t trit = *a;

	if (trit > 0)
	{
		*s = -1;
	}
	else if (trit < 0)
	{
		*s = 1;
	}
	else
	{
		*s = 0;
	}
}

/* Троичное xor тритов */
void xor_t(int8_t *a, int8_t *b, int8_t *s)
{
	if (*a == -1 && *b == -1)
	{
		*s = -1;
	}
	else if (*a == 1 && *b == -1)
	{
		*s = 1;
	}
	else if (*a == -1 && *b == 1)
	{
		*s = 1;
	}
	else if (*a == 1 && *b == 1)
	{
		*s = -1;
	}
	else if (*a == 0 && *b == 1)
	{
		*s = 0;
	}
	else if (*a == 0 && *b == -1)
	{
		*s = 0;
	}
	else if (*a == 1 && *b == 0)
	{
		*s = 0;
	}
	else if (*a == -1 && *b == 0)
	{
		*s = 0;
	}
	else
	{
		*s = 0;
	}
}

/* Троичное or тритов */
/* OR (A, B)
 *   A ∨ B
 *       B
 *	  - 0 +
 *   - - 0 +
 * A	0 0	0 +
 *   + + + +
 */
void or_t(int8_t *a, int8_t *b, int8_t *s)
{
	if (*a == -1 && *b == -1)
	{
		*s = -1;
	}
	else if (*a == 1 && *b == -1)
	{
		*s = 1;
	}
	else if (*a == -1 && *b == 1)
	{
		*s = 1;
	}
	else if (*a == 1 && *b == 1)
	{
		*s = 1;
	}
	else if (*a == 0 && *b == 1)
	{
		*s = 1;
	}
	else if (*a == 0 && *b == -1)
	{
		*s = -1;
	}
	else if (*a == 1 && *b == 0)
	{
		*s = 1;
	}
	else if (*a == -1 && *b == 0)
	{
		*s = 0;
	}
	else
	{
		*s = 0;
	}
}

/**  -------------------------------------------------------------
 *   Троичные числа
 *
 *   TRITS-64 = [t63...t0] - обозначение позиции тритов в числе
 *
 */

/* Получить целое со знаком трита в позиции троичного числа */
int8_t get_long_trit(long_trs_t t, uint8_t pos)
{
	t.l = min(t.l, SIZE_LONG_TRITS_MAX);
	pos = min(pos, SIZE_LONG_TRITS_MAX - 1);
	if ((t.t0 & (((uint64_t)(1)) << pos)) > 0)
	{
		if ((t.t1 & (((uint64_t)(1)) << pos)) > 0)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

/* Установить трит в троичном числе */
long_trs_t set_long_trit(long_trs_t t, uint8_t pos, int8_t trit)
{
	long_trs_t r = t;
	r.l = min(r.l, SIZE_LONG_TRITS_MAX);
	pos = min(pos, SIZE_LONG_TRITS_MAX - 1);
	if (trit > 0)
	{
		r.t1 |= (((uint64_t)(1)) << pos);
		r.t0 |= (((uint64_t)(1)) << pos);
	}
	else if (trit < 0)
	{
		r.t1 &= ~(((uint64_t)(1)) << pos);
		r.t0 |= (((uint64_t)(1)) << pos);
	}
	else
	{
		r.t1 &= ~(((uint64_t)(1)) << pos);
		r.t0 &= ~(((uint64_t)(1)) << pos);
	}
	return r;
}

/* Операция знак SGN троичного числа */
int8_t sgn_long_trs(long_trs_t x)
{
	int8_t i;
	x.l = min(x.l, SIZE_LONG_TRITS_MAX);
	for (i = x.l; i > 0; i--)
	{
		if ((x.t0 & (((uint64_t)(1)) << i)) > 0)
		{
			if ((x.t1 & (((uint64_t)(1)) << i)) > 0)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
	}
	return 0;
}

/**
 * Операция сдвига тритов
 * Параметр:
 * if(d > 0) then "Вправо"
 * if(d == 0) then "Нет сдвига"
 * if(d < 0) then "Влево"
 * Возврат: Троичное число
 */
long_trs_t shift_long_trs(long_trs_t t, int8_t d)
{
	if (d > 0) /* сдвиг  */
	{
		t.t1 = t.t1 << d;
		t.t0 = t.t0 << d;
	}
	else if (d < 0)
	{
		t.t1 = t.t1 >> (-d);
		t.t0 = t.t0 >> (-d);
	}
	return t;
}

/* Оперция присваивания троичных чисел в регистры */
void copy_long_trs(long_trs_t *src, long_trs_t *dst)
{
	if (src->l == dst->l)
	{
		dst->t1 = src->t1;
		dst->t0 = src->t0;
	}
	else if (src->l > dst->l)
	{
		dst->t1 = src->t1;
		dst->t0 = src->t0 & ((uint64_t)(0xFFFFFFFFFFFFFFFF) << dst->l);
	}
	else
	{
		dst->t1 = src->t1;
		dst->t0 = src->t0 & ((uint64_t)(0xFFFFFFFFFFFFFFFF) >> src->l);
	}
}

/**  -------------------------------------------------------------
 *   Троичные числа
 *
 *   TRITS-32 = [t31...t0] - обозначение позиции тритов в числе
 *
 */

/* Получить целое со знаком трита в позиции троичного числа */
int8_t get_trit(trs_t t, uint8_t pos)
{
	t.l = min(t.l, SIZE_TRITS_MAX);
	pos = min(pos, SIZE_TRITS_MAX);
	if ((t.t0 & (1 << pos)) > 0)
	{
		if ((t.t1 & (1 << pos)) > 0)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

/* Установить трит в троичном числе */
trs_t set_trit(trs_t t, uint8_t pos, int8_t trit)
{
	trs_t r = t;
	r.l = min(r.l, SIZE_TRITS_MAX);
	pos = min(pos, SIZE_TRITS_MAX);
	if (trit > 0)
	{
		r.t1 |= (1 << pos);
		r.t0 |= (1 << pos);
	}
	else if (trit < 0)
	{
		r.t1 &= ~(1 << pos);
		r.t0 |= (1 << pos);
	}
	else
	{
		r.t1 &= ~(1 << pos);
		r.t0 &= ~(1 << pos);
	}
	return r;
}

/* Операция знак SGN троичного числа */
int8_t sgn_trs(trs_t x)
{
	int8_t i;
	x.l = min(x.l, SIZE_TRITS_MAX);
	for (i = x.l; i > 0; i--)
	{
		if ((x.t0 & (1 << (i - 1))) > 0)
		{
			if ((x.t1 & (1 << (i - 1))) > 0)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
	}
	return 0;
}

/* Операция OR trs */
trs_t or_trs(trs_t x, trs_t y)
{
	trs_t r;
	int8_t i, j;
	int8_t a, b, s;

	x.l = min(x.l, SIZE_TRITS_MAX);
	x.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);
	x.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);

	y.l = min(y.l, SIZE_TRITS_MAX);
	y.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);
	y.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);

	if (x.l >= y.l)
	{
		j = x.l;
	}
	else
	{
		j = y.l;
	}

	r.l = j;

	for (i = 0; i < j; i++)
	{
		a = get_trit(x, i);
		b = get_trit(y, i);
		or_t(&a, &b, &s);
		r = set_trit(x, i, s);
	}

	return r;
}
/* Операции AND trs */
trs_t and_trs(trs_t x, trs_t y)
{
	trs_t r;

	int8_t i, j, ll;
	int8_t a, b, s;

	x.l = min(x.l, SIZE_TRITS_MAX);
	x.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);
	x.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);

	y.l = min(y.l, SIZE_TRITS_MAX);
	y.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);
	y.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);

	if (x.l >= y.l)
	{
		j = y.l;
		ll = x.l;
	}
	else
	{
		j = x.l;
		ll = y.l;
	}

	for (i = 0; i < j; i++)
	{
		a = get_trit(x, i);
		b = get_trit(y, i);
		and_t(&a, &b, &s);
		r = set_trit(x, i, s);
	}
	r.l = ll;
	return r;
}

/* Операции XOR trs */
trs_t xor_trs(trs_t x, trs_t y)
{
	trs_t r;

	int8_t i, j, ll;
	int8_t a, b, s;

	x.l = min(x.l, SIZE_TRITS_MAX);
	x.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);
	x.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);

	y.l = min(y.l, SIZE_TRITS_MAX);
	y.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);
	y.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);

	if (x.l >= y.l)
	{
		j = y.l;
		ll = x.l;
	}
	else
	{
		j = x.l;
		ll = y.l;
	}

	for (i = 0; i < j; i++)
	{
		a = get_trit(x, i);
		b = get_trit(y, i);
		xor_t(&a, &b, &s);
		r = set_trit(x, i, s);
	}
	r.l = ll;
	return r;
}

/* Операции Setun XOR trs */
trs_t xor_setun_trs(trs_t x, trs_t y)
{
	trs_t r;

	int8_t i, j, ll;
	int8_t a, b, s;

	x.l = min(x.l, SIZE_TRITS_MAX);
	x.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);
	x.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);

	y.l = min(y.l, SIZE_TRITS_MAX);
	y.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);
	y.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);

	if (x.l >= y.l)
	{
		j = y.l;
		ll = x.l;
	}
	else
	{
		j = x.l;
		ll = y.l;
	}

	for (i = 0; i < j; i++)
	{
		a = get_trit(x, i);
		b = get_trit(y, i);
		xor_t(&a, &b, &s);
		r = set_trit(x, i, s);
	}
	r.l = ll;
	return r;
}

/* Операция NOT trs */
trs_t not_trs(trs_t x)
{

	trs_t r = x;
	x.l = min(x.l, SIZE_TRITS_MAX);
	r.l = x.l;

	r.t1 = ~r.t1;
	r.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - r.l);
	r.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - r.l);

	return r;
}

/* Троичное NEG отрицания тритов */
trs_t neg_trs(trs_t t)
{
	return not_trs(t);
}

/* Троичный INC trs */
void inc_trs(trs_t *t)
{
	trs_t x;
	trs_t y;

	y = *t;
	y.l = t->l;

	x.t1 = 0;
	x.t0 = 0;
	x.l = 1;
	x = set_trit(x, 0, 1);
	*t = add_trs(y, x);
}

/* Операция DEC trs */
void dec_trs(trs_t *t)
{
	trs_t x;
	trs_t r;
	x = *t;
	x.t1 = 0;
	x.t0 = 0;
	x.l = t->l;
	x = set_trit(x, 0, -1);
	r = add_trs(*t, x);

	t->l = r.l;
	t->t1 = r.t1;
	t->t0 = r.t0;
}

/* Троичное сложение тритов */
trs_t add_trs(trs_t x, trs_t y)
{
	int8_t i, j;
	int8_t a, b, s, p0, p1;
	trs_t r;

	/* Результат для Сетунь-1958 R,S */
	uint8_t m = 0;

	x.l = min(x.l, SIZE_TRITS_MAX);
	x.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);
	x.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - x.l);

	y.l = min(y.l, SIZE_TRITS_MAX);
	y.t1 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);
	y.t0 &= 0xFFFFFFFF >> (SIZE_TRITS_MAX - y.l);

	if (x.l >= y.l)
	{
		j = x.l;
	}
	else
	{
		j = y.l;
	}

	r.l = j;
	r.t1 = 0;
	r.t0 = 0;

	p0 = 0;
	p1 = 0;

	for (i = 0; i < j; i++)
	{
		a = get_trit(x, i);
		b = get_trit(y, i);
		sum_t(&a, &b, &p0, &s, &p1);
		//
		if (s > 0)
		{
			r.t1 |= (1 << m);
			r.t0 |= (1 << m);
		}
		else if (s < 0)
		{
			r.t1 &= ~(1 << m);
			r.t0 |= (1 << m);
		}
		else
		{
			r.t1 &= ~(1 << m);
			r.t0 &= ~(1 << m);
		}
		m += 1;
		p0 = p1;
		p1 = 0;
	}

	/* Установить трит переполнения */
	/* ph1 */
	if (r.t0 & (1 << 19) > 0)
	{
		if (r.t1 & (1 << 19) > 0)
		{
			set_trit_setun(ph1, 1, 1);
		}
		else
		{
			set_trit_setun(ph1, 1, -1);
		}
	}
	else
	{
		set_trit_setun(ph1, 1, 0);
	}

	/* Установить трит переполнения */
	/* ph2 */
	if (r.t0 & (1 << 18) > 0)
	{
		if (r.t1 & (1 << 18) > 0)
		{
			set_trit_setun(ph2, 1, 1);
		}
		else
		{
			set_trit_setun(ph2, 1, -1);
		}
	}
	else
	{
		set_trit_setun(ph2, 1, 0);
	}

	/* результат */
	r.l = j;

	return r;
}

/* Троичное сложение тритов для длинных слов */
long_trs_t add_long_trs(long_trs_t xx, long_trs_t yy)
{
	int8_t i, j;
	int8_t a, b, s, p0, p1;
	long_trs_t rr;

	uint8_t m = 0;

	xx.l = min(xx.l, SIZE_LONG_TRITS_MAX);
	yy.l = min(yy.l, SIZE_LONG_TRITS_MAX);
	if (xx.l >= yy.l)
	{
		j = xx.l;
	}
	else
	{
		j = yy.l;
	}

	rr.l = j;
	rr.t1 = 0;
	rr.t0 = 0;

	p0 = 0;
	p1 = 0;

	for (i = 0; i < j; i++)
	{
		a = get_long_trit(xx, i);
		b = get_long_trit(yy, i);
		sum_t(&a, &b, &p0, &s, &p1);
		//
		if (s > 0)
		{
			rr.t1 = rr.t1 | (uint64_t)(1) << m;
			rr.t0 = rr.t0 | (uint64_t)(1) << m;
		}
		else if (s < 0)
		{
			rr.t1 = rr.t1 & ~((uint64_t)(1) << m);
			rr.t0 = rr.t0 | (uint64_t)(1) << m;
		}
		else
		{
			rr.t1 = rr.t1 & ~((uint64_t)(1) << m);
			rr.t0 = rr.t0 & ~((uint64_t)(1) << m);
		}
		m += 1;
		p0 = p1;
		p1 = 0;
	}

	if (p1 != 0)
	{
		if (s > 0)
		{
			rr.t1 |= 1 << (j + 1);
			rr.t0 |= 1 << (j + 1);
		}
		else if (s < 0)
		{
			rr.t1 &= ~(1 << (j + 1));
			rr.t0 |= 1 << (j + 1);
		}
		else
		{
			rr.t1 &= ~(1 << (j + 1));
			rr.t0 &= ~(1 << (j + 1));
		}
	}

	/* Результат */
	rr.t1 &= ~((~(uint64_t)(0)) << (j + 1));
	rr.t0 &= ~((~(uint64_t)(0)) << (j + 1));
	rr.l = j + 1;

	return rr;
}

/* Троичное вычитание тритов */
trs_t sub_trs(trs_t x, trs_t y)
{
	int8_t i, j;
	int8_t a, b, s, p0, p1;
	trs_t r;

	/* Результат для Сетунь-1958 R,S */
	uint8_t m = 0;

	x.l = min(x.l, SIZE_TRITS_MAX);
	y.l = min(y.l, SIZE_TRITS_MAX);
	if (x.l >= y.l)
	{
		j = x.l;
	}
	else
	{
		j = y.l;
	}

	r.l = j;
	r.t1 = 0;
	r.t0 = 0;

	p0 = 0;
	p1 = 0;

	for (i = 0; i < j; i++)
	{
		a = get_trit(x, i);
		b = get_trit(y, i);
		b = -b;
		sum_t(&a, &b, &p0, &s, &p1);
		//
		if (s > 0)
		{
			r.t1 |= 1 << m;
			r.t0 |= 1 << m;
		}
		else if (s < 0)
		{
			r.t1 &= ~(1 << m);
			r.t0 |= 1 << m;
		}
		else
		{
			r.t1 &= ~(1 << m);
			r.t0 &= ~(1 << m);
		}
		m += 1;
		p0 = p1;
		p1 = 0;
	}

	/* Установить трит переполнения */
	/* ph1 */
	if (r.t0 & (1 << 19) > 0)
	{
		if (r.t1 & (1 << 19) > 0)
		{
			set_trit(ph1, 1, 1);
		}
		else
		{
			set_trit(ph1, 1, -1);
		}
	}
	else
	{
		set_trit(ph1, 1, 0);
	}

	/* Установить трит переполнения */
	/* ph2 */
	if (r.t0 & (1 << 18) > 0)
	{
		if (r.t1 & (1 << 18) > 0)
		{
			set_trit_setun(ph2, 1, 1);
		}
		else
		{
			set_trit_setun(ph2, 1, -1);
		}
	}
	else
	{
		set_trit_setun(ph2, 1, 0);
	}

	/* результат */
	r.t1 = r.t1 & ~(0xFFFC0000); /* t[1.18] */
	r.t0 = r.t0 & ~(0xFFFC0000); /* t[1.18] */
	r.l = j;

	return r;
}

/* Троичное сложение тритов для длинных слов */
long_trs_t sub_long_trs(long_trs_t xx, long_trs_t yy)
{
	int8_t i, j;
	int8_t a, b, s, p0, p1;
	long_trs_t rr;

	uint8_t m = 0;

	xx.l = min(xx.l, SIZE_LONG_TRITS_MAX);
	yy.l = min(yy.l, SIZE_LONG_TRITS_MAX);
	if (xx.l >= yy.l)
	{
		j = xx.l;
	}
	else
	{
		j = yy.l;
	}

	rr.l = j;
	rr.t1 = 0;
	rr.t0 = 0;

	p0 = 0;
	p1 = 0;

	for (i = 0; i < j; i++)
	{
		a = get_long_trit(xx, i);
		b = -get_long_trit(yy, i);
		sum_t(&a, &b, &p0, &s, &p1);
		//
		if (s > 0)
		{
			rr.t1 = rr.t1 | (uint64_t)(1) << m;
			rr.t0 = rr.t0 | (uint64_t)(1) << m;
		}
		else if (s < 0)
		{
			rr.t1 = rr.t1 & ~((uint64_t)(1) << m);
			rr.t0 = rr.t0 | (uint64_t)(1) << m;
		}
		else
		{
			rr.t1 = rr.t1 & ~((uint64_t)(1) << m);
			rr.t0 = rr.t0 & ~((uint64_t)(1) << m);
		}
		m += 1;
		p0 = p1;
		p1 = 0;
	}

	if (p1 != 0)
	{
		if (s > 0)
		{
			rr.t1 |= 1 << (j + 1);
			rr.t0 |= 1 << (j + 1);
		}
		else if (s < 0)
		{
			rr.t1 &= ~(1 << (j + 1));
			rr.t0 |= 1 << (j + 1);
		}
		else
		{
			rr.t1 &= ~(1 << (j + 1));
			rr.t0 &= ~(1 << (j + 1));
		}
	}

	/* Результат */
	rr.t1 &= ~((~(uint64_t)(0)) << (j + 1));
	rr.t0 &= ~((~(uint64_t)(0)) << (j + 1));
	rr.l = j + 1;

	return rr;
}

/**
 * Операция сдвига тритов
 * Параметр:
 * if(d > 0) then "Влево"
 * if(d == 0) then "Нет сдвига"
 * if(d < 0) then "Вправо"
 * Возврат: Троичное число
 */
trs_t shift_trs(trs_t t, int8_t d)
{
	if (d > 0) /* сдвиг  */
	{
		t.t1 <<= d;
		t.t0 <<= d;
	}
	else if (d < 0)
	{
		t.t1 >>= -d;
		t.t0 >>= -d;
	}
	return t;
}

/* Троичное умножение тритов */
trs_t mul_trs(trs_t a, trs_t b)
{
	int8_t i;
	//
	int8_t s;
	int8_t l;
	trs_t r;

	long_trs_t aa;
	long_trs_t bb;
	long_trs_t rr;

	/* a => aa */
	aa.t1 = (uint64_t)a.t1;
	aa.t0 = (uint64_t)a.t0;
	aa.l = a.l;

	/* b => bb */
	bb.t1 = (uint64_t)b.t1;
	bb.t0 = (uint64_t)b.t0;
	bb.l = b.l;

	/* rr => 0 */
	rr.t1 = 0;
	rr.t0 = 0;

	/* Уменьшить количество операций */
	aa.l = min(aa.l, SIZE_WORD_LONG_LONG);
	bb.l = min(bb.l, SIZE_WORD_LONG_LONG);
	rr.l = SIZE_WORD_LONG_LONG;

	/* Умножение троичных числа */
	for (i = 0; i < bb.l; i++)
	{
		s = get_long_trit(bb, i);
		if (s > 0)
		{
			rr = add_long_trs(rr, shift_long_trs(aa, -i));
		}
		else if (s < 0)
		{
			rr = sub_long_trs(rr, shift_long_trs(aa, -i));
		}
		else
		{	/* s == 0 */
			/* no calculate */
		}
	}

	rr = shift_long_trs(rr, 18);
	rr.t1 &= ~((~(uint64_t)(0)) << SIZE_WORD_LONG);
	rr.t0 &= ~((~(uint64_t)(0)) << SIZE_WORD_LONG);

	/* b => bb */
	r.t1 = (uint32_t)rr.t1;
	r.t0 = (uint32_t)rr.t0;
	r.l = SIZE_WORD_LONG;
	return r;
}

/* Троичное деление тритов */
trs_t div_trs(trs_t a, trs_t b)
{
	trs_t r;
	r.l = 0;
	r.t1 = 0;
	r.t0 = 0;
	return r;
}

/* Оперция присваивания троичных чисел в регистры */
void copy_trs(trs_t *src, trs_t *dst)
{
	if (src->l == dst->l)
	{
		dst->t1 = src->t1;
		dst->t0 = src->t0;
	}
	else if (src->l > dst->l)
	{
		dst->t1 = src->t1;
		dst->t0 = src->t0 & (0xFFFFFFFF << dst->l);
	}
	else
	{
		dst->t1 = src->t1;
		dst->t0 = src->t0 & (0xFFFFFFFF >> src->l);
	}
}

/* Получить часть тритов из троичного числа */
trs_t slice_trs(trs_t t, int8_t p1, int8_t p2)
{
	trs_t r;

	if (t.l == 0)
	{
		return t; /* Error */
	}
	if (p1 > p2)
	{
		return t; /* Error */
	}
	if ((p2 - p1 + 1) > t.l)
	{
		return t; /* Error */
	}

	r = t;
	r.t1 >>= p1;
	r.t0 >>= p1;
	r.t1 &= (0xFFFFFFFF >> p1);
	r.t0 &= (0xFFFFFFFF >> p1);
	r.l = p2 - p1 + 1;

	return r;
}

/**  -------------------------------------------------------------
 *   Троичные числа регистров Setun-1958
 *
 *   SETUN-T1  = [s1]      - обозначение позиции тритов в регистре
 *   SETUN-T5  = [s1...s5]
 *   SETUN-T9  = [s1...s9]
 *   SETUN-T18 = [s1...s18]
 */

/* Получить целое со знаком трита в позиции троичного регистра */
int8_t get_trit_setun(trs_t t, uint8_t pos)
{
	t.l = min(t.l, SIZE_WORD_LONG);
	pos = min(pos, SIZE_WORD_LONG);
	if ((t.t0 & (1 << (t.l - pos))) > 0)
	{
		if ((t.t1 & (1 << (t.l - pos))) > 0)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

/* Установить трит в троичном регистре */
trs_t set_trit_setun(trs_t t, uint8_t pos, int8_t trit)
{
	trs_t r = t;

	t.l = min(t.l, SIZE_WORD_LONG);
	pos = min(pos, SIZE_WORD_LONG);
	if (trit > 0)
	{
		r.t1 |= (1 << (t.l - pos));
		r.t0 |= (1 << (t.l - pos));
	}
	else if (trit < 0)
	{
		r.t1 &= ~(1 << (t.l - pos));
		r.t0 |= (1 << (t.l - pos));
	}
	else
	{
		r.t1 &= ~(1 << (t.l - pos));
		r.t0 &= ~(1 << (t.l - pos));
	}
	return r;
}

/**
 * Получить часть тритов из троичного числа
 */
trs_t slice_trs_setun(trs_t t, int8_t p1, int8_t p2)
{
	trs_t r;

	if (t.l == 0)
	{
		return t; /* Error */
	}
	if (p1 > p2)
	{
		return t; /* Error */
	}
	if ((p2 - p1 + 1) > t.l)
	{
		return t; /* Error */
	}

	r = t;
	r.t1 >>= (t.l - p2);
	r.t0 >>= (t.l - p2);
	r.t1 &= (0xFFFFFFFF >> (t.l - p2));
	r.t0 &= (0xFFFFFFFF >> (t.l - p2));
	r.l = p2 - p1 + 1;

	return r;
}

/* Операция присваивания троичных чисел в регистры */
void copy_trs_setun(trs_t *src, trs_t *dst)
{
	if (src->l == dst->l)
	{
		dst->t1 = src->t1;
		dst->t0 = src->t0;
	}
	else if (src->l > dst->l)
	{
		int8_t s = src->l - dst->l;
		dst->t1 = src->t1 >> s;
		dst->t0 = src->t0 >> s;
		dst->t0 &= ~(0xFFFFFFFF << dst->l);
	}
	else
	{
		int8_t s = dst->l - src->l;
		dst->t1 = src->t1 << s;
		dst->t0 = src->t0 << s;
		dst->t0 &= ~(0xFFFFFFFF << dst->l);
	}
}

/* Проверить на переполнение 18-тритного числа */
int8_t over_check(void)
{
	if (get_trit_setun(ph1, 1) != 0)
	{
		return 1; /* OVER Error  */
	}
	else
	{
		return 0;
	}
}

/* Проверить на переполнение 18-тритного числа */
int8_t over_word_long(trs_t x)
{
	ph1 = set_trit_setun(ph1, 1, get_trit_setun(x, 1));
	ph2 = set_trit_setun(ph2, 1, get_trit_setun(x, 2));

	if (get_trit_setun(ph1, 1) != 0)
	{
		return 1; /* OVER Error  */
	}
	else
	{
		return 0;
	}
}

/* Преобразование трита в номер зоны */
int8_t trit2grfram(trs_t t)
{
	if ((t.t0 & (1 << 0)) > 0)
	{
		if ((t.t1 & (1 << 0)) > 0)
		{
			return 1;
		}
		else
		{
			return -1;
		}
	}
	return 0;
}

/* Дешифратор тритов в индекс строки зоны памяти */
int16_t addr_trit2addr_index(trs_t t)
{
	int8_t i;
	int8_t n;
	t.t1 &= (uint32_t)0x3FFFF;
	return t.t1 >>= 1;
}

/* Дешифратор тритов в индекс адреса памяти */
uint8_t zone_drum_to_index(trs_t z)
{
	int8_t r;

	r = trs2digit(z);

	if (r > NUMBER_ZONE_DRUM + ZONE_DRUM_BEG - 1)
	{
		r = NUMBER_ZONE_DRUM + ZONE_DRUM_BEG - 1;
	}
	else if (r < 0)
	{
		r = 0;
	}

	return r;
}

/* Дешифратор трита в индекс адреса физической памяти FRAM */
uint8_t addr2grfram(trs_t z)
{
	trs_t a = z;
	int8_t r;
	r = get_trit_setun(a, 1);
	if (r < 0)
	{
		return 0;
	}
	else if (r > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/* Дешифратор строки 9-тритов в зоне памяти FRAM */
uint8_t addr2row_fram(trs_t z)
{
	int8_t r = 40;
	r += get_trit_setun(z, 4) * 1;
	r += get_trit_setun(z, 3) * 3;
	r += get_trit_setun(z, 2) * 9;
	r += get_trit_setun(z, 1) * 27;
	return (uint8_t)r;
}
/* Дешифратор строки 9-тритов в зоне памяти FRAM */
trs_t rowzose2addr(uint8_t rind, uint8_t zind)
{
	trs_t r;
	int8_t dd = 40;
	rind -= 40;
	r.l = 5;
	r = set_trit_setun(r, 5, zind);
	// r += get_trit_setun(z, 4) * 1;
	// r += get_trit_setun(z, 3) * 3;
	// r += get_trit_setun(z, 2) * 9;
	// r += get_trit_setun(z, 1) * 27;
	return r;
}

/* Новый адрес кода машины */
trs_t next_address(trs_t c)
{
	trs_t r;
	int8_t trit;
	r = c;
	inc_trs(&r);
	trit = get_trit_setun(r, 5);
	if (trit < 0)
	{
		inc_trs(&r);
	}
	mod_3_n(&r, 5);
	r.l = 5;
	return r;
}

/* Новый адрес кода машины */
trs_t next_ind(trs_t c)
{
	trs_t r;
	r = c;
	if (get_trit(r, 5) == 0)
		/* 0 */
		inc_trs(&r);
	else if (get_trit(r, 5) >= 1)
	{
		/* + */
		inc_trs(&r);
		inc_trs(&r);
	}
	else if (get_trit(r, 5) <= -1)
	{
		/* - */
		inc_trs(&r);
	}
	return r;
}

void clean_fram_zone(trs_t z)
{

	int8_t zn;
	int8_t grfram;
	int8_t row;

	zn = get_trit_setun(z, 1);

	if (zn > 0)
	{
		grfram = 0;
	}
	else if (zn < 0)
	{
		grfram = 2;
	}
	else
	{
		grfram = 1;
	}
	for (row = 0; row < SIZE_GR_TRIT_FRAM; row++)
	{
		mem_fram[row][grfram].l = SIZE_WORD_SHORT;
		mem_fram[row][grfram].t1 = 0;
		mem_fram[row][grfram].t0 = 0;
	}
}

/* Операция очистить память ферритовую */
void clean_fram(void)
{
	int8_t grfram;
	int8_t row;
	for (grfram = 0; grfram < SIZE_GRFRAM; grfram++)
	{
		for (row = 0; row < SIZE_GR_TRIT_FRAM; row++)
		{
			mem_fram[row][grfram].l = SIZE_WORD_SHORT;
			mem_fram[row][grfram].t1 = 0;
			mem_fram[row][grfram].t0 = 0;
		}
	}
}

/* Операция очистить память на магнитном барабане */
void clean_drum(void)
{
	int8_t zone;
	int8_t row;
	for (zone = ZONE_DRUM_BEG; zone < NUMBER_ZONE_DRUM + ZONE_DRUM_BEG; zone++)
	{
		for (row = 0; row < SIZE_ZONE_TRIT_DRUM; row++)
		{
			mem_drum[zone][row].l = SIZE_WORD_SHORT;
			mem_drum[zone][row].t1 = 0;
			mem_drum[zone][row].t0 = 0;
		}
	}
}

/* Функция "Читать троичное число из ферритовой памяти" */
trs_t ld_fram(trs_t ea)
{
	uint8_t zind;
	uint8_t rind;
	int8_t eap5;
	trs_t zr;
	trs_t rr;
	trs_t rrr;
	trs_t res;

	/* Индекс строки в зоне памяти FRAM */
	rr = slice_trs_setun(ea, 1, 4);
	rind = addr2row_fram(rr);

	/* Зона памяти FRAM */
	zr = slice_trs_setun(ea, 5, 5);
	zind = addr2grfram(zr);

	res.t1 = 0;
	res.t0 = 0;
	res.l = 9;

	eap5 = get_trit_setun(ea, 5);
	if (eap5 < 0)
	{
		/* Прочитать 18-тритное число */
		/* прочитать 1...9 старшую часть 18-тритного числа */
		rrr = mem_fram[rind][0];
		rrr = shift_trs(rrr, 9);
		rrr.l = 18;
		mod_3_n(&rrr, rrr.l);

		/* прочитать 10...18 младшую часть 18-тритного числа */
		res = mem_fram[rind][1];
		res.l = 9;
		res = add_trs(res, rrr);
		res.l = 18;
		mod_3_n(&res, res.l);
	}
	else if (eap5 == 0)
	{
		/* Прочитать 1...9 старшую часть 18-тритного числа */
		res = mem_fram[rind][zind];
	}
	else
	{ /* eap5 > 0 */
		/* Прочитать 10...18 младшую часть 18-тритного числа */
		res = mem_fram[rind][zind];
	}
	return res;
}

/* Функция "Записи троичного числа в ферритовую память" */
void st_fram(trs_t ea, trs_t v)
{
	int8_t eap5;
	uint8_t rind, zind;
	trs_t rr, zr;
	trs_t s = v;

	/* Индекс строки в зоне физической памяти FRAM */
	rr = slice_trs_setun(ea, 1, 4);
	rind = addr2row_fram(rr);

	/* Зона физической памяти FRAM */
	zr = slice_trs_setun(ea, 5, 5);
	zind = addr2grfram(zr);

	eap5 = get_trit_setun(ea, 5);

	// viv+ dbg	printf(" ri=%d, zi=%d\r\n",rind,zind);

	if (eap5 < 0)
	{ /* Записать 18-тритное число */
		mem_fram[rind][0] = slice_trs_setun(s, 1, 9);
		mem_fram[rind][1] = slice_trs_setun(s, 10, 18);
	}
	else if (eap5 == 0)
	{ /* Записать 9-тритное число */
		mem_fram[rind][zind] = slice_trs_setun(s, 1, 9);
	}
	else
	{ /* eap5 > 0 */
		/* Записать 18-тритное число */
		mem_fram[rind][zind] = slice_trs_setun(s, 1, 9);
	}
}

/* Копировать страницу из память fram на магнитного барабана brum */
void fram_to_drum(trs_t ea)
{
	int8_t sng;
	trs_t fram_inc;
	trs_t k1;
	trs_t k2_k5;
	trs_t mr;

	/* Номер зоны FRAM */
	k1 = slice_trs_setun(ea, 1, 1);
	sng = trit2int(k1);

	/* Номер зоны DRUM */
	k2_k5 = slice_trs_setun(ea, 2, 5);

	/* Какая страница FRAM */
	if (sng < 0)
	{
		fram_inc = smtr("----0");
	}
	else if (sng == 0)
	{
		fram_inc = smtr("0---0");
	}
	else
	{
		fram_inc = smtr("+---0");
	}

	/* Копировать страницу */
	for (uint8_t m = 0; m < SIZE_ZONE_TRIT_FRAM; m++)
	{
		mr = ld_fram(fram_inc);
		st_drum(k2_k5, m, mr);
		fram_inc = next_address(fram_inc);
	}
}

/*  Операция чтения в память магнитного барабана */
trs_t ld_drum(trs_t ea, uint8_t ind)
{
	uint8_t zind;
	trs_t zr;
	trs_t rr;
	trs_t res;

	zr = slice_trs_setun(ea, 1, 4);
	zr.l = 4;
	zind = zone_drum_to_index(zr);

	if (ind > SIZE_ZONE_TRIT_DRUM - 1)
	{
		ind = SIZE_ZONE_TRIT_DRUM - 1;
	}
	else if (ind < 0)
	{
		ind = 0;
	}
	res.l = 9;
	copy_trs_setun(&mem_drum[zind][ind], &res);
	return res;
}

/* Операция записи в память магнитного барабана */
void st_drum(trs_t ea, uint8_t ind, trs_t v)
{
	uint8_t zind;
	uint8_t rind;
	trs_t zr;
	trs_t rr;

	zr = slice_trs_setun(ea, 1, 4);
	zr.l = 4;
	zind = zone_drum_to_index(zr);

	if (ind > SIZE_ZONE_TRIT_DRUM - 1)
	{
		ind = SIZE_ZONE_TRIT_DRUM - 1;
	}
	copy_trs_setun(&v, &mem_drum[zind][ind]);
}

/* Копировать страницу с магнитного барабана в память fram */
void drum_to_fram(trs_t ea)
{
	int8_t sng;
	trs_t zram;
	trs_t fram_inc;

	trs_t k1;
	trs_t k2_k5;
	trs_t mr;

	fram_inc.l = 4;
	fram_inc.t1 = 0;
	fram_inc.t0 = 0;

	/* Номер зоны FRAM */
	k1 = slice_trs_setun(ea, 1, 1);
	sng = trit2int(k1);

	/* Номер зоны DRUM */
	k2_k5 = slice_trs_setun(ea, 2, 5);
	k2_k5.l = 4;

	if (sng < 0)
	{
		fram_inc = smtr("----0");
	}
	else if (sng == 0)
	{
		fram_inc = smtr("0---0");
	}
	else
	{
		fram_inc = smtr("+---0");
	}

	/* Копировать страницу */
	for (uint8_t m = 0; m < SIZE_ZONE_TRIT_FRAM; m++)
	{
		mr = ld_drum(k2_k5, m);
		st_fram(fram_inc, mr);
		fram_inc = next_address(fram_inc);
	}
}

/** ***********************************************
 *  Алфавит троичной симметричной системы счисления
 *  -----------------------------------------------
 *  Троичный код в девятеричной системы
 *  -----------------------------------------------
 *  С символами: W, X, Y, Z, 0, 1, 2, 3, 4.
 */
uint8_t trit2lt(int8_t v)
{
	switch (v)
	{
	case -4:
		return 'W';
		break;
	case -3:
		return 'X';
		break;
	case -2:
		return 'Y';
		break;
	case -1:
		return 'Z';
		break;
	case 0:
		return '0';
		break;
	case 1:
		return '1';
		break;
	case 2:
		return '2';
		break;
	case 3:
		return '3';
		break;
	case 4:
		return '4';
		break;
	default:
		return '0';
		break;
	}
}

/**
 *  Вид '-','0','+' в десятичный код
 */
int8_t symtrs2numb(uint8_t c)
{
	switch (c)
	{
	case '-':
		return -1;
		break;
	case '0':
		return 0;
		break;
	case '+':
		return 1;
		break;
	default:
		return 0;
		break;
	}
}

/**
 *  Bp  Вид '-','0','+'
 */
uint8_t numb2symtrs(int8_t v)
{

	if (v <= -1)
	{
		return '-';
	}
	else if (v == 0)
	{
		return '0';
	}
	else
	{
		return '+';
	}
}

/**
 *  Вид '-','0','+' строка в десятичный код
 */
int8_t str_symtrs2numb(uint8_t *s)
{
	int8_t i;
	int8_t len;
	int8_t res = 0;

	len = strlen(s);

	for (i = 0; i < len; i++)
	{
		res += symtrs2numb(s[i]) * pow3(i);
		printf("%c", s[i]);
	}

	return res;
}

/**
 * Cтроку вида {'-','0','+'} в троичное число
 */
trs_t smtr(uint8_t *s)
{
	int8_t i;
	int8_t trit;
	int8_t len;
	trs_t t;

	len = strlen(s);
	len = min(len, SIZE_TRITS_MAX);
	t.l = len;
	t.t1 = 0;
	t.t0 = 0;
	for (i = 0; i < len; i++)
	{
		trit = symtrs2numb(*(s + len - i - 1));
		t = set_trit(t, i, trit);
	}
	return t;
}

/**
 *  Девятеричный вид в троичный код
 */
uint8_t *lt2symtrs(int8_t v)
{
	switch (v)
	{
	case 'W':
	case 'w':
		return "--";
		break;
	case 'X':
	case 'x':
		return "-0";
		break;
	case 'Y':
	case 'y':
		return "-+";
		break;
	case 'Z':
	case 'z':
		return "0-";
		break;
	case '0':
		return "00";
		break;
	case '1':
		return "0+";
		break;
	case '2':
		return "+-";
		break;
	case '3':
		return "+0";
		break;
	case '4':
		return "++";
		break;
	default:
		return "  ";
		break;
	}
}

/**
 *  Троичный код в девятеричной системы
 *
 *  С символами: W, X, Y, Z, 0, 1, 2, 3, 4.
 */
uint8_t trit_to_lt(int8_t v)
{
	switch (v)
	{
	case -4:
		return 'W';
		break;
	case -3:
		return 'X';
		break;
	case -2:
		return 'Y';
		break;
	case -1:
		return 'Z';
		break;
	case 0:
		return '0';
		break;
	case 1:
		return '1';
		break;
	case 2:
		return '2';
		break;
	case 3:
		return '3';
		break;
	case 4:
		return '4';
		break;
	default:
		return '0';
		break;
	}
}

trs_t digit2trs(int8_t n)
{
	trs_t r;
	if (n < 3)
	{
		// printf("%d",n);
	}
	else
	{
		digit2trs(n / 3);
		// printf("%d",n%3);
	}
	return r;
}

/**
 * Троичный код в девятеричной системы
 */
int32_t trs2digit(trs_t t)
{
	uint8_t i;
	int32_t l = 0;
	for (i = 0; i < t.l; i++)
	{
		l += get_trit(t, i) * pow3(i);
	}
	return l;
}

/**
 * Девятеричный вид в троичный код
 */
void cmd_str_2_trs(uint8_t *syms, trs_t *r)
{
	uint8_t i;
	uint8_t symtrs_str[40];

	if (strlen(syms) != 5)
	{
		r->l = 9;
		r->t1 = 0;
		r->t0 = 0;
		printf(" --- ERROR syms\r\n");
		return;
	}

	sprintf(symtrs_str, "%2s%2s%2s%2s%2s",
			lt2symtrs(syms[0]),
			lt2symtrs(syms[1]),
			lt2symtrs(syms[2]),
			lt2symtrs(syms[3]),
			lt2symtrs(syms[4]));

	for (i = 1; i < 10; i++)
	{
		*r = set_trit_setun(*r, i, symtrs2numb(symtrs_str[i]));
	}
}

/**
 * Печать троичного числа в строку
 */
void trs2str(trs_t t)
{
	int8_t i, j, n;
	int8_t t0, t1;

	n = t.l;
	i = t.l - 1;
	if (n % 2)
	{
		t1 = 0;
		t0 = get_trit(t, i);
		printf("%c", trit2lt(t1 * 3 + t0));
		n -= 1;
		i -= 1;
	}

	if (t.l == 1)
		return;

	while (1)
	{
		t1 = get_trit(t, i);
		t0 = get_trit(t, i - 1);
		printf("%c", trit2lt(t1 * 3 + t0));
		n -= 2;
		i -= 2;
		if (i < 0)
			break;
	}
}

/**
 * Печать троичного числа в строку
 */
void long_trs2str(long_trs_t t)
{
	int8_t i, j, n;
	int8_t t0, t1;

	n = t.l;
	i = t.l - 1;
	if (n % 2)
	{
		t1 = 0;
		t0 = get_long_trit(t, i);
		printf("%c", trit2lt(t1 * 3 + t0));
		n -= 1;
		i -= 1;
	}

	if (t.l == 1)
		return;

	while (1)
	{
		t1 = get_long_trit(t, i);
		t0 = get_long_trit(t, i - 1);
		printf("%c", trit2lt(t1 * 3 + t0));
		n -= 2;
		i -= 2;
		if (i < 0)
			break;
	}
}

/**
 * Печать троичного числа в строку
 */
void trit2symtrs(trs_t t)
{

	int8_t i, j, n;
	int8_t t0;
	trs_t x;

	n = t.l;

	for (i = 0; i < n; i++)
	{
		x = t;
		x.t1 >>= (n - 1 - i);
		t0 = trit2int(x);
		printf("%c", numb2symtrs(t0));
	}
	return;
}

/**
 * Печать троичного регистра
 *
 */
void view_short_reg(trs_t *t, uint8_t *ch)
{
	int8_t i;
	int8_t l;
	int8_t trit;
	trs_t tv = *t;

	printf("%s: ", (char *)ch);
	if (tv.l <= 0)
	{
		printf("\n");
		return;
	}

	l = min(tv.l, SIZE_TRITS_MAX);
	printf("[");
	// printf("\nt1 % 8x\n",t->t1);
	// printf("t2 % 8x\n",t->t0);
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_trit(tv, l - 1 - i);
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");
	//
	tv = *t;
	trs2str(tv);
	printf(", "); //
	printf("(%li)", (long int)trs2digit(*t));
#if 0	
	printf(", {");
	for (i = 0; i < l; i++)	{
		tv = *t;
		trit = get_trit(tv, l - 1 - i);
		printf("%i", trit);
	}
	printf("}");
#endif
	printf("\r\n");
}

/**
 * Печать троичного регистра
 *
 */
void view_step_short_reg(trs_t *t, uint8_t *ch)
{
	int8_t i;
	int8_t l;
	int8_t trit;
	trs_t tv = *t;

	printf("%s: ", (char *)ch);
	if (tv.l <= 0)
	{
		printf("\r\n");
		return;
	}

	l = min(tv.l, SIZE_TRITS_MAX);
	printf("[");
	// printf("\r\nt1 % 8x\r\n",t->t1);
	// printf("t2 % 8x\r\n",t->t0);
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_trit(tv, l - 1 - i);
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");
	//
	tv = *t;
	trs2str(tv);
	printf(", "); //
	printf("(%li)", (long int)trs2digit(*t));
#if 0	
	printf(", {");
	for (i = 0; i < l; i++)	{
		tv = *t;
		trit = get_trit(tv, l - 1 - i);
		printf("%i", trit);
	}
	printf("}");
#endif
	printf("\t\t-> zone[% 2i] : ", get_trit_setun(tv, 1));

	if (get_trit_setun(tv, 5) < 0)
	{
		tv = set_trit_setun(tv, 5, 0);
		MR = ld_fram(tv);
		trs2str(MR);
		printf(" ");
		tv = set_trit_setun(tv, 5, 1);
		MR = ld_fram(tv);
		trs2str(MR);
		// printf("\r\n");
		tv = set_trit_setun(tv, 5, -1);
		MR = ld_fram(tv);
	}
	else
	{
		MR = ld_fram(tv);
		trs2str(MR);
		// printf("\r\n");
	}

	printf(", ");
	printf("(%li)", (long int)trs2digit(MR));
	printf("\r\n");

	//
	// tv = next_address(tv);
	// MR = ld_fram(tv);
	// printf("                                \t");
	// trs2str(MR);
	//
	// printf("\r\n");
}

/**
 * Печать троичного регистра
 *
 */
void view_step_new_addres(trs_t *t, uint8_t *ch)
{
	int8_t i;
	int8_t l;
	int8_t trit;
	trs_t tv = *t;

	printf("%s: ", (char *)ch);
	if (tv.l <= 0)
	{
		printf("\r\n");
		return;
	}

	l = min(tv.l, SIZE_TRITS_MAX);
	printf("[");
	// printf("\r\nt1 % 8x\r\n",t->t1);
	// printf("t2 % 8x\r\n",t->t0);
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_trit(tv, l - 1 - i);
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");
	//
	tv = *t;
	trs2str(tv);
	printf(", "); //
	printf("(%li)", (long int)trs2digit(*t));

	printf("\t\t-> zone[% 2i] : ", get_trit_setun(tv, 1));

	if (get_trit_setun(tv, 5) < 0)
	{

		tv = set_trit_setun(tv, 5, 0);
		MR = ld_fram(tv);
		trs2str(MR);
		printf(" ");
		tv = set_trit_setun(tv, 5, 1);
		MR = ld_fram(tv);
		trs2str(MR);
		// printf("\r\n");
		tv = set_trit_setun(tv, 5, -1);
		MR = ld_fram(tv);
	}
	else
	{
		MR = ld_fram(tv);
		trs2str(MR);
		// printf("\r\n");
	}
	printf(", ");
	printf("(%li)", (long int)trs2digit(MR));
	printf("\r\n");

#if 0	
	printf(", {");
	for (i = 0; i < l; i++)	{
		tv = *t;
		trit = get_trit(tv, l - 1 - i);
		printf("%i", trit);
	}
	printf("}");
#endif
}

/**
 * Печать троичного регистра
 *
 */
void view_short_long_reg(long_trs_t *t, uint8_t *ch)
{
	int8_t i;
	int8_t l;
	int8_t trit;
	long_trs_t tv = *t;

	printf("%s: ", (char *)ch);
	if (tv.l <= 0)
	{
		printf("\r\n");
		return;
	}

	l = min(tv.l, SIZE_LONG_TRITS_MAX);
	printf("[");
	// printf("\r\nt1 %p\r\n",(* long unsigned int)t->t1);
	// printf("t2 %p\r\n",(* long unsigned int)t->t0);
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_long_trit(tv, l - 1 - i);
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");
	//
	tv = *t;
	long_trs2str(tv);
	printf(", "); //
	// printf("(%li)", (long int)trs2digit(*t));
	printf(", {");
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_long_trit(tv, l - 1 - i);
		printf("%i", trit);
	}
	printf("}");
	//
	printf("\r\n");
}

/**
 * Печать контрольных сумм
 */
void view_checksum_setun(trs_t t)
{
	trs_t check;
	trs_t ncheck;

	printf("KC:\r\n");
	//
	check = slice_trs_setun(t, 1, 9);
	view_short_reg(&check, "");
	//
	check = slice_trs_setun(t, 10, 18);
	view_short_reg(&check, "");

	printf("-KC = 0-KC:\r\n");
	check = neg_trs(t);
	ncheck = slice_trs_setun(check, 1, 9);
	ncheck.l = 9;
	view_short_reg(&ncheck, "");
	//
	check = slice_trs_setun(check, 10, 18);
	check.l = 9;
	view_short_reg(&check, "");
	//
	printf("\r\n");
}

/**
 * Печать регистров машины Сетунь-1958
 */
void view_short_regs(void)
{
	int8_t i;

	// printf("[Registers Setun-1958]\r\n");
	printf("\r\n");
	view_short_reg(&K, "  K  ");
	view_short_reg(&F, "  F  ");
	view_short_reg(&C, "  C  ");
	view_short_reg(&W, "  W  ");
	view_short_reg(&ph1, "  ph1");
	view_short_reg(&ph2, "  ph2");
	view_short_reg(&S, "  S  ");
	view_short_reg(&R, "  R  ");
	view_short_reg(&MB, "  MB ");
}

/**
 * Печать памяти FRAM машины Сетунь-1958
 */
void view_elem_fram(trs_t ea)
{
	int8_t j;
	trs_t tv;

	uint8_t zind;
	uint8_t rind;
	int8_t eap5;
	trs_t zr;
	trs_t rr;
	trs_t r;

	/* Зона памяти FRAM */
	zr = slice_trs_setun(ea, 5, 5);
	zind = addr2grfram(zr);

	/* Индекс строки в зоне памяти FRAM */
	rr = slice_trs_setun(ea, 1, 4);
	rind = addr2row_fram(rr);

	r = mem_fram[rind][zind];

	printf("fram[");
	for (j = 1; j < 6; j++)
	{
		printf("%c", numb2symtrs(get_trit_setun(ea, j)));
	}
	printf("] (%3d:%2d) = ", rind - SIZE_GR_TRIT_FRAM / 2, zind);

	printf("[");
	for (j = 1; j < 10; j++)
	{
		printf("%c", numb2symtrs(get_trit_setun(r, j)));
	}
	printf("], ");
	trs2str(r);
	printf("\r\n");
}

void view_fram(trs_t addr1, trs_t addr2)
{
	trs_t ad1 = addr1;
	trs_t ad2 = addr2;
	int16_t a1 = trs2digit(ad1);
	int16_t a2 = trs2digit(ad2);

	if ((a2 >= a1) && (a2 >= ZONE_M_FRAM_BEG && a2 <= ZONE_P_FRAM_END) && (a2 >= ZONE_M_FRAM_BEG && a2 <= ZONE_P_FRAM_END))
	{
		for (uint16_t i = 0; i < (abs(a2 - a1) + 1); i++)
		{
			// inc_trs(&ad1);
			if (trit2int(ad1) < 0)
			{
				inc_trs(&ad1);
				i += 1;
			}
			view_elem_fram(ad1);
			inc_trs(&ad1);
		}
	}
}

/**
 * Печать памяти FRAM машины Сетунь-1958
 */
void dump_fram(void)
{
	int8_t grfram;
	int8_t row;
	int8_t j;
	trs_t tv;
	trs_t r;

	printf("\r\n[ Dump FRAM Setun-1958: ]\r\n");

	for (row = 0; row < SIZE_GR_TRIT_FRAM; row++)
	{
		for (grfram = 0; grfram < SIZE_GRFRAM; grfram++)
		{
			r = mem_fram[row][grfram];
			r.l = 9;
			// viv+ dbg view_short_reg(&r,"r");

			printf("fram[.] (%3d:%2d) : ", row - SIZE_GR_TRIT_FRAM / 2, grfram);
			// printf("%s",trs2str());
			printf(" [");
			for (j = 1; j < 10; j++)
			{
				printf("%c", numb2symtrs(get_trit_setun(r, j)));
			}
			printf("], ");

			trs2str(r);
			printf("\r\n");
		}
	}
}

/**
 * Печать дампа зоны FRAM машины Сетунь-1958
 * по шаблону из книг математического обеспечения
 */
void dump_fram_zone(trs_t z)
{
	int8_t grfram;
	int8_t row;
	int8_t j;
	int8_t sng = 0;

	trs_t fram_row1;
	trs_t fram_row2;

	trs_t tv;
	trs_t r;

	trs_t ksum;

	printf("\r\n[ Dump FRAM Setun-1958: ]\n");

	sng = get_trit_setun(z, 1);

	/* Какая страница FRAM */
	if (sng < 0)
	{
		fram_row1 = smtr("-----");
		fram_row2 = smtr("-00+-");
	}
	else if (sng == 0)
	{
		fram_row1 = smtr("0----");
		fram_row2 = smtr("000+-");
	}
	else
	{
		fram_row1 = smtr("+----");
		fram_row2 = smtr("+00+-");
	}

	/* print FRAM */
	clear(&ksum);
	ksum.l = 18;

	trs_t inr = fram_row1;
	inr = set_trit_setun(inr, 5, 0);
	for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
	{
		tv = ld_fram(inr);
		ksum = add_trs(ksum, tv);
		inr = next_address(inr);
	}

	printf("Zone =% 2i\r\n", sng);
	printf("\r\n");

	for (uint8_t i = 0; i < 14; i++)
	{
		/* Print ROW1 */
		// addr -1
		r = slice_trs_setun(fram_row1, 2, 3);
		trs2str(r);
		r = slice_trs_setun(fram_row1, 4, 5);
		trs2str(r);
		printf(" ");
		// addr 0
		fram_row1 = set_trit_setun(fram_row1, 5, 0);
		r = slice_trs_setun(fram_row1, 2, 3);
		trs2str(r);
		r = slice_trs_setun(fram_row1, 4, 5);
		trs2str(r);
		printf("  ");
		// addr 1
		tv = ld_fram(fram_row1);
		r = slice_trs_setun(tv, 1, 1);
		trs2str(r);
		printf(" ");
		r = slice_trs_setun(tv, 2, 3);
		trs2str(r);
		r = slice_trs_setun(tv, 4, 5);
		trs2str(r);
		printf(" ");
		r = slice_trs_setun(tv, 6, 7);
		trs2str(r);
		r = slice_trs_setun(tv, 8, 9);
		trs2str(r);

		printf("        ");
		if (i <= 12)
		{
			/* print ROW2 */
			r = slice_trs_setun(fram_row2, 2, 3);
			trs2str(r);
			r = slice_trs_setun(fram_row2, 4, 5);
			trs2str(r);
			printf(" ");

			fram_row2 = set_trit_setun(fram_row2, 5, 0);
			r = slice_trs_setun(fram_row2, 2, 3);
			trs2str(r);
			r = slice_trs_setun(fram_row2, 4, 5);
			trs2str(r);
			printf("  ");

			tv = ld_fram(fram_row2);
			r = slice_trs_setun(tv, 1, 1);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(tv, 2, 3);
			trs2str(r);
			r = slice_trs_setun(tv, 4, 5);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(tv, 6, 7);
			trs2str(r);
			r = slice_trs_setun(tv, 8, 9);
			trs2str(r);
			printf(" ");
			printf("\r\n");
		}
		else
		{
			printf("KC     ");
			r = slice_trs_setun(ksum, 1, 1);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(ksum, 2, 3);
			trs2str(r);
			r = slice_trs_setun(ksum, 4, 5);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(ksum, 6, 7);
			trs2str(r);
			r = slice_trs_setun(ksum, 8, 9);
			trs2str(r);
			printf("\r\n");
		}
		/* print ROW1 */
		printf("   ");
		fram_row1 = set_trit_setun(fram_row1, 5, 1);
		r = slice_trs_setun(fram_row1, 2, 3);
		trs2str(r);
		r = slice_trs_setun(fram_row1, 4, 5);
		trs2str(r);
		printf("  ");
		//
		//
		tv = ld_fram(fram_row1);
		r = slice_trs_setun(tv, 1, 1);
		trs2str(r);
		printf(" ");
		r = slice_trs_setun(tv, 2, 3);
		trs2str(r);
		r = slice_trs_setun(tv, 4, 5);
		trs2str(r);
		printf(" ");
		r = slice_trs_setun(tv, 6, 7);
		trs2str(r);
		r = slice_trs_setun(tv, 8, 9);
		trs2str(r);
		fram_row1 = set_trit_setun(fram_row1, 5, -1);

		printf("        ");

		if (i <= 12)
		{
			/* print ROW2 */
			printf("   ");
			fram_row2 = set_trit_setun(fram_row2, 5, 1);
			r = slice_trs_setun(fram_row2, 2, 3);
			trs2str(r);
			r = slice_trs_setun(fram_row2, 4, 5);
			trs2str(r);
			printf("  ");
			//
			tv = ld_fram(fram_row2);
			r = slice_trs_setun(tv, 1, 1);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(tv, 2, 3);
			trs2str(r);
			r = slice_trs_setun(tv, 4, 5);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(tv, 6, 7);
			trs2str(r);
			r = slice_trs_setun(tv, 8, 9);
			trs2str(r);
			fram_row2 = set_trit_setun(fram_row2, 5, -1);
			printf("\r\n");
		}
		else
		{
			printf("       ");
			r = slice_trs_setun(ksum, 10, 10);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(ksum, 11, 12);
			trs2str(r);
			r = slice_trs_setun(ksum, 13, 14);
			trs2str(r);
			printf(" ");
			r = slice_trs_setun(ksum, 15, 16);
			trs2str(r);
			r = slice_trs_setun(ksum, 17, 18);
			trs2str(r);
			printf("\r\n");
		}

		/* Next address*/
		inc_trs(&fram_row1);
		inc_trs(&fram_row1);
		inc_trs(&fram_row1);
		inc_trs(&fram_row2);
		inc_trs(&fram_row2);
		inc_trs(&fram_row2);
	}
}

/**
 * Печать короткого слова BRUM машины Сетунь-1958
 */
void view_drum_zone(trs_t zone)
{
	int8_t j;
	trs_t zr;
	uint8_t zind;
	uint8_t rind;
	trs_t tv;

	/* Зона памяти DRUM */
	zr = slice_trs_setun(zone, 1, 4);
	zind = zone_drum_to_index(zr);

	printf("\r\n[ Dump DRUM Setun-1958: ]\r\n");
	printf("[ Zone = %2i ]\r\n", zind);
	printf("\r\n");

	for (uint8_t i = 0; i < SIZE_ZONE_TRIT_DRUM; i++)
	{
		// Читать короткое слово
		trs_t mr = ld_drum(zr, i);
		mr.l = 9;
		uint32_t r = ld_drum(zr, i).t1 & (uint32_t)(0x3FFFF);
		printf("drum[% 3i:% 3i ] ", zind, i);
		/* Вывод короткого троичного слова */
		printf(" [");
		for (j = 1; j < 10; j++)
		{
			printf("%c", numb2symtrs(get_trit_setun(mr, j)));
		};

		printf("], ");

		trs2str(mr);
		printf("\r\n");

	} /* for() */
}

/**
 * Печать памяти DRUM машины Сетунь-1958
 */
void dump_drum(void)
{
	int8_t zone;
	int8_t row;
	int8_t j;
	trs_t tv;
	trs_t r;

	printf("\r\n[ BRUM Setun-1958 ]\r\n");

	for (zone = ZONE_DRUM_BEG; zone < ZONE_DRUM_END; zone++)
	{
		for (row = 0; row < SIZE_ZONE_TRIT_DRUM; row++)
		{
			copy_trs_setun(&mem_drum[zone][row], &r);

			printf("drum[%3i:%3i] = [", zone, row - SIZE_ZONE_TRIT_DRUM / 2);

			j = 0;
			do
			{
				j++;
			} while (j < 9);

			printf("], ");

			trs2str(r);

			printf("\r\n");
		}
	}
}

/** ********************************************
 *  Реалиазция ввода и вывода  для "Сетунь-1958"
 *  --------------------------------------------
 */
#define TAB_SIZE (32)
#define TAB_MIN_SIZE (9)
#define SYMBOL_SIZE (1)
#define STRING_PAPER_SIZE (6)
uint8_t *tab4_0[TAB_SIZE][STRING_PAPER_SIZE];
trs_t tab4_1[TAB_SIZE];
trs_t tab4_2[TAB_SIZE];
uint8_t *tab4_3[TAB_SIZE][SYMBOL_SIZE];
uint8_t *tab4_4[TAB_SIZE][SYMBOL_SIZE];
uint8_t *tab4_5[TAB_SIZE][SYMBOL_SIZE];
uint8_t *tab4_6[TAB_SIZE][SYMBOL_SIZE];
uint8_t *tab4_7[TAB_MIN_SIZE][STRING_PAPER_SIZE];
trs_t tab4_8[TAB_MIN_SIZE];

/*
 * Инициализация таблицы кодирования
 * информации ввод и вывода
 */
void init_tab4(void)
{

	/* Init tab4_0 */
	uint8_t offset = 13;
	memmove(tab4_0[trs2digit(smtr("---")) + offset], "_OO.O_", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("--0")) + offset], "_O_.OO", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("--+")) + offset], "OO_.OO", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("-0-")) + offset], "_OO.__", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("-00")) + offset], "___.OO", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("-0+")) + offset], "O__.OO", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("-+-")) + offset], "_OO._O", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("-+0")) + offset], "__O.OO", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("-++")) + offset], "O_O.OO", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("0--")) + offset], "_O_.O_", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("0-0")) + offset], "_O_.__", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("0-+")) + offset], "_O_._O", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("00-")) + offset], "___.O_", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("000")) + offset], "_OO.OO", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("00+")) + offset], "___._O", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("0+-")) + offset], "__O.O_", STRING_PAPER_SIZE);
	//
	memmove(tab4_0[trs2digit(smtr("0+0")) + offset], "__O.__", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("0++")) + offset], "__O._O", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("+--")) + offset], "OO_.O_", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("+-0")) + offset], "OO_.__", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("+-+")) + offset], "OO_._O", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("+0-")) + offset], "O__.O_", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("+00")) + offset], "O__.__", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("+0+")) + offset], "O__._O", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("++-")) + offset], "O_O.O_", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("++0")) + offset], "O_O._O", STRING_PAPER_SIZE);
	memmove(tab4_0[trs2digit(smtr("+++")) + offset], "O_O.__", STRING_PAPER_SIZE);

	/* Init tab4_1 */
	tab4_1[pl_to_ind("_OO.O_")] = smtr("---");
	tab4_1[pl_to_ind("_O_.OO")] = smtr("--0");
	tab4_1[pl_to_ind("OO_.OO")] = smtr("--+");
	tab4_1[pl_to_ind("_OO.__")] = smtr("-0-");
	tab4_1[pl_to_ind("___.OO")] = smtr("-00");
	tab4_1[pl_to_ind("O__.OO")] = smtr("-0+");
	tab4_1[pl_to_ind("_OO._O")] = smtr("-+-");
	tab4_1[pl_to_ind("__O.OO")] = smtr("-+0");
	tab4_1[pl_to_ind("O_O.OO")] = smtr("-++");
	tab4_1[pl_to_ind("_O_.O_")] = smtr("0--");
	tab4_1[pl_to_ind("_O_.__")] = smtr("0-0");
	tab4_1[pl_to_ind("_O_._O")] = smtr("0-+");
	tab4_1[pl_to_ind("___.O_")] = smtr("00-");
	tab4_1[pl_to_ind("_OO.OO")] = smtr("000");
	tab4_1[pl_to_ind("___._O")] = smtr("00+");
	tab4_1[pl_to_ind("__O.O_")] = smtr("0+-");
	//
	tab4_1[pl_to_ind("__O.__")] = smtr("0+0");
	tab4_1[pl_to_ind("__O._O")] = smtr("0++");
	tab4_1[pl_to_ind("OO_.O_")] = smtr("+--");
	tab4_1[pl_to_ind("OO_.__")] = smtr("+-0");
	tab4_1[pl_to_ind("OO_._O")] = smtr("+-+");
	tab4_1[pl_to_ind("O__.O_")] = smtr("+0-");
	tab4_1[pl_to_ind("O__.__")] = smtr("+00");
	tab4_1[pl_to_ind("O__._O")] = smtr("+0+");
	tab4_1[pl_to_ind("O_O.O_")] = smtr("++-");
	tab4_1[pl_to_ind("O_O._O")] = smtr("++0");
	tab4_1[pl_to_ind("O_O.__")] = smtr("+++");
	tab4_1[pl_to_ind("___.__")] = smtr("000");
	tab4_1[pl_to_ind("OOO.__")] = smtr("000");
	tab4_1[pl_to_ind("OOO._O")] = smtr("000");
	tab4_1[pl_to_ind("OOO.O_")] = smtr("000");
	tab4_1[pl_to_ind("OOO.OO")] = smtr("000");

	/* Init tab4_2 */
	tab4_2[pl_to_ind("_OO.O_")] = smtr("+++");
	tab4_2[pl_to_ind("_O_.OO")] = smtr("+++");
	tab4_2[pl_to_ind("OO_.OO")] = smtr("+++");
	tab4_2[pl_to_ind("_OO.__")] = smtr("+++");
	tab4_2[pl_to_ind("___.OO")] = smtr("+++");
	tab4_2[pl_to_ind("O__.OO")] = smtr("+++");
	tab4_2[pl_to_ind("_OO._O")] = smtr("+++");
	tab4_2[pl_to_ind("__O.OO")] = smtr("+++");
	tab4_2[pl_to_ind("O_O.OO")] = smtr("+++");
	tab4_2[pl_to_ind("_O_.O_")] = smtr("--");
	tab4_2[pl_to_ind("_O_.__")] = smtr("-0");
	tab4_2[pl_to_ind("_O_._O")] = smtr("-+");
	tab4_2[pl_to_ind("___.O_")] = smtr("0-");
	tab4_2[pl_to_ind("_OO.OO")] = smtr("00");
	tab4_2[pl_to_ind("___._O")] = smtr("0+");
	tab4_2[pl_to_ind("__O.O_")] = smtr("+-");
	//
	tab4_2[pl_to_ind("__O.__")] = smtr("+0");
	tab4_2[pl_to_ind("__O._O")] = smtr("++");
	tab4_2[pl_to_ind("OO_.O_")] = smtr("+++");
	tab4_2[pl_to_ind("OO_.__")] = smtr("+++");
	tab4_2[pl_to_ind("OO_._O")] = smtr("+++");
	tab4_2[pl_to_ind("O__.O_")] = smtr("+++");
	tab4_2[pl_to_ind("O__.__")] = smtr("+++");
	tab4_2[pl_to_ind("O__._O")] = smtr("+++");
	tab4_2[pl_to_ind("O_O.O_")] = smtr("+++");
	tab4_2[pl_to_ind("O_O._O")] = smtr("+++");
	tab4_2[pl_to_ind("O_O.__")] = smtr("+++");
	tab4_2[pl_to_ind("___.__")] = smtr("+++");
	tab4_2[pl_to_ind("OOO.__")] = smtr("+++");
	tab4_2[pl_to_ind("OOO._O")] = smtr("+++");
	tab4_2[pl_to_ind("OOO.O_")] = smtr("+++");
	tab4_2[pl_to_ind("OOO.OO")] = smtr("+++");

	/* Init tab4_3 */
	memmove(tab4_3[pl_to_ind("_OO.O_")], "\0", SYMBOL_SIZE); // стоп
	memmove(tab4_3[pl_to_ind("_O_.OO")], "-", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OO_.OO")], "_", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("_OO.__")], "\n", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("___.OO")], "/", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("O__.OO")], ".", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("_OO._O")], "x", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("__O.OO")], "+", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("O_O.OO")], "V", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("_O_.O_")], "W", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("_O_.__")], "X", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("_O_._O")], "Y", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("___.O_")], "Z", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("_OO.OO")], "O", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("___._O")], "1", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("__O.O_")], "2", SYMBOL_SIZE);
	//
	memmove(tab4_3[pl_to_ind("__O.__")], "3", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("__O._O")], "4", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OO_.O_")], "5", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OO_.__")], "6", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OO_._O")], "7", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("O__.O_")], "8", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("O__.__")], "9", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("O__._O")], " ", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("O_O.O_")], " ", SYMBOL_SIZE); // цифровой регистр
	memmove(tab4_3[pl_to_ind("O_O._O")], ")", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("O_O.__")], " ", SYMBOL_SIZE); // буквенный регистр
	memmove(tab4_3[pl_to_ind("___.__")], " ", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OOO.__")], " ", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OOO._O")], " ", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OOO.O_")], " ", SYMBOL_SIZE);
	memmove(tab4_3[pl_to_ind("OOO.OO")], " ", SYMBOL_SIZE); // вычеркивание

	/* Init tab4_4 */
	memmove(tab4_4[pl_to_ind("_OO.O_")], "\0", SYMBOL_SIZE); // стоп
	memmove(tab4_4[pl_to_ind("_O_.OO")], "-", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OO_.OO")], "_", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("_OO.__")], "\n", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("___.OO")], "Ю", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("O__.OO")], ".", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("_OO._O")], "x", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("__O.OO")], "+", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("O_O.OO")], "Э", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("_O_.O_")], "Ж", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("_O_.__")], "Х", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("_O_._O")], "У", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("___.O_")], "Ц", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("_OO.OO")], "O", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("___._O")], "1", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("__O.O_")], "2", SYMBOL_SIZE);
	//
	memmove(tab4_4[pl_to_ind("__O.__")], "3", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("__O._O")], "4", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OO_.O_")], "5", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OO_.__")], "6", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OO_._O")], "7", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("O__.O_")], "8", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("O__.__")], "9", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("O__._O")], " ", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("O_O.O_")], " ", SYMBOL_SIZE); // цифровой регистр
	memmove(tab4_4[pl_to_ind("O_O._O")], "Ф", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("O_O.__")], " ", SYMBOL_SIZE); // буквенный регистр
	memmove(tab4_4[pl_to_ind("___.__")], " ", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OOO.__")], " ", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OOO._O")], " ", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OOO.O_")], " ", SYMBOL_SIZE);
	memmove(tab4_4[pl_to_ind("OOO.OO")], " ", SYMBOL_SIZE); // вычеркивание

	/* Init tab4_5 */
	memmove(tab4_5[pl_to_ind("_OO.O_")], " ", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("_O_.OO")], "F", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OO_.OO")], "_", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("_OO.__")], "\n", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("___.OO")], "G", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("O__.OO")], "H", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("_OO._O")], "=", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("__O.OO")], "I", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("O_O.OO")], "J", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("_O_.O_")], "K", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("_O_.__")], "L", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("_O_._O")], "M", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("___.O_")], "N", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("_OO.OO")], "P", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("___._O")], "Q", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("__O.O_")], "R", SYMBOL_SIZE);
	//
	memmove(tab4_5[pl_to_ind("__O.__")], "S", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("__O._O")], "T", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OO_.O_")], "U", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OO_.__")], "A", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OO_._O")], "B", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("O__.O_")], "C", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("O__.__")], "D", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("O__._O")], "E", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("O_O.O_")], " ", SYMBOL_SIZE); // цифровой регистр
	memmove(tab4_5[pl_to_ind("O_O._O")], "(", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("O_O.__")], " ", SYMBOL_SIZE); // буквенный регистр
	memmove(tab4_5[pl_to_ind("___.__")], " ", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OOO.__")], " ", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OOO._O")], " ", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OOO.O_")], " ", SYMBOL_SIZE);
	memmove(tab4_5[pl_to_ind("OOO.OO")], " ", SYMBOL_SIZE); // вычеркивание

	/* Init tab4_6 */
	memmove(tab4_6[pl_to_ind("_OO.O_")], " ", SYMBOL_SIZE); // стоп
	memmove(tab4_6[pl_to_ind("_O_.OO")], "Б", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OO_.OO")], "_", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("_OO.__")], "\n", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("___.OO")], "Щ", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("O__.OO")], "Н", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("_OO._O")], "=", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("__O.OO")], "П", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("O_O.OO")], "Ы", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("_O_.O_")], "К", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("_O_.__")], "Г", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("_O_._O")], "М", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("___.O_")], "И", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("_OO.OO")], "Р", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("___._O")], "Й", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("__O.O_")], "Я", SYMBOL_SIZE);
	//
	memmove(tab4_6[pl_to_ind("__O.__")], "Ь", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("__O._O")], "Т", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OO_.O_")], "П", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OO_.__")], "А", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OO_._O")], "В", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("O__.O_")], "С", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("O__.__")], "Д", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("O__._O")], "Е", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("O_O.O_")], " ", SYMBOL_SIZE); // цифровой регистр
	memmove(tab4_6[pl_to_ind("O_O._O")], "Ш", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("O_O.__")], " ", SYMBOL_SIZE); // буквенный регистр
	memmove(tab4_6[pl_to_ind("___.__")], " ", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OOO.__")], " ", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OOO._O")], " ", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OOO.O_")], " ", SYMBOL_SIZE);
	memmove(tab4_6[pl_to_ind("OOO.OO")], " ", SYMBOL_SIZE); // вычеркивание

	/* Init tab4_7 */
	uint8_t offset4_7 = 4;
	memmove(tab4_7[trs2digit(smtr("--")) + offset4_7], "_O_.O_", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("-0")) + offset4_7], "_O_.__", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("-+")) + offset4_7], "_O_._O", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("0-")) + offset4_7], "___.O_", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("00")) + offset4_7], "_OO.OO", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("0+")) + offset4_7], "___._O", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("+-")) + offset4_7], "__O.O_", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("+0")) + offset4_7], "__O.__", STRING_PAPER_SIZE);
	memmove(tab4_7[trs2digit(smtr("++")) + offset4_7], "__O._O", STRING_PAPER_SIZE);

	/* Init tab4_8 */
	tab4_8[pl_to_ind("_O_.O_")] = smtr("--");
	tab4_8[pl_to_ind("_O_.__")] = smtr("-0");
	tab4_8[pl_to_ind("_O_._O")] = smtr("-+");
	tab4_8[pl_to_ind("___.O_")] = smtr("0-");
	tab4_8[pl_to_ind("_OO.OO")] = smtr("00");
	tab4_8[pl_to_ind("___._O")] = smtr("0+");
	tab4_8[pl_to_ind("__O.O_")] = smtr("+-");
	tab4_8[pl_to_ind("__O.__")] = smtr("+0");
	tab4_8[pl_to_ind("__O._O")] = smtr("++");
}

/* Декодирование строки в индекс таблицы */
uint8_t pl_to_ind(uint8_t *perline)
{

	uint8_t byte;

	byte = 0;
	for (int i = 0; i < strlen(perline); i++)
	{

		if (i == 3)
		{
			if (perline[i] != '.')
				return 0; /* error string paper */
		}
		else
		{
			if ((perline[i] == 'O') || (perline[i] == 'o'))
			{

				switch (i)
				{
				case 0:
					byte |= (1 << 4);
					break;
				case 1:
					byte |= (1 << 3);
					break;
				case 2:
					byte |= (1 << 2);
					break;
				case 4:
					byte |= (1 << 1);
					break;
				case 5:
					byte |= (1 << 0);
					break;
				}
			}
		}
	}
	return byte;
}

/* Декодирование комбинации пробивок в троичное число */
int16_t Decoder_String_from_Paper_Line(void)
{

	uint8_t byte;
	int len;
	int16_t count = 0;
	uint8_t cmd[20];

	FILE *filepl;
	filepl = fopen("software/paper.txt", "r");
	if (filepl == NULL)
	{
		printf("ERR fopen %s\r\n", "software/paper.txt");
		return 0;
	}
	printf("open %s\r\n", "software/paper.txt");

	while (fscanf(filepl, "%s", cmd) != EOF)
	{
		printf("%s : ", cmd);

		byte = 0;
		for (int i = 0; i < strlen(cmd); i++)
		{

			if (i == 3)
			{
				if (cmd[i] != '.')
					return 0; /* error string paper */
			}
			else
			{
				if ((cmd[i] == 'O') || (cmd[i] == 'o'))
				{

					switch (i)
					{
					case 0:
						byte |= (1 << 4);
						break;
					case 1:
						byte |= (1 << 3);
						break;
					case 2:
						byte |= (1 << 2);
						break;
					case 4:
						byte |= (1 << 1);
						break;
					case 5:
						byte |= (1 << 0);
						break;
					}
				}
			}
		}
		printf("byte=%i\r\n", byte);
		count += 1;
	}
	fclose(filepl);

	printf("\r\nr=%i\n", count);

	return 1;
}

/* Декодирование комбинации пробивок в троичное число */
trs_t Decoder_Command_Paper_Line(char *paperline, uint8_t *err)
{

	uint8_t byte;
	int len;
	int16_t count = 0;
	trs_t res;

	byte = 0;
	for (int i = 0; i < strlen(paperline); i++)
	{

		if (i == 3)
		{
			if (paperline[i] != '.')
			{
				*err = 1;	/* Error#1 */
				return res; /* error string paper */
			}
		}
		else
		{
			if ((paperline[i] == 'O') || (paperline[i] == 'o'))
			{

				switch (i)
				{
				case 0:
					byte |= (1 << 4);
					break;
				case 1:
					byte |= (1 << 3);
					break;
				case 2:
					byte |= (1 << 2);
					break;
				case 4:
					byte |= (1 << 1);
					break;
				case 5:
					byte |= (1 << 0);
					break;
				}
			}
		}
	}
	// printf(" byte=%i\r\n", byte);	//viv+ dbg

	/* Проверить допустимые комбинации пробивок */
	int r;
	res = tab4_2[byte];
	r = trs2digit(res);

	if (r > 4)
	{
		/* Error */
		*err = 1; /* Error */
	}
	else
	{
		/* Valid */
		*err = 0; /* Ok' */
	}

	return res;
}

/* Декодирование комбинации пробивок в троичное число */
trs_t Decoder_Symbol_Paper_Line(char *paperline, uint8_t *err)
{

	uint8_t byte;
	int len;
	int16_t count = 0;
	trs_t res;

	byte = 0;
	for (int i = 0; i < strlen(paperline); i++)
	{

		if (i == 3)
		{
			if (paperline[i] != '.')
			{
				*err = 1;	/* Error#1 */
				return res; /* error string paper */
			}
		}
		else
		{
			if ((paperline[i] == 'O') || (paperline[i] == 'o'))
			{

				switch (i)
				{
				case 0:
					byte |= (1 << 4);
					break;
				case 1:
					byte |= (1 << 3);
					break;
				case 2:
					byte |= (1 << 2);
					break;
				case 4:
					byte |= (1 << 1);
					break;
				case 5:
					byte |= (1 << 0);
					break;
				}
			}
		}
	}

	/* Проверить допустимые комбинации пробивок */
	int r;
	res = tab4_1[byte];
	
	/* Valid */
	*err = 0; /* Ok' */

	return res;
}

/**
 * Печать на электрифицированную пишущую машинку
 * 'An electrified typewriter'
 */
void electrified_typewriter(trs_t t, uint8_t local)
{

	int32_t code;

	color_sw += 0;

	russian_latin_sw = local;
	code = trs2digit(t);

	switch (code)
	{
	case 6: /* t = 1-10 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "А");
				fwrite("А", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "6");
				fwrite("6", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "A");
				fwrite("А", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "6");
				fwrite("6", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 7: /* t = 1-11 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "В");
				fwrite("В", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "7");
				fwrite("7", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "B");
				fwrite("В", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "7");
				fwrite("7", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 8: /* t = 10-1 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "С");
				fwrite("С", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "8");
				fwrite("8", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "C");
				fwrite("С", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "8");
				fwrite("8", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 9: /* t = 100 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Д");
				fwrite("Д", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "9");
				fwrite("9", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "D");
				fwrite("D", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "9");
				fwrite("9", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 10: /* t = 101 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Е");
				fwrite("Е", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", " ");
				fwrite(" ", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "E");
				fwrite("E", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", " ");
				fwrite(" ", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -12: /* t = -1-10 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Б");
				fwrite("Б", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "-");
				fwrite("-", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "F");
				fwrite("F", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "-");
				fwrite("-", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -9: /* t = -100 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Щ");
				fwrite("Щ", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Ю");
				fwrite("Ю", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "G");
				fwrite("G", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "/");
				fwrite("/", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -8: /* t = -101 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Н");
				fwrite("Н", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", ",");
				fwrite(",", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "H");
				fwrite("H", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", ".");
				fwrite(".", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -6: /* t = -110  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "I");
				fwrite("I", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "+");
				fwrite("+", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Л");
				fwrite("Л", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "+");
				fwrite("+", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -5: /* t = -111 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Ы");
				fwrite("Ы", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Э");
				fwrite("Э", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "J");
				fwrite("J", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "V");
				fwrite("V", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -4: /* t = 0-1-1 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "К");
				fwrite("К", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Ж");
				fwrite("Ж", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "K");
				fwrite("K", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "W");
				fwrite("W", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -3: /* t = 0-10  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Г");
				fwrite("Г", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Х");
				fwrite("Х", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "L");
				fwrite("L", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "X");
				fwrite("X", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -2: /* t = 0-11  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "М");
				fwrite("М", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "У");
				fwrite("У", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "M");
				fwrite("M", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Y");
				fwrite("Y", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -1: /* t = 00-1  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "И");
				fwrite("И", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Ц");
				fwrite("Ц", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "N");
				fwrite("N", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Z");
				fwrite("Z", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 0: /* t = 000  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Р");
				fwrite("P", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "О");
				fwrite("О", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "P");
				fwrite("P", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "O");
				fwrite("O", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 1: /* t = 001  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Й");
				fwrite("Й", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "1");
				fwrite("1", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Q");
				fwrite("Q", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "1");
				fwrite("1", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 2: /* t = 01-1  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Я");
				fwrite("Я", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "2");
				fwrite("2", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "R");
				fwrite("R", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "2");
				fwrite("2", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 3: /* t = 010  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Ь");
				fwrite("Ь", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "3");
				fwrite("3", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "S");
				fwrite("S", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "3");
				fwrite("3", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 4: /* t = 011  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Т");
				fwrite("Т", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "4");
				fwrite("4", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "T");
				fwrite("T", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "4");
				fwrite("4", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 5: /* t = 1-1-1 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "П");
				fwrite("П", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "5");
				fwrite("5", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "U");
				fwrite("U", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "5");
				fwrite("5", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 13: /* t = 111 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "Ш");
				fwrite("Ш", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "Ф");
				fwrite("Ф", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "(");
				fwrite("(", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", ")");
				fwrite(")", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -7: /* t = -11-1 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "=");
				fwrite("=", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "х");
				fwrite("х", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "=");
				fwrite("=", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "x");
				fwrite("x", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case -11: /* t = -1-11 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
					// переключить цвет черный
				break;
			default: /* number */
					 // переключить цвет красный
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "?");
				fwrite("?", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "?");
				fwrite("?", 1, 1, tty1);
				break;
			}
			break;
		}
		break;

	case 12: /* t = 110  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				letter_number_sw = 0;
				break;
			default: /* number */
				letter_number_sw = 0;
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				letter_number_sw = 0;
				break;
			default: /* number */
				letter_number_sw = 0;
				break;
			}
			break;
		}
		break;

	case 11: /* t = 11-1  */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				letter_number_sw = 1;
				break;
			default: /* number */
				letter_number_sw = 1;
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				letter_number_sw = 1;
				break;
			default: /* number */
				letter_number_sw = 1;
				break;
			}
			break;
		}
		break;

	case -10: /* t = -10-1 */
		switch (russian_latin_sw)
		{
		case 0: /* russian */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "\r\n");
				fwrite("\n", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "\r\n");
				fwrite("\n", 1, 1, tty1);
				break;
			}
			break;
		default: /* latin */
			switch (letter_number_sw)
			{
			case 0: /* letter */
				printf("%s", "\r\n");
				fwrite("\n", 1, 1, tty1);
				break;
			default: /* number */
				printf("%s", "\r\n");
				fwrite("\r\n", 1, 1, tty1);
				break;
			}
			break;
		}
		break;
	}
}

uint8_t Begin_Read_Commands_from_FT1(FILE *file)
{
	trs_t fa;
	/* zone 0 FRAM */
	fa = smtr("0---0");
	return (Read_Commands_from_FT1(file, fa));
}

/**
 * Чтение команд с фотосчитывателя №1
 */
uint8_t Read_Commands_from_FT1(FILE *file, trs_t fa)
{
	if (DEBUG > 0)
	{
		printf("[ Read commands from FT1 ]\r\n");
	}

	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	trs_t dst;
	trs_t addr;
	trs_t oper;
	uint8_t cmd[20];

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;
	//
	dst.l = 9;
	MR.l = 18;
	int i = 0;

	trs_t tcmd;
	tcmd.l = 9;
	tcmd.t0 = 0;
	tcmd.t1 = 0;
	//
	uint8_t cnt_cmd = 54;
	uint8_t cnt_line = 5;
	//
	while ((fscanf(file, "%s", cmd) != EOF) && (cnt_cmd != 0))
	{
		uint8_t err = 0;
		trs_t res;

		res = Decoder_Command_Paper_Line(cmd, &err);
		if (err == 0)
		{

			if (cnt_line > 0)
			{
				int8_t p1 = get_trit_setun(res, 1);
				int8_t p2 = get_trit_setun(res, 2);
				tcmd = set_trit_setun(tcmd, 8, p1);
				tcmd = set_trit_setun(tcmd, 9, p2);
				cnt_line -= 1;
			}
			if (cnt_line == 0)
			{
				cnt_line = 5;
				cnt_cmd -= 1;
				// view_short_reg(&tcmd, "tcmd");
				//
				sum = add_trs(sum, tcmd);

				dsun += trs2digit(tcmd);

				// debug_print("%s -> [", tcmd);
				debug_print(" -> [");

				if (DEBUG > 0)
				{
					trs2str(tcmd);
				}
				debug_print("]");
				if (DEBUG > 0)
				{
					view_short_reg(&fa, " addr");
				}
				else
				{
					if (DEBUG > 0)
					{
						printf("\r\n");
					}
				}

				st_fram(fa, tcmd);
				fa = next_address(fa);
				mod_3_n(&fa, 5);
				tcmd.t0 = 0;
				tcmd.t1 = 0;
				//
				i += 1;
			}
			else
			{
				tcmd = shift_trs(tcmd, 2);
			}
		}
	}

	if (DEBUG > 0)
	{
		printf("\r\ni=%i\r\n", i);
		/* Печать контрольных сумм */
		view_checksum_setun(sum);
	}

	if (cnt != SIZE_ZONE_TRIT_FRAM)
		return 1; /* Error #2 */

	return 0; /* OK' */
}

/**
 * Чтение символов с фотосчитывателя №1
 */
uint8_t Read_Symbols_from_FT1(FILE *file, trs_t fa)
{
	if (DEBUG > 0)
	{
		printf("[ Read symbols from FT1 ]\r\n");
	}

	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	trs_t dst;
	trs_t addr;
	trs_t oper;
	uint8_t cmd[20];

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;
	//
	dst.l = 9;
	MR.l = 18;
	int i = 0;

	trs_t tcmd;
	tcmd.l = 9;
	tcmd.t0 = 0;
	tcmd.t1 = 0;
	//
	uint8_t cnt_cmd = 54;
	uint8_t cnt_line = 3;
	//
	while ((fscanf(file, "%s", cmd) != EOF) && (cnt_cmd != 0))
	{
		uint8_t err = 0;
		trs_t res;

		res = Decoder_Symbol_Paper_Line(cmd, &err);
		if (err == 0)
		{
			/* Установить триты */
			if (cnt_line > 0)
			{
				int8_t p1 = get_trit_setun(res, 1);
				int8_t p2 = get_trit_setun(res, 2);
				int8_t p3 = get_trit_setun(res, 3);
				tcmd = set_trit_setun(tcmd, 7, p1);
				tcmd = set_trit_setun(tcmd, 8, p2);
				tcmd = set_trit_setun(tcmd, 9, p3);

				cnt_line -= 1;
			}

			if (cnt_line == 0)
			{
				cnt_line = 3;
				cnt_cmd -= 1;

				sum = add_trs(sum, tcmd);
				dsun += trs2digit(tcmd);

				debug_print(" -> [");

				if (DEBUG > 0)
				{
					trs2str(tcmd);
				}
				debug_print("]");
				if (DEBUG > 0)
				{
					view_short_reg(&fa, " addr");
				}
				else
				{
					if (DEBUG > 0)
					{
						printf("\r\n");
					}
				}

				st_fram(fa, tcmd);
				fa = next_address(fa);
				mod_3_n(&fa, 5);
				tcmd.t0 = 0;
				tcmd.t1 = 0;

				int32_t dtres = trs2digit(res);
				if (dtres == -13)
				{
					/* Символ 'ст' '_OO.O_' */
					break; /* exit while(...) */
				}
				//
				i += 1;
			}
			else
			{
				tcmd = shift_trs(tcmd, 3);
			}
		}
	}

	if (DEBUG > 0)
	{
		printf("\r\ni=%i\r\n", i);
		/* Печать контрольных сумм */
		view_checksum_setun(sum);
	}

	if (cnt != SIZE_ZONE_TRIT_FRAM)
		return 1; /* Error #2 */

	return 0; /* OK' */
}

/**
 * Чтение команд с фотосчитывателя №2
 */
uint8_t Read_Commands_from_FT2(FILE *file, trs_t fa)
{
	printf("[ Read commands from FT2 ]\r\n");
	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	trs_t dst;
	trs_t addr;
	trs_t oper;
	uint8_t cmd[20];

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;
	//
	dst.l = 9;
	MR.l = 18;
	int i = 0;

	while (fscanf(file, "%s", cmd) != EOF)
	{
		if (strlen(cmd) != 5)
			continue;

		cmd_str_2_trs(cmd, &dst);
		sum = add_trs(sum, dst);
		i += 1;

		dsun += trs2digit(dst);

		debug_print("%s -> [", cmd);

		if (DEBUG > 0)
		{
			trs2str(dst);
		}
		debug_print("]");
		if (DEBUG > 0)
		{
			view_short_reg(&fa, " addr");
		}
		else
		{
			if (DEBUG > 0)
			{
				printf("\r\n");
			}
		}

		st_fram(fa, dst);
		fa = next_address(fa);
		mod_3_n(&fa, 5);

		if (i == SIZE_ZONE_TRIT_FRAM)
			break;
	}

	printf("\r\ni=%i\r\n", i);
	/* Печать контрольных сумм */
	view_checksum_setun(sum);

	if (cnt != SIZE_ZONE_TRIT_FRAM)
		return 1; /* Error #2 */

	return 0; /* OK' */
}

/**
 * Чтение символов с фотосчитывателя №1
 */
uint8_t Read_Symbols_from_FT2(FILE *file, trs_t fa)
{
	if (DEBUG > 0)
	{
		printf("[ Read symbols from FT2 ]\r\n");
	}

	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	trs_t dst;
	trs_t addr;
	trs_t oper;
	uint8_t cmd[20];

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;
	//
	dst.l = 9;
	MR.l = 18;
	int i = 0;

	trs_t tcmd;
	tcmd.l = 9;
	tcmd.t0 = 0;
	tcmd.t1 = 0;
	//
	uint8_t cnt_cmd = 54;
	uint8_t cnt_line = 3;
	//
	while ((fscanf(file, "%s", cmd) != EOF) && (cnt_cmd != 0))
	{
		uint8_t err = 0;
		trs_t res;

		res = Decoder_Symbol_Paper_Line(cmd, &err);
		if (err == 0)
		{
			if (cnt_line > 0)
			{
				int8_t p1 = get_trit_setun(res, 1);
				int8_t p2 = get_trit_setun(res, 2);
				int8_t p3 = get_trit_setun(res, 3);
				tcmd = set_trit_setun(tcmd, 7, p1);
				tcmd = set_trit_setun(tcmd, 8, p1);
				tcmd = set_trit_setun(tcmd, 9, p2);

				cnt_line -= 1;
			}
			if (cnt_line == 0)
			{
				cnt_line = 3;
				cnt_cmd -= 1;

				sum = add_trs(sum, tcmd);
				int32_t dtcmd = trs2digit(tcmd);
				dsun += trs2digit(tcmd);
				if (dtcmd == 14)
				{
					/* Символ 'ст' '_OO.0_' */
					break; /* exit while(...) */
				}

				debug_print(" -> [");

				if (DEBUG > 0)
				{
					trs2str(tcmd);
				}
				debug_print("]");
				if (DEBUG > 0)
				{
					view_short_reg(&fa, " addr");
				}
				else
				{
					if (DEBUG > 0)
					{
						printf("\r\n");
					}
				}

				st_fram(fa, tcmd);
				fa = next_address(fa);
				mod_3_n(&fa, 5);
				tcmd.t0 = 0;
				tcmd.t1 = 0;
				//
				i += 1;
			}
			else
			{
				tcmd = shift_trs(tcmd, 3);
			}
		}
	}

	if (DEBUG > 0)
	{
		printf("\r\ni=%i\r\n", i);
		/* Печать контрольных сумм */
		view_checksum_setun(sum);
	}

	if (cnt != SIZE_ZONE_TRIT_FRAM)
		return 1; /* Error #2 */

	return 0; /* OK' */
}

/**
 * Вывод на TTY1
 */
uint8_t Write_Commands_to_TTY1(FILE *file, trs_t fa)
{
	uint8_t offset = 13;

	if (file == NULL)
		return 1; /* Error */

	electrified_typewriter(fa, russian_latin_sw);

	return 0; /* OK' */
}

/**
 * Вывод на TTY1
 */
uint8_t Write_Symbols_to_TTY1(FILE *file, trs_t fa)
{

	uint8_t offset = 13;

	if (file == NULL)
		return 1; /* Error */

	electrified_typewriter(fa, russian_latin_sw);

	return 0; /* OK' */
}

/**
 * Перфорация на PTP1
 */
uint8_t Perforation_Commands_to_PTP1(FILE *file, trs_t fa)
{
	uint8_t offset = 13;
	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	fprintf(file, "%s\r\n", (char *)tab4_0[trs2digit(fa) + offset]);

	return 0; /* OK' */
}

/**
 * Перфорация на PTP1
 */
uint8_t Perforation_Symbols_to_PTP1(FILE *file, trs_t fa)
{
	uint8_t offset = 13;
	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	fprintf(file, "%s\r\n", (char *)tab4_0[trs2digit(fa) + offset]);

	return 0; /* OK' */
}

/** *******************************************
 *  Реалиазция виртуальной машины "Сетунь-1958"
 *  -------------------------------------------
 */

/**
 * Аппаратный сброс.
 * Очистить память и регистры
 * виртуальной машины "Сетунь-1958"
 */
void reset_setun_1958(void)
{
	clean_fram(); /* Очистить  FRAM */
	clean_drum(); /* Очистить  DRUM */
}

/**
 * Вернуть модифицированное K(1:9) для выполнения операции "Сетунь-1958"
 */
trs_t control_trs(trs_t a)
{
	int8_t k9;
	trs_t k1_5;
	trs_t cn;

	clear(&cn);

	/* Адресная часть K(9) */
	k1_5 = slice_trs_setun(a, 1, 5);

	/* Признак модификации адремной части K(9) */
	k9 = get_trit_setun(a, 9);

	/* Модицикация адресной части K(1:5) */
	if (k9 > 0)
	{ /* A(1:5) = A(1:5) + F(1:5) */
		k1_5 = add_trs(k1_5, F);
		mod_3_n(&k1_5, 5);
	}
	else if (k9 < 0)
	{ /* A(1:5) = A(1:5) - F(1:5) */
		k1_5 = sub_trs(k1_5, F);
		mod_3_n(&k1_5, 5);
	}
	else
	{	/*  A(1:5) = A(1:5) */
		/* Без изменений    */
	}

	cn = shift_trs(k1_5, 4);
	cn.l = 9;

	a = slice_trs_setun(a, 6, 9);

	cn = add_trs(a, cn);
	cn.l = 9;

	return cn;
}

/******************************************************************************************
					Таблица операций машина "Сетунь-1958"
-------------------------------------------------------------------------------------------
Код(троичный)	Код			Название операции			W		Содержание команды
-------------------------------------------------------------------------------------------
+00	30			9			Посылка в S					w(S)	(A*)=>(S)
+0+	33			10			Сложение в S				w(S)	(S)+(A*)=>(S)
+0-	3X			8			Вычитание в S				w(S)	(S)-(A*)=>(S)
++0	40			12			Умножение 0					w(S)	(S)=>(R); S=0; (A*)(R)=>(S)
+++	43			13			Умножение +					w(S)	(S)+(A*)(R)=>(S)
++-	4X			11			Умножение -					w(S)	(A*)+(S)(R)=>(S)
+-0	20			6			Поразрядное умножение		w(S)	(A*)[x](S)=>(S)
+-+	23			7			Посылка в R					w(R)	(A*)=>(R)
+--	2X			5			Останов	Стоп;				w(R)	STOP; (A*)=>(C)
0+0	10			3			Условный переход 0			-		A*=>(C) при w=0
0++	13			4			Условный переход +			-		A*=>(C) при w=+
0+-	1X			2			Условный переход -			-		A*=>(C) при w=-
000	00			0			Безусловный переход			-		A*=>(C)
00+	03			1			Запись из C					-		(C)=>(A*)
00-	0X			-1			Запись из F					w(F)	(F)=>(A*)
0-0	Z0			-3			Посылка в F					w(F)	(A*)=>(F)
0--	ZX			-4			Сложение в F				w(F)	(F)+(A*)=>(F)
0-+	Z3			-2			Сложение в F c (C)			w(F)	(C)+(A*)=>(F)
-+0	Y0			-6			Сдвиг (S) на S				w(S)	(A*)=>(S)
-++	Y3			-5			Запись из S					w(S)	(S)=>(A*)
-+-	YX			-7			Нормализация				w(S)	Норм.(S)=>(A*); (N)=>(S)
-00	X0			-9			Вывод-ввод Ввод в Фа*.		-		Вывод из Фа*
-0+ X3			-8			Запись на МБ				-		(Фа*)=>(Мд*)
-0- XX			-10			Считывание с МБ				-		(Мд*)=>(Фа*)
--0	W0			-12			Не задействована					Аварийный стоп
--+	W3			-11			Не задействована					Аварийный стоп
---	WX			-13			Не задействована					Аварийный стоп
-----------------------------------------------------------------------------------------*/

/**
 * Выполнить операцию K(1:9) машины "Сетунь-1958"
 */
int8_t execute_trs(trs_t addr, trs_t oper)
{
	// TODO проверить выполнение команды
	// для С(5) = -1 выполнить 2-раза старшей половине A(9:18) и сделать inc C

	trs_t k1_5;		 /* K(1:5)	*/
	trs_t k6_8;		 /* K(6:8)	*/
	int8_t codeoper; /* Код операции */

	/* Адресная часть */
	k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;

	/* Код операции */
	k6_8 = oper;
	k6_8.l = 3;

	codeoper = get_trit_setun(k6_8, 1) * 9 +
			   get_trit_setun(k6_8, 2) * 3 +
			   get_trit_setun(k6_8, 3);

	/* ---------------------------------------
	 *  Выполнить операцию машины "Сетунь-1958"
	 *  ---------------------------------------
	 */

	/*
	 * Описание реализации команд машины «Сетунь»
	 *
	 * 5-разрядный регистр управления С, в котором содержится адрес
	 * выполняемой команды, после выполнения каждой команды в регистре С
	 * формируется адрес следующей команды причём за командой являющейся первой
	 * коротким кодом какой-либо ячейки, следует­ команда, являющаяся вторым
	 * коротким кодом этой ячейки, а вслед за ней — ко­манда, являющаяся первым
	 * коротким кодом следующей ячейки, и т. д.;
	 * этот порядок может быть нарушен при выполнении команд
	 * перехода.
	 *
	 * При выполнении команд, использую­щих регистры F и С, операции производятся
	 * над 5-разрядными кодами, которые можно рассматривать как целые числа.
	 * При выборке из памяти 5-разрядный код рассматривается как старшие пять разрядов
	 * соответст­вующего короткого или длинного кода, при запоминании в ячейке
	 * па­мяти 5-разрядный код записывается в старшие пять разрядов и допол­няется
	 * до соответствующего короткого или длинного кода записью нулей в остальные разряды.
	 *
	 * При выполнении команд, использующих регистры S и R, выбираемые из памяти короткие коды
	 * рассматриваются как длин­ные с добавлением нулей в девять младших разрядов, а в оперативную
	 * память в качестве короткого кода записывается содержимое девяти старших разрядов регистра S
	 * (запись в оперативную память непосред­ственно из регистра R невозможна).
	 *
	 * После выполнения каждой команды вырабатывается значение неко­торого признака W = W(X) {-1,0,1},
	 * где X — обозначение какого-либо регистра, или сохраняется предыду­щее значение этого признака.
	 *
	 * Команды условного перехода выполняются по-разному в зависимости от значения этого признака W.
	 *
	 * При выполнении операций сложения, вычитания и умножения, использующих регистр S,
	 * может произойти останов машины по переполнению, если результат выполнения
	 * этой опе­рации, посылаемый в регистр S, окажется по модулю больше 4,5 .
	 *
	 * Операция сдвига производит сдвиг содержимого регистра S на |N|-разрядов, где N рассматривается
	 * как 5-разрядный код, хранящийся в ячейке A*, т.е. N = (А*). Сдвиг производится влево при N > 0 и
	 * вправо при N < 0. При N = 0 содержимое регистра S не изменяется.
	 *
	 * Операция нормализации производит сдвиг (S) при (S) != 0 в таком направлении и на такое число
	 * разрядов |N|, чтобы результат, посылаемый в ячейку A*, был но модулю больше 1/2 , но меньше 3/2,
	 * т.е. чтобы в двух старших разрядах результата была записана комбинация троичных цифр 01 или 0-1.
	 * При этом в регистр S посылается число N (5-разрядный код), знак которого определяется
	 * направлением сдвига, а именно: N > 0 при сдвиге вправо и N < 0 при сдвиге влево. При (S) = 0 или при
	 * 1/2 <|(S)| < 3/2 в ячейку А* посылается (S), а в регистр S посылается N = 0.
	 *
	 * Остальные операции, содержащиеся в табл. 1, ясны без дополни­тельных пояснений.
	 *
	 */
	if (DEBUG > 0)
	{
		view_step_new_addres(&k1_5, "A*");
	}

	switch (codeoper)
	{
	case (+1 * 9 + 0 * 3 + 0):
	{ // +00 : Посылка в S	(A*)=>(S)
		debug_print(" k6..8[+00]: (A*)=>(S)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR.l = 18;
			MR = shift_trs(MR, 9);
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		copy_trs_setun(&MR, &S);
		W = set_trit_setun(W, 1, sgn_trs(S));
		C = next_address(C);
	}
	break;
	case (+1 * 9 + 0 * 3 + 1):
	{ // +0+ : Сложение в S	(S)+(A*)=>(S)
		debug_print(" k6..8[+0+]: (S)+(A*)=>(S)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR.l = 18;
			MR = shift_trs(MR, 9);
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		S = add_trs(S, MR);
		mod_3_n(&S, S.l); /* очистить неиспользованные триты */
		W = set_trit_setun(W, 1, sgn_trs(S));
		if (over_check() > 0)
		{
			goto error_over;
		}
		C = next_address(C);
	}
	break;
	case (+1 * 9 + 0 * 3 - 1):
	{ // +0- : Вычитание в S	(S)-(A*)=>(S)
		debug_print(" k6..8[+0-]: (S)-(A*)=>(S)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR = shift_trs(MR, 9);
			MR.l = 18;
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		S = sub_trs(S, MR);
		W = set_trit_setun(W, 1, sgn_trs(S));
		if (over_check() > 0)
		{
			goto error_over;
		}
		C = next_address(C);
	}
	break;
	case (+1 * 9 + 1 * 3 + 0):
	{ // ++0 : Умножение +	(S)=>(R); S=0; (A*)(R)=>(S)
		debug_print(" k6..8[++0]: (S)=>(R); S=0; (A*)(R)=>(S)\n");
		copy_trs_setun(&S, &R);
		S.t1 = 0;
		S.t0 = 0;
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR = shift_trs(MR, 9);
			MR.l = 18;
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		trs_t temp = mul_trs(MR, R);
		mod_3_n(&temp, 18);
		copy_trs_setun(&temp, &S);
		W = set_trit_setun(W, 1, sgn_trs(S));
		if (over_check() > 0)
		{
			goto error_over;
		}
		C = next_address(C);
	}
	break;
	case (+1 * 9 + 1 * 3 + 1):
	{ // +++ : Умножение +	(S)+(A*)(R)=>(S)
		debug_print(" k6..8[+++]: (S)+(A*)(R)=>(S)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR = shift_trs(MR, 9);
			MR.l = 18;
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		trs_t temp = slice_trs(mul_trs(MR, R), 0, 17);
		S = add_trs(temp, S);
		W = set_trit_setun(W, 1, sgn_trs(S));
		if (over_check() > 0)
		{
			goto error_over;
		}
		C = next_address(C);
	}
	break;
	case (+1 * 9 + 1 * 3 - 1):
	{ // ++- : Умножение - (A*)+(S)(R)=>(S)
		debug_print(" k6..8[++-]: (A*)+(S)(R)=>(S)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR = shift_trs(MR, 9);
			MR.l = 18;
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		trs_t temp = mul_trs(S, R);
		S = add_trs(slice_trs(temp, 0, 17), MR);
		W = set_trit_setun(W, 1, sgn_trs(S));
		if (over_check() > 0)
		{
			goto error_over;
		}
		C = next_address(C);
	}
	break;
	case (+1 * 9 - 1 * 3 + 0):
	{ // +-0 : Поразрядное умножение	(A*)[x](S)=>(S)
		debug_print(" k6..8[+-0]: (A*)[x](S)=>(S)\n");
		MR = ld_fram(k1_5);

		if (MR.l != 18)
		{
			MR = shift_trs(MR, 9);
			MR.l = 18;
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		S = and_trs(MR, S);
		W = set_trit_setun(W, 1, sgn_trs(S));
		C = next_address(C);
	}
	break;
	case (+1 * 9 - 1 * 3 + 1):
	{ // +-+ : Посылка в R	(A*)=>(R)
		debug_print(" k6..8[+-+]: (A*)=>(R)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR = shift_trs(MR, 9);
			MR.l = 18;
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		copy_trs_setun(&MR, &R);
		W = set_trit_setun(W, 1, sgn_trs(S));
		C = next_address(C);
	}
	break;
	case (+1 * 9 - 1 * 3 - 1):
	{ // +-- : Останов	Стоп; (A*)=>(R)
		debug_print(" k6..8[+--]: STOP (A*)=>(R)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		if (MR.l != 18)
		{
			MR = shift_trs(MR, 9);
			MR.l = 18;
			mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		}
		copy_trs_setun(&MR, &R);

		return STOP;
	}
	break;
	case (+0 * 9 + 1 * 3 + 0):
	{ // 0+0 : Условный переход -	A*=>(C) при w=0
		debug_print(" k6..8[0+0]: A*=>(C) при w=0\n");
		int8_t w;
		w = sgn_trs(W);
		if (w == 0)
		{
			copy_trs_setun(&k1_5, &C);
		}
		else
		{
			C = next_address(C);
		}
	}
	break;
	case (+0 * 9 + 1 * 3 + 1):
	{ // 0++ : Условный переход -	A*=>(C) при w=0
		debug_print(" k6..8[0++]: A*=>(C) при w=+1\n");
		int8_t w;
		w = sgn_trs(W);
		if (w > 0)
		{
			copy_trs_setun(&k1_5, &C);
		}
		else
		{
			C = next_address(C);
		}
	}
	break;
	case (+0 * 9 + 1 * 3 - 1):
	{ // 0+- : Условный переход -	A*=>(C) при w=-
		debug_print(" k6..8[0+-]: A*=>(C) при w=-1\n");
		int8_t w;
		w = sgn_trs(W);
		if (w < 0)
		{
			copy_trs_setun(&k1_5, &C);
		}
		else
		{
			C = next_address(C);
		}
	}
	break;
	case (+0 * 9 + 0 * 3 + 0):
	{ //  000 : Безусловный переход	A*=>(C)
		debug_print(" k6..8[000]: A*=>(C)\n");
		copy_trs_setun(&k1_5, &C);
		C.l = 5;
	}
	break;
	case (+0 * 9 + 0 * 3 + 1):
	{ // 00+ : Запись из C	(C)=>(A*)
		debug_print(" k6..8[00+]: (C)=>(A*)\n");
		if (get_trit_setun(k1_5, 5) < 0)
		{
			MR.l = 18;
			copy_trs_setun(&C, &MR);
		}
		else
		{
			MR.l = 9;
			copy_trs_setun(&C, &MR);
		}
		st_fram(k1_5, MR);
		C = next_address(C);
	}
	break;
	case (+0 * 9 + 0 * 3 - 1):
	{ // 00- : Запись из F	(F)=>(A*)
		debug_print(" k6..8[00-]: (F)=>(A*)\n");
		if (get_trit_setun(k1_5, 5) < 0)
		{
			MR.l = 18;
			copy_trs_setun(&F, &MR);
		}
		else
		{
			MR.l = 9;
			copy_trs_setun(&F, &MR);
		}
		st_fram(k1_5, MR);
		W = set_trit_setun(W, 1, sgn_trs(F));
		C = next_address(C);
	}
	break;
	case (+0 * 9 - 1 * 3 + 0):
	{ // 0-0 : Посылка в F	(A*)=>(F)
		debug_print(" k6..8[0-0]: (A*)=>(F)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		copy_trs_setun(&MR, &F);
		F = slice_trs_setun(MR, 1, 5);
		F.l = 5;
		W = set_trit_setun(W, 1, sgn_trs(F));
		C = next_address(C);
	}
	break;
	case (+0 * 9 - 1 * 3 + 1):
	{ // 0-+ : Сложение в F c (C)	(C)+(A*)=>F
		debug_print(" k6..8[0-+]: (C)+(A*)=>F\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		MR = slice_trs_setun(MR, 1, 5);
		mod_3_n(&MR, 5);
		F = add_trs(C, MR);

		mod_3_n(&F, 5); /* очистить неиспользованные триты */
		W = set_trit_setun(W, 1, sgn_trs(F));
		C = next_address(C);
	}
	break;
	case (+0 * 9 - 1 * 3 - 1):
	{ // 0-- : Сложение в F	(F)+(A*)=>(F)
		debug_print(" k6..8[0--]: (F)+(A*)=>(F)\n");
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		MR = slice_trs_setun(MR, 1, 5);
		MR.l = 5;
		F = add_trs(F, MR);
		mod_3_n(&F, 5); /* очистить неиспользованные триты */
		W = set_trit_setun(W, 1, sgn_trs(F));
		C = next_address(C);
	}
	break;
	case (-1 * 9 + 1 * 3 + 0):
	{ // -+0 : Сдвиг	Сдвиг (S) на (A*)=>(S)
		debug_print(" k6..8[-+0]: Сдвиг (S) на (A*)=>(S)\n");
		/*
		 * Операция сдвига производит сдвиг содержимого регистра S на N
		 * разрядов, где N рассматривается как 5-разрядный код, хранящийся в
		 * ячейке А*, т. е. N = (А*). Сдвиг производится влево при N > 0 и вправо
		 * при N < 0. При N = 0 содержимое регистра S не изменяется.
		 */
		MR = ld_fram(k1_5);
		mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
		S = shift_trs(S, trs2digit(slice_trs_setun(MR, 1, 5)));
		mod_3_n(&S, 18); /* очистить неиспользованные триты */
		W = set_trit_setun(W, 1, sgn_trs(S));
		C = next_address(C);
	}
	break;
	case (-1 * 9 + 1 * 3 + 1):
	{ // -++ : Запись из S	(S)=>(A*)
		debug_print(" k6..8[-++]: (S)=>(A*)\n");
		st_fram(k1_5, S);
		W = set_trit_setun(W, 1, sgn_trs(S));
		C = next_address(C);
	}
	break;
	case (-1 * 9 + 1 * 3 - 1):
	{ // -+- : Нормализация	Норм.(S)=>(A*); (N)=>(S)
		debug_print(" k6..8[-+-]: Норм.(S)=>(A*); (N)=>(S)\n");
		/*
		 * Операция нормализации производит сдвиг (S) при (S) != 0 в таком направлении и на такое число
		 * разрядов |N|, чтобы результат, посылаемый в ячейку A*, был но модулю больше 1/2 , но меньше 3/2,
		 * т.е. чтобы в двух старших разрядах результата была записана комбинация троичных цифр 01 или 0-1.
		 * При этом в регистр S посылается число N (5-разрядный код), знак которого определяется
		 * направлением сдвига, а именно: N > 0 при сдвиге вправо и N < 0 при сдвиге влево. При (S) = 0 или при
		 * 1/2 <|(S)| < 3/2 в ячейку А* посылается (S), а в регистр S посылается N = 0.
		 */
		mod_3_n(&S, 18); /* очистить неиспользованные триты */
		if (S.t0 != 0)
		{
			/* Сдвиг S */
			if (get_trit_setun(S, 1) != 0)
			{
				S = shift_trs(S, -1); /* сдвиг вправо */
				st_fram(k1_5, S);
				S.t1 = 0;
				S.t0 = 0;
				inc_trs(&S);
				S = shift_trs(S, 13);
			}
			else if (get_trit_setun(S, 2) == 0)
			{
				uint8_t n = 0;
				for (uint8_t i = 0; i < 16; i++)
				{
					S = shift_trs(S, 1);
					n++;
					if (get_trit_setun(S, 2) != 0)
					{
						break;
					}
				}
				st_fram(k1_5, S);
				S.t1 = 0;
				S.t0 = 0;
				for (uint8_t i = 0; i < n; i++)
				{
					dec_trs(&S);
				}
				S = shift_trs(S, 13);
			}
			else
			{
				st_fram(k1_5, S);
				S.t1 = 0;
				S.t0 = 0;
			}
		}
		else
		{
			/* S == 0 */
			st_fram(k1_5, S);
			S.t1 = 0;
			S.t0 = 0;
		}
		/* Определить знак S */
		W = set_trit_setun(W, 1, sgn_trs(S));
		C = next_address(C);
	}
	break;
	case (-1 * 9 + 0 * 3 + 0):
	{ // -00 : Ввод в Фа* - Вывод из Фа*
		debug_print(" k6..8[-00]: Ввод в Фа* - Вывод из Фа*\n");

		trs_t fa;
		if (DEBUG > 0)
		{
			view_short_reg(&k1_5, "k1_5");
		}

		int8_t s = get_trit_setun(k1_5, 1);
		if (s == -1)
		{
			fa = smtr("----0");
		}
		else if (s == 1)
		{
			fa = smtr("+---0");
		}
		else
		{
			fa = smtr("0---0");
		}

		int8_t codeio = get_trit_setun(k1_5, 2) * 27 +
						get_trit_setun(k1_5, 3) * 9 +
						get_trit_setun(k1_5, 4) * 3 +
						get_trit_setun(k1_5, 5);

		/* Тип устройства ввода/вывода */
		switch (codeio)
		{
		case (+0 * 27 + 0 * 9 + 0 * 3 + 1): /* Ввод с ФТ-1 в виде команд */
			debug_print("   k2..5[000+]: Ввод с ФТ-1 в виде команд\n");
			Read_Commands_from_FT1(ptr1, fa);
			break;
		case (+0 * 27 + 0 * 9 + 1 * 3 + 0): /* Ввод с ФТ-2 в виде команд */
			debug_print("   k2..5[00+0]: Ввод с ФТ-2 в виде команд\n");
			Read_Commands_from_FT2(ptr2, fa);
			break;
		case (+0 * 27 + 0 * 9 + 0 * 3 - 1): /* Ввод с ФТ-1 в виде символов */
			debug_print("   k2..5[000-]: Ввод с ФТ-1 в виде символов\n");
			Read_Symbols_from_FT1(ptr1, fa);
			break;
		case (+0 * 27 + 0 * 9 - 1 * 3 + 0): /* Ввод с ФТ-2 в виде символов */
			debug_print("   k2..5[00-0]: Ввод с ФТ-2 в виде символов\n");
			Read_Symbols_from_FT2(ptr2, fa);
			break;
		case (+1 * 27 + 0 * 9 + 0 * 3 + 0): /* Перфорация троичных кодов на Перфоратор ПЛ (Телетайп ТП) */
			debug_print("   k2..5[+000]: Перфорация троичных кодов на Перфоратор ПЛ (Телетайп ТП)\n");

			for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
			{
				int32_t symbcode;
				trs_t symb;
				symb.l = 2;
				//
				MR = ld_fram(fa);

				symb = slice_trs_setun(MR, 1, 1);
				Perforation_Commands_to_PTP1(ptp1, symb);

				symb = slice_trs_setun(MR, 2, 3);
				Perforation_Commands_to_PTP1(ptp1, symb);

				symb = slice_trs_setun(MR, 4, 5);
				Perforation_Commands_to_PTP1(ptp1, symb);

				symb = slice_trs_setun(MR, 6, 7);
				Perforation_Commands_to_PTP1(ptp1, symb);

				symb = slice_trs_setun(MR, 8, 9);
				Perforation_Commands_to_PTP1(ptp1, symb);

				fa = next_address(fa);
			}
			break;
		case (-1 * 27 + 0 * 9 + 0 * 3 + 0): /* Перфорация в виде троичных символов на Перфоратор ПЛ (Телетайп ТП) */
			debug_print("   k2..5[-000]: Перфорация троичных символов на Перфоратор ПЛ (Телетайп ТП)\n");

			for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
			{
				int32_t symbcode;
				trs_t symb;
				symb.l = 3;
				//
				MR = ld_fram(fa);
				symb = slice_trs_setun(MR, 1, 3);
				Perforation_Symbols_to_PTP1(ptp1, symb);

				symb = slice_trs_setun(MR, 4, 6);
				Perforation_Symbols_to_PTP1(ptp1, symb);

				symb = slice_trs_setun(MR, 7, 9);
				if (trs2digit(symb) == -13)
					break;
				Perforation_Symbols_to_PTP1(ptp1, symb);

				fa = next_address(fa);
			}
			break;
		case (+0 * 27 + 1 * 9 + 0 * 3 + 0): /* Печать в виде команд на Пишущей машинке ПМ (ЭУМ-46) */
			debug_print("   k2..5[0+00]: Печать в виде команд на Пишущей машинке ПМ (ЭУМ-46)\n");

			uint8_t current = letter_number_sw;
			letter_number_sw = 1; /* цифровой регистр */

			for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
			{
				int32_t symbcode;
				trs_t symb;
				symb.l = 3;
				//
				MR = ld_fram(fa);

				symb = slice_trs_setun(MR, 1, 1);
				Write_Commands_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 2, 3);
				Write_Commands_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 4, 5);
				Write_Commands_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 6, 7);
				Write_Commands_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 8, 9);
				Write_Commands_to_TTY1(tty1, symb);

				symb = smtr("-0-");
				Write_Commands_to_TTY1(tty1, symb);

				fa = next_address(fa);
			}
			letter_number_sw = current; /* вернуть регистр */
			break;
		case (+0 * 27 - 1 * 9 + 0 * 3 + 0): /* Печать одним цветом в виде символов на пишущей машинке ПМ (ЭУМ-46) */
			debug_print("   k2..5[0-00]: Печать одним цветом в виде символов на пишущей машинке ПМ (ЭУМ-46)\n");

			for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
			{
				int32_t symbcode;
				trs_t symb;
				symb.l = 3;
				//
				MR = ld_fram(fa);

				symb = slice_trs_setun(MR, 1, 3);
				Write_Symbols_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 4, 6);
				Write_Symbols_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 7, 9);
				if (trs2digit(symb) == -13)
					break;
				Write_Symbols_to_TTY1(tty1, symb);

				fa = next_address(fa);
			}
			break;
		case (+1 * 27 - 1 * 9 + 0 * 3 + 0): /* Печать в виде символов на Пишущей машинке ПМ (ЭУМ-46) */
			debug_print("   k2..5[+-00]: Печать в виде символов на пишущей машинке ПМ (ЭУМ-46)\n");
			for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
			{
				int32_t symbcode;
				trs_t symb;
				symb.l = 3;
				//
				MR = ld_fram(fa);

				symb = slice_trs_setun(MR, 1, 3);
				Write_Symbols_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 4, 6);
				Write_Symbols_to_TTY1(tty1, symb);

				symb = slice_trs_setun(MR, 7, 9);
				if (trs2digit(symb) == -13)
					break;
				Write_Symbols_to_TTY1(tty1, symb);
				fa = next_address(fa);
			}
			break;
		default: /* Ошибка: Не поддерживается устройвтво ввода/вывода */
			return STOP_ERROR;
			break;
		}
		debug_print("\n");
		C = next_address(C);
	}
	break;
	case (-1 * 9 + 0 * 3 + 1):
	{ // -0+ : Запись на МБ	(Фа*)=>(Мд*)
		debug_print(" k6..8[-0+]: (Фа*)=>(Мд*)\n");
		int32_t zone = trs2digit(slice_trs_setun(k1_5, 2, 5));
		if ((zone < ZONE_DRUM_BEG) || (zone > ZONE_DRUM_END))
		{
			return STOP_ERROR_MB_NUMBER;
		}
		else
		{
			fram_to_drum(slice_trs_setun(k1_5, 1, 5));
			MB = slice_trs_setun(k1_5, 2, 5);
			mod_3_n(&MB, 4); /* очистить неиспользованные триты */
		}
		C = next_address(C);
	}
	break;
	case (-1 * 9 + 0 * 3 - 1):
	{ // -0- : Считывание с МБ	(Мд*)=>(Фа*)
		debug_print(" k6..8[-0-]: (Мд*)=>(Фа*)\n");
		int32_t zone = trs2digit(slice_trs_setun(k1_5, 2, 5));
		if (zone == 0)
		{
			clean_fram_zone(slice_trs_setun(k1_5, 1, 1));
		}
		else if ((zone < ZONE_DRUM_BEG) || (zone > ZONE_DRUM_END))
		{
			return STOP_ERROR_MB_NUMBER;
		}
		else
		{
			drum_to_fram(slice_trs_setun(k1_5, 1, 5));
		}
		MB = slice_trs_setun(k1_5, 2, 5);
		mod_3_n(&MB, 4); /* очистить неиспользованные триты */
		C = next_address(C);
	}
	break;
	case (-1 * 9 - 1 * 3 + 0):
	{ // --0 : Не задействована	Стоп
		debug_print(" k6..8[--0]: STOP BREAK\n");
		view_short_reg(&k6_8, "k6..8=");
		return STOP_ERROR;
	}
	break;
	case (-1 * 9 - 1 * 3 + 1):
	{ // --+ : Не задействована	Стоп
		debug_print(" k6..8[--+]: STOP BREAK\n");
		view_short_reg(&k6_8, "k6..8=");
		return STOP_ERROR;
	}
	break;
	case (-1 * 9 - 1 * 3 - 1):
	{ // --- : Не задействована	Стоп
		debug_print(" k6..8[---]: STOP BREAK\n");
		view_short_reg(&k6_8, "k6..8=");
		return STOP_ERROR;
	}
	break;
	default:
	{ // Не допустимая команда машины
		debug_print("k6..8 =[]   : STOP! NO OPERATION\n");
		view_short_reg(&k6_8, "k6..8=");
		return STOP_ERROR;
	}
	break;
	}

	if (DEBUG > 0)
	{
		view_short_regs();
	}

	return OK;

error_over:
	return STOP_ERROR;
}

/* ****************************************
 * Тестирование функций операций с тритами
 * ----------------------------------------
 */
void pl(int8_t a, int8_t c)
{
	printf(" a,c {% 2i}->% 2i\r\n", a, c);
}
void p(int8_t a, int8_t b, int8_t c)
{
	printf(" a,b,c {% 2i,% 2i}->% 2i\r\n", a, b, c);
}
void pp(int8_t a, int8_t b, int8_t c, int8_t p0, int8_t p1)
{
	printf(" a,b,p0,c,p1 {% 2i,% 2i,% 2i}->% 2i,% 2i\r\n", a, b, p0, c, p1);
}

void Test1_Ariphmetic_Ternary(void)
{

	int8_t a, b, c, p0, p1;

	printf("\r\n --- TEST #1  Logic opertion trits  for VM SETUN-1958 --- \r\n\r\n");

	printf(" 1.1. Логическая операция NOT\r\n");
	printf(" C = NOT A \r\n");

	for (a = -1; a < 2; a++)
	{
		not_t(&a, &c);
		pl(a, c);
	}

	printf(" 1.2. Логическая операция AND\r\n");
	printf(" C = A AND B \r\n");
	for (a = -1; a < 2; a++)
	{
		for (b = -1; b < 2; b++)
		{
			and_t(&a, &b, &c);
			p(a, b, c);
		}
	}

	printf(" 1.2. Логическая операция OR\r\n");
	printf(" C = A OR B \r\n");
	for (a = -1; a < 2; a++)
	{
		for (b = -1; b < 2; b++)
		{
			or_t(&a, &b, &c);
			p(a, b, c);
		}
	}

	printf(" 1.3. Логическая операция XOR\r\n");
	printf(" C = A XOR B \r\n");
	for (a = -1; a < 2; a++)
	{
		for (b = -1; b < 2; b++)
		{
			xor_t(&a, &b, &c);
			p(a, b, c);
		}
	}

	printf(" 1.4. Логическая операция SUM FULL\r\n");
	printf(" C, P1 = A + B + P0 \r\n");
	for (a = -1; a < 2; a++)
	{
		for (b = -1; b < 2; b++)
		{
			for (p0 = -1; p0 < 2; p0++)
			{
				sum_t(&a, &b, &p0, &c, &p1);
				pp(a, b, c, p0, p1);
			}
		}
	}
	printf("\n --- END TEST #1 --- \r\n");
}

/** *********************************************
 *  Тестирование функций операций с тритами
 *  ---------------------------------------------
 */
void pt(int8_t t, uint8_t p)
{
	printf("t[% 2i]=% 2i\r\n", p, t);
}
void pln(int8_t l)
{
	printf("TRIT-%02i : ", l);
}
void Test2_Opers_TRITS_32(void)
{
	int8_t trit; // трит
	trs_t tr1;	 // троичное число
	trs_t tr2;	 // троичное число

	printf("\r\n --- TEST #2  Operations TRITS-32 for VM SETUN-1958 --- \r\n");

	// t2.1 POW3
	printf("\r\nt2.1 --- POW3(...)\r\n");
	printf("pow3(3)  = %d\r\n", (int32_t)pow3(3));
	printf("pow3(18) = %d\r\n", (int32_t)pow3(18));

	// t2.2 Point address
	printf("\r\nt2.2 --- Point address\r\n");
	addr pMem;
	pMem = 0xffffffff;
	printf("pMem = 0xffffffff [%ji]\r\n", pMem);

	// t2.3
	printf("\r\nt2.3 --- TRIT-32\r\n");
	clear_full(&tr1);
	tr1.l = 1;
	tr1.t1 = ~0;
	tr1.t0 = 1;
	pln(1);
	view_short_reg(&tr1, "tr1");
	tr1.t0 = 1 << 4;
	pln(5);
	tr1.l = 5;
	view_short_reg(&tr1, "tr1");
	tr1.t0 = 1 << 8;
	pln(9);
	tr1.l = 9;
	view_short_reg(&tr1, "tr1");
	tr1.t0 = 1 << 17;
	pln(18);
	tr1.l = 18;
	view_short_reg(&tr1, "tr1");
	tr1.t0 = 1 << 31;
	pln(32);
	tr1.l = 32;
	view_short_reg(&tr1, "tr1");

	// t2.4
	printf("\r\nt2.4 --- get_trit(...)\r\n");
	tr1 = smtr("+0-0+0-0+");
	view_short_reg(&tr1, "tr1");
	trit = get_trit(tr1, 8);
	pt(trit, 8);
	trit = get_trit(tr1, 7);
	pt(trit, 7);
	trit = get_trit(tr1, 6);
	pt(trit, 6);
	trit = get_trit(tr1, 5);
	pt(trit, 5);
	trit = get_trit(tr1, 4);
	pt(trit, 4);
	trit = get_trit(tr1, 3);
	pt(trit, 3);
	trit = get_trit(tr1, 2);
	pt(trit, 2);
	trit = get_trit(tr1, 1);
	pt(trit, 1);
	trit = get_trit(tr1, 0);
	pt(trit, 0);

	// t2.4
	printf("\r\nt2.4 --- set_trit(...)\r\n");
	tr1 = set_trit(tr1, 8, 0);
	pt(0, 8);
	tr1 = set_trit(tr1, 7, 1);
	pt(1, 7);
	tr1 = set_trit(tr1, 6, 0);
	pt(0, 6);
	tr1 = set_trit(tr1, 5, -1);
	pt(-1, 5);
	tr1 = set_trit(tr1, 4, 0);
	pt(0, 4);
	tr1 = set_trit(tr1, 3, 1);
	pt(1, 3);
	tr1 = set_trit(tr1, 2, 0);
	pt(0, 2);
	tr1 = set_trit(tr1, 1, -1);
	pt(-1, 1);
	tr1 = set_trit(tr1, 0, 0);
	pt(0, 0);
	view_short_reg(&tr1, "tr1");

	// t2.5
	printf("\r\nt2.5 --- input TRITS-32s\r\n");
	printf("input tr1=[+000000000000000000000000000000-] \r\n");
	tr1 = smtr("+000000000000000000000000000000-");
	view_short_reg(&tr1, "tr1=");
	printf("input tr2=[-000000000+] \r\n");
	tr2 = smtr("-000000000+");
	view_short_reg(&tr2, "tr2=");

	// t2.6
	printf("\nt2.6 --- trit2int(...)\r\n");
	tr1 = smtr("00-");
	printf("tr1=[00-]\r\n");
	printf("trit2int(tr1) = % 2i\r\n", trit2int(tr1));
	tr1 = smtr("000");
	printf("tr1=[000]\r\n");
	printf("trit2int(tr1) = % 2i\r\n", trit2int(tr1));
	tr1 = smtr("00+");
	printf("tr1=[00+]\r\n");
	printf("trit2int(tr1) = % 2i\r\n", trit2int(tr1));

	// t2.7
	printf("\r\nt2.7 --- printf long int\r\n");
	long int ll = 0x00000001lu;
	printf("long int ll = 0x00000001lu\r\n");
	printf("ll = %lu\r\n", ll);
	ll <<= 1;
	printf("ll <<= 1; ll = %lu\r\n", ll);
	ll <<= 1;
	printf("ll <<= 1; ll = %lu\r\n", ll);
	printf("\r\n");

	// t2.8
	printf("\r\nt2.8 --- sgn_trs(...)\r\n");
	tr1 = smtr("+0000");
	printf("tr1=[+0000]\r\n");
	printf("sgn_trs(tr1) = % 2i\r\n", sgn_trs(tr1));
	tr1 = smtr("00000");
	printf("tr1=[00000]\r\n");
	printf("sgn_trs(tr1) = % 2i\r\n", sgn_trs(tr1));
	tr1 = smtr("-0000");
	printf("tr1=[-0000]\r\n");
	printf("sgn_trs(tr1) = % 2i\r\n", sgn_trs(tr1));

	trs_t w1;
	w1.l = 1;
	w1 = smtr("-");
	printf("w1=[-]\r\n");
	printf("sgn_trs(w1) = % 2i\r\n", sgn_trs(w1));
	w1 = smtr("0");
	printf("w1=[0]\r\n");
	printf("sgn_trs(w1) = % 2i\r\n", sgn_trs(w1));
	w1 = smtr("+");
	printf("w1=[+]\r\n");
	printf("sgn_trs(w1) = % 2i\r\n", sgn_trs(w1));

	// t2.9
	printf("\r\nt2.9 --- view_short_reg()\r\n");
	tr1 = smtr("+-0+-0+-0");
	view_short_reg(&tr1, "tr1 =");

	// t2.10
	printf("\r\nt2.10 --- slice_trs(...)\r\n");
	tr1 = smtr("+000000000000000000000000000000-");
	printf("tr1 := [+000000000000000000000000000000-]\r\n");
	tr2 = slice_trs(tr1, 0, 15);
	view_short_reg(&tr2, "tr2[15,0] =");
	printf("tr1 := [+000000000000000000000000000000-]\r\n");
	tr2 = slice_trs(tr1, 16, 31);
	view_short_reg(&tr2, "tr2[31,16] =");

	// t2.11
	printf("\nt2.11 --- copy_trs(...)\r\n");
	tr1 = smtr("+000000000000000000000000000000-");
	printf("tr1 := [+000000000000000000000000000000-]\r\n");
	tr2 = smtr("00000000000000000000000000000000");
	printf("tr2 := [00000000000000000000000000000000]\r\n");
	copy_trs(&tr1, &tr2);
	view_short_reg(&tr2, "tr2 =");
	tr1 = smtr("+00-00+00");
	printf("tr1 := [+00-00+00]\r\n");
	tr2 = smtr("00000000000000000000000000000000");
	printf("tr2 := [00000000000000000000000000000000]\r\n");
	copy_trs(&tr1, &tr2);
	view_short_reg(&tr2, "tr2 =");

	// t2.12
	printf("\r\nt2.12 --- inc_trs(...)\r\n");
	tr1 = smtr("+0-0+---");
	view_short_reg(&tr1, "tr1 =");
	inc_trs(&tr1);
	printf("inc_trs(&tr1)\r\n");
	view_short_reg(&tr1, "tr1 =");

	// t2.13
	printf("\r\nt2.13 --- dec_trs(...)\r\n");
	tr1 = smtr("+0-0-+0-");
	view_short_reg(&tr1, "tr1 =");
	dec_trs(&tr1);
	printf("dec_trs(&tr1)\r\n");
	view_short_reg(&tr1, "tr1 =");

	// t2.14
	printf("\r\nt2.14 --- add_trs(...)\r\n");
	tr1 = smtr("+---+");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("+0-+0");
	view_short_reg(&tr2, "tr2 =");
	tr1 = add_trs(tr1, tr2);
	printf("tr1 = add_trs(tr1,tr2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	// t2.14.1
	printf("\r\nt2.14.1 --- add_trs(...)\r\n");
	tr1 = smtr("-+0000000++++++++");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("0+0000000");
	view_short_reg(&tr2, "tr2 =");
	tr1 = add_trs(tr1, tr2);
	printf("tr1 = add_trs(tr1,tr2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	// t2.14.2
	printf("\r\nt2.14.2 --- add_trs(...)\r\n");
	tr1 = smtr("0+0000000+++++++++");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("0+0000000000000000");
	view_short_reg(&tr2, "tr2 =");
	tr1 = add_trs(tr1, tr2);
	printf("tr1 = add_trs(tr1,tr2)\n");
	view_short_reg(&tr1, "tr1 =");

	// t2.15
	printf("\r\nt2.15 --- sub_trs(...)\r\n");
	tr1 = smtr("+0-+00--");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("+0-0+--+");
	view_short_reg(&tr2, "tr2 =");
	tr1 = sub_trs(tr1, tr2);
	printf("tr1 = sub_trs(tr1,tr2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	// t2.15
	printf("\r\nt2.15 --- shift_trs(...)\r\n");
	tr1 = smtr("0000+0000");
	view_short_reg(&tr1, "tr1 =");
	tr1 = shift_trs(tr1, -2);
	printf("tr1 = shift_trs(tr1,-2)\r\n");
	view_short_reg(&tr1, "tr1 =");
	tr1 = shift_trs(tr1, -2);
	printf("tr1 = shift_trs(tr1,-2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	// t2.16
	printf("\r\nt2.16 --- mul_trs(...)\r\n");
	tr1 = smtr("+0000000000000000+");
	tr1.l = 18;
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("0-0000000000000000");
	tr2.l = 18;
	view_short_reg(&tr2, "tr2 =");

	tr2 = mul_trs(tr1, tr2);
	view_short_reg(&tr2, "tr2 =");

	// t2.17
	printf("\r\nt2.17 --- mul_trs(...)\r\n");
	tr1 = smtr("0000000++");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("000000000000000---");
	view_short_reg(&tr2, "tr2 =");
	tr2 = mul_trs(tr1, tr2);
	view_short_reg(&tr2, "tr2 =");

	// t2.18
	printf("\r\nt2.18 --- shift_long_trs(...)\r\n");
	long_trs_t ltr1;
	ltr1.l = 64;
	tr1 = smtr("0-00+0000");
	ltr1.t1 = tr1.t1;
	ltr1.t0 = tr1.t0;

	// printf("ltr1.t1=%p\r\n",(long int)ltr1.t1);
	// printf("ltr1.t0=%p\r\n",(long int)ltr1.t0);

	view_short_long_reg(&ltr1, "ltr1 =");
	ltr1 = shift_long_trs(ltr1, -2);
	printf("ltr1 = shift_long_trs(ltr1,-2)\r\n");
	view_short_long_reg(&ltr1, "ltr1 =");
	ltr1 = shift_long_trs(ltr1, -2);
	printf("ltr1 = shift_long_trs(ltr1,-2)\r\n");
	view_short_long_reg(&ltr1, "ltr1 =");

	printf("\r\n --- END TEST #2 --- \r\n");
}

/** *********************************************
 *  Тестирование виртуальной машины "Сетунь-1958"
 *  типов данных, функции
 *  ---------------------------------------------
 */
void ps(int8_t t, uint8_t p)
{
	printf("S[% 3i]=% 2i\r\n", p, t);
}

void Test3_Setun_Opers(void)
{
	int8_t trit;
	trs_t aa;
	trs_t ad1;
	trs_t ad2;
	trs_t addr;
	trs_t oper;
	trs_t exK;
	trs_t m0;
	trs_t m1;
	int8_t ret_exec;

	printf("\r\n --- TEST #3  Operations for VM SETUN-1958 --- \r\n\r\n");

	reset_setun_1958();

	// t3.1
	printf("\r\nt3.1 --- get_trit_setun(...)\r\n");
	S = smtr("+000000-000000000-");
	view_short_reg(&S, "S");
	trit = get_trit_setun(S, 1);
	ps(trit, 1);
	trit = get_trit_setun(S, 8);
	ps(trit, 8);
	trit = get_trit_setun(S, 18);
	ps(trit, 18);

	// t3.2
	printf("\r\nt3.2 --- set_trit_setun(...)\r\n");
	S = smtr("000000000000000000");
	S = set_trit_setun(S, 2, -1);
	ps(-1, 2);
	S = set_trit_setun(S, 5, 1);
	ps(1, 5);
	S = set_trit_setun(S, 18, -1);
	ps(-1, 18);
	view_short_reg(&S, "S");

	// t3.3
	printf("\r\nt3.3 --- sgn_trs(...)\r\n");
	S = smtr("+00000000000000000");
	printf("S=[+00000000000000000]\r\n");
	printf("sgn_trs(S) = % 2i\n", sgn_trs(S));
	S = smtr("000000000000000000");
	printf("S=[000000000000000000]\r\n");
	printf("sgn_trs(S) = % 2i\n", sgn_trs(S));
	S = smtr("-00000000000000000");
	printf("S=[-00000000000000000]\r\n");
	printf("sgn_trs(S) = % 2i\n", sgn_trs(S));

	// t3.4
	printf("\r\nt3.4 --- slice_trs_setun(...)\r\n");
	S = smtr("+0000000000000000-000000");
	K = slice_trs_setun(S, 1, 9);
	view_short_reg(&S, "S");
	view_short_reg(&K, "K[1,9] =");
	K = slice_trs_setun(S, 10, 18);
	view_short_reg(&S, "S");
	view_short_reg(&K, "K[10,18] =");

	// t3.5
	printf("\r\nt3.5 --- copy_trs_setun(...)\r\n");
	S = smtr("+0000000000000000-000000");
	copy_trs_setun(&S, &K);
	view_short_reg(&S, "S");
	view_short_reg(&K, "K");
	K = smtr("---------");
	copy_trs_setun(&K, &S);
	view_short_reg(&S, "S");
	view_short_reg(&K, "K");

	printf("\r\nt3.6 --- S = S + R\r\n");
	S = smtr("0000000000000+---+");
	R = smtr("0000000000000+0-+0");
	view_short_reg(&S, "S");
	view_short_reg(&R, "R");
	S = add_trs(S, R);
	view_short_reg(&S, "S=S+R");

	// t3.7
	printf("\r\nt3.7 --- next_address(...)\r\n");
	C = smtr("000--");
	view_short_reg(&C, "beg  C");
	for (int8_t i = 0; i < 10; i++)
	{
		C = next_address(C);
		view_short_reg(&C, "next C");
	}

	// t3.8
	printf("\r\nt3.8 --- control_trs(...)\r\n");
	trs_t Mem;
	Mem.l = 9;
	Mem = smtr("00000000+");
	view_short_reg(&Mem, "Mem");
	F = smtr("0000+");
	view_short_reg(&F, "F");
	K = control_trs(Mem);
	view_short_reg(&K, "K");

	Mem = smtr("000000000");
	view_short_reg(&Mem, "Mem");
	F = smtr("000++");
	view_short_reg(&F, "F");
	K = control_trs(Mem);
	view_short_reg(&K, "K");

	Mem = smtr("00000000-");
	view_short_reg(&Mem, "Mem");
	F = smtr("000++");
	view_short_reg(&F, "F");
	K = control_trs(Mem);
	view_short_reg(&K, "K");

	// t3.8
	printf("\r\nt3.8 --- st_fram(...)\r\n");
	//
	aa = smtr("00000");
	K = smtr("+0000000-");
	st_fram(aa, K);
	view_short_reg(&aa, "aa");
	view_short_reg(&K, "K");
	//
	aa = smtr("0000+");
	K = smtr("-0000000+");
	st_fram(aa, K);
	view_short_reg(&aa, "aa");
	view_short_reg(&K, "K");
	//
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	// t3.9
	printf("\r\nt3.9 --- ld_fram(...)\r\n");
	aa = smtr("000++");
	K = smtr("+000-000+");
	printf("st_fram(aa, K)\r\n");
	st_fram(aa, K);
	view_short_reg(&aa, "aa");
	view_short_reg(&K, "K");
	printf("ld_fram(aa)\r\n");
	K = ld_fram(aa);
	view_short_reg(&aa, "aa");
	view_short_reg(&K, "K");

	// t3.10 test Oper=k6..8[+00]: (A*)=>(S)
	printf("\r\nt3.10:  Oper=k6..8[+00]: (A*)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	addr = smtr("0000+");
	m1 = smtr("00000+000");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);

	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	// t3.11 test Oper=k6..8[-++]: (S)=>(A*)
	printf("\r\nt3.11:  Oper=k6..8[-++]: (S)=>(A*)\r\n");
	//
	reset_setun_1958();
	//
	S = smtr("-0000000+000-+0000");
	//
	addr = smtr("0000+");
	m1 = smtr("0000--++0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]r\\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();
	//
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	// t3.12 test Oper=k6..8[+-+]: (A*)=>(R)
	printf("\r\nt3.12:  Oper=k6..8[+-+]: (A*)=>(R)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	addr = smtr("0000+");
	m1 = smtr("00000+-+0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	// t3.13 test Oper=k6..8[0-0]: (A*)=>(F)
	printf("\r\nt3.12:  Oper=k6..8[0-0]: (A*)=>(F)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	addr = smtr("0000+");
	m1 = smtr("000000-00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	// t3.14 test Oper=k6..8[00-]: (F)=>(A*)
	printf("\r\nt3.14:  Oper=k6..8[00-]: (F)=>(A*)\r\n");
	//
	reset_setun_1958();
	//
	F = smtr("-000+");
	//
	addr = smtr("0000+");
	m1 = smtr("0000000-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();
	//
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	// t3.15 test Oper=k6..8[00+]: (C)=>(A*)
	printf("\r\nt3.15:  Oper=k6..8[00+]: (C)=>(A*)\r\n");
	//
	reset_setun_1958();
	//
	F = smtr("-000+");
	//
	addr = smtr("0000+");
	m1 = smtr("0000000+0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();
	//
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	// t3.16 test Oper=k6..8[+0+]: (S)+(A*)=>(S)
	printf("\r\nt3.16:  Oper=k6..8[+0+]: (S)+(A*)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("0000000+0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("00000000000000000+");
	//
	addr = smtr("0000+");
	m1 = smtr("00000+0+0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	// t3.17 test Oper=k6..8[+0-]: (S)-(A*)=>(S)
	printf("\r\nt3.17:  Oper=k6..8[+0-]: (S)-(A*)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("0000000++");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("000000000000000+0+");
	view_short_reg(&S, "S=");
	//
	addr = smtr("0000+");
	m1 = smtr("00000+0-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	// t3.18 test Oper=k6..8[++0]: (S)=>(R); (A*)(R)=>(S)
	printf("\r\nt3.18:  Oper=k6..8[++0]: (S)=>(R); (A*)(R)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("0+0000000");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("00000000+++0000000");
	view_short_reg(&S, "S=");
	//
	addr = smtr("0000+");
	m1 = smtr("00000++00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	// t3.19 test Oper=k6..8[+++]: (S)+(A*)(R)=>(S)
	printf("\r\nt3.19:  Oper=k6..8[+++]: (S)+(A*)(R)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("000000--0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	//
	R = smtr("0000000000000000+0");
	view_short_reg(&R, "R=");
	//
	addr = smtr("0000+");
	m1 = smtr("00000+++0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	// t3.21 test Oper=k6..8[++-]: (A*)+(S)(R)=>(S)=>(S)
	printf("\r\nt3.21:  Oper=k6..8[++-]: (A*)+(S)(R)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("000000--0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	//
	R = smtr("0000000000000000+0");
	view_short_reg(&R, "R=");
	//
	addr = smtr("0000+");
	m1 = smtr("00000++-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	printf("   \r\n");

	// t3.22 test Oper=k6..8[+-0]: (A*)[x](S)=>(S)
	printf("\r\nt3.22:  Oper=k6..8[+-0]: (A*)[x](S)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("000000--0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	//
	R = smtr("0000000000000000+0");
	view_short_reg(&R, "R=");
	//
	addr = smtr("0000+");
	m1 = smtr("00000+-00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	// view_short_regs();

	// t3.23 test Oper=k6..8[-+0]: S сд. (A*)=>(S)
	printf("\r\nt3.23:  Oper=k6..8[-+0]: S сд. (A*)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("-000-");
	m0 = smtr("+0000000-+000-000-");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	//
	// R = smtr("0000000000000000+0");
	// view_short_reg(&R, "R=");
	//
	addr = smtr("0000+");
	m1 = smtr("-000-0-+00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	view_short_reg(&S, "S=");
	// view_short_regs();

	// t3.24 test Oper=k6..8[-+-]: Норм.(S)=>(A*); (N)=>(S)
	printf("\r\nt3.24:  Oper=k6..8[-+-]:  Норм.(S)=>(A*); (N)=>(S)\r\n");
	//
	reset_setun_1958();
	//
	addr = smtr("00000");
	m0 = smtr("00000000-");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	//
	// R = smtr("0000000000000000+0");
	// view_short_reg(&R, "R=");
	//
	addr = smtr("0000+");
	m1 = smtr("00000-+-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	printf("S.l=%d\r\n", S.l);
	view_short_reg(&S, "S=");

	addr = smtr("00000");
	view_elem_fram(addr);

	// view_short_regs();

	// t3.25 test Oper=k6..8[-00]:
	printf("\r\nt3.25:  Oper=k6..8[-00]: \r\n");

	printf("\r\n --- END TEST #3 --- \r\n\r\n");
}

void Test4_Setun_Opers(void)
{
	int8_t trit;
	trs_t aa;
	trs_t ad1;
	trs_t ad2;
	trs_t addr;
	trs_t oper;
	trs_t exK;
	trs_t m0;
	trs_t m1;
	int8_t ret_exec;

	printf("\r\n --- TEST #4  Operations for VM SETUN-1958 --- \r\n\r\n");

	// t4.1 test Oper=k6..8[+00]: (A*)=>(S)
	printf("\r\nt4.1:  Oper=k6..8[+00]: (A*)=>(S)\r\n");
	//
	reset_setun_1958();

	printf("\r\n");
	// FRAM(-1) test
	addr = smtr("-0000");
	m0 = smtr("00000000+");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	addr = smtr("-000+");
	m0 = smtr("-0000000+");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//
	addr = smtr("-00+-");
	m0 = smtr("0000000+0");
	st_fram(addr, m0);
	view_elem_fram(addr);

	/* Begin address fram */
	addr = smtr("0000+");
	m1 = smtr("-000-+000");
	st_fram(addr, m1);
	view_short_reg(&m1, "m1");
	view_elem_fram(addr);

	printf("\r\n");

	// work VM Setun-1958
	C = smtr("0000+");
	K = ld_fram(C);
	view_short_reg(&K, "C(0000+)");

	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("\r\n [status: OK']\r\n");
	if (ret_exec != 0)
		printf("\r\n [status: ERR#%d]\r\n", ret_exec);

	printf("\r\n --- END TEST #4 --- \r\n");
}

void Test5_Setun_Load(void)
{
	printf("\r\n --- TEST #5  Load program FT1,FT2 for VM SETUN-1958 --- \r\n\r\n");

	FILE *file_lst;

	/* Переменная, в которую поочередно будут помещаться считываемые строки */
	char str[80] = {0};

	/* Указатель, в который будет помещен адрес массива, в который считана */
	/* строка, или NULL если достигнут коней файла или произошла ошибка */
	char *estr;

	file_lst = fopen("ur1/00_ip5.lst", "r");
	if (file_lst == NULL)
	{
		printf("ERR fopen ur1/00_ip5.lst\r\n");
		return;
	}
	else
	{
		printf("fopen: ur1/00_ip5.lst\r\n");

		/* Чтение (построчно) данных из файла в бесконечном цикле */
		while (1)
		{
			/* Чтение одной строки  из файла */
			estr = fgets(str, sizeof(str), file_lst);

			/* Проверка на конец файла или ошибку чтения */
			if (estr == NULL)
			{
				/* Проверяем, что именно произошло: кончился файл */
				/* или это ошибка чтения */
				if (feof(file_lst) != 0)
				{
					/* Если файл закончился, выводим сообщение о завершении */
					/* чтения и выходим из бесконечного цикла */
					printf("\r\nЧтение файла закончено\r\n");
					break;
				}
				else
				{
					/* Если при чтении произошла ошибка, выводим сообщение */
					/* об ошибке и выходим из бесконечного цикла */
					printf("\r\nОшибка чтения из файла\r\n");
					break;
				}
			}

			/* ---------------------------------
			 * Загрузить из файла тест-программу
			 * ---------------------------------
			 */
			printf("\r\n --- Load ur1/%s --- \r\n", str);

			FILE *file;
			char path_str[160] = {0};
			uint8_t cmd[20];
			trs_t inr;
			trs_t dst;

			inr = smtr("0---0"); /* cчетчик адреса коротких слов */
			trs_t sum;
			trs_t tmp;

			tmp.l = 18;
			tmp.t1 = 0;
			tmp.t0 = 0;

			int64_t dsun = 0;
			sum.l = 18;
			sum.t1 = 0;
			sum.t0 = 0;
			//
			dst.l = 9;
			MR.l = 18;
			int i = 0;

			strcat(path_str, "ur1/");
			if (str[strlen(str) - 1] == 0x0A)
			{
				str[strlen(str) - 1] = 0;
			}
			str[strlen(str)] = 0;
			strcat(path_str, str);

			file = fopen(path_str, "r");
			if (file == NULL)
			{
				printf("ERR fopen %s\r\n", path_str);
				return;
			}

			while (fscanf(file, "%s", cmd) != EOF)
			{
				cmd_str_2_trs(cmd, &dst);
				sum = add_trs(sum, dst);
				i += 1;

				dsun += trs2digit(dst);

				debug_print("%s -> [", cmd);
				trs2str(dst);
				debug_print("]");
				if (DEBUG > 0)
				{
					view_short_reg(&inr, " addr");
				}
				else
				{
					printf("\r\n");
				}

				st_fram(inr, dst);
				inr = next_address(inr);
			}
			fclose(file);

			printf("\r\n i=%i\n", i);

			/* Печать контрольных сумм */
			printf("\r\n");
			view_checksum_setun(sum);
			//
		}
	}
	fclose(file_lst);
	printf("fclose: ur1/00_ip5.lst\r\n");

	printf("\r\n --- END TEST #5 --- \r\n");
}

void Test6_Setun_Load(void)
{
	printf("\r\n --- TEST #6  Load program FT1,FT2 for VM SETUN-1958 --- \r\n\r\n");

	FILE *file_lst;

	/* Переменная, в которую поочередно будут помещаться считываемые строки */
	char str[80] = {0};

	/* Указатель, в который будет помещен адрес массива, в который считана */
	/* строка, или NULL если достигнут коней файла или произошла ошибка */
	char *estr;

	file_lst = fopen("software/ip5_in_out_10_3/00_ip5_in_out_10_3.lst", "r");
	if (file_lst == NULL)
	{
		printf("ERR fopen software/ip5_in_out_10_3/00_ip5_in_out_10_3.lst\r\n");
		return;
	}
	else
	{
		printf("fopen: software/ip5_in_out_10_3/00_ip5_in_out_10_3.lst\r\n");

		/* Чтение (построчно) данных из файла в бесконечном цикле */
		while (1)
		{
			/* Чтение одной строки  из файла */
			estr = fgets(str, sizeof(str), file_lst);

			/* Проверка на конец файла или ошибку чтения */
			if (estr == NULL)
			{
				/* Проверяем, что именно произошло: кончился файл */
				/* или это ошибка чтения */
				if (feof(file_lst) != 0)
				{
					/* Если файл закончился, выводим сообщение о завершении */
					/* чтения и выходим из бесконечного цикла */
					printf("\r\nЧтение файла закончено\r\n");
					break;
				}
				else
				{
					/* Если при чтении произошла ошибка, выводим сообщение */
					/* об ошибке и выходим из бесконечного цикла */
					printf("\r\nОшибка чтения из файла\r\n");
					break;
				}
			}

			/* ---------------------------------
			 * Загрузить из файла тест-программу
			 * ---------------------------------
			 */
			printf("\r\n --- Load software/ip5_in_out_10_3/%s --- \r\n", str);

			FILE *file;
			char path_str[160] = {0};
			uint8_t cmd[20];
			trs_t inr;
			trs_t dst;

			inr = smtr("0---0"); /* cчетчик адреса коротких слов */
			trs_t sum;
			trs_t tmp;

			tmp.l = 18;
			tmp.t1 = 0;
			tmp.t0 = 0;

			int64_t dsun = 0;
			sum.l = 18;
			sum.t1 = 0;
			sum.t0 = 0;
			//
			dst.l = 9;
			MR.l = 18;
			int i = 0;

			strcat(path_str, "software/ip5_in_out_10_3/");
			if (str[strlen(str) - 1] == 0x0A)
			{
				str[strlen(str) - 1] = 0;
			}
			str[strlen(str)] = 0;
			strcat(path_str, str);

			file = fopen(path_str, "r");
			if (file == NULL)
			{
				printf("ERR fopen %s\r\n", path_str);
				return;
			}

			while (fscanf(file, "%s", cmd) != EOF)
			{
				cmd_str_2_trs(cmd, &dst);
				sum = add_trs(sum, dst);
				i += 1;

				dsun += trs2digit(dst);

				debug_print("%s -> [", cmd);
				trs2str(dst);
				debug_print("]");
				if (DEBUG > 0)
				{
					view_short_reg(&inr, " addr");
				}
				else
				{
					printf("\r\n");
				}

				st_fram(inr, dst);
				inr = next_address(inr);
			}
			fclose(file);

			printf("\r\n i=%i\r\n", i);

			/* Печать контрольных сумм */
			printf("\r\n");
			view_checksum_setun(sum);
			//
		}
	}
	fclose(file_lst);
	printf("fclose: software/ip5_in_out_10_3/00_ip5_in_out.lst\r\n");

	printf("\r\n --- END TEST #6 --- \r\n");
}

void Test7_Setun_Load(void)
{
	printf("\r\n --- TEST #7  Load program FT1,FT2 for VM SETUN-1958 --- \r\n\r\n");

	FILE *file_lst;

	/* Переменная, в которую поочередно будут помещаться считываемые строки */
	char str[80] = {0};

	/* Указатель, в который будет помещен адрес массива, в который считана */
	/* строка, или NULL если достигнут коней файла или произошла ошибка */
	char *estr;

	file_lst = fopen("software/ip5_in_out_3_10/00_ip5_in_out_3_10.lst", "r");
	if (file_lst == NULL)
	{
		printf("ERR fopen software/ip5_in_out_3_10/00_ip5_in_out_3_10.lst\r\n");
		return;
	}
	else
	{
		printf("fopen: software/ip5_in_out_3_10/00_ip5_in_out_3_10.lst\r\n");

		/* Чтение (построчно) данных из файла в бесконечном цикле */
		while (1)
		{
			/* Чтение одной строки  из файла */
			estr = fgets(str, sizeof(str), file_lst);

			/* Проверка на конец файла или ошибку чтения */
			if (estr == NULL)
			{
				/* Проверяем, что именно произошло: кончился файл */
				/* или это ошибка чтения */
				if (feof(file_lst) != 0)
				{
					/* Если файл закончился, выводим сообщение о завершении */
					/* чтения и выходим из бесконечного цикла */
					printf("\r\nЧтение файла закончено\r\n");
					break;
				}
				else
				{
					/* Если при чтении произошла ошибка, выводим сообщение */
					/* об ошибке и выходим из бесконечного цикла */
					printf("\r\nОшибка чтения из файла\r\n");
					break;
				}
			}

			/* ---------------------------------
			 * Загрузить из файла тест-программу
			 * ---------------------------------
			 */
			printf("\r\n --- Load software/ip5_in_out_3_10/%s --- \r\n", str);

			FILE *file;
			char path_str[160] = {0};
			uint8_t cmd[20];
			trs_t inr;
			trs_t dst;

			inr = smtr("0---0"); /* cчетчик адреса коротких слов */
			trs_t sum;
			trs_t tmp;

			tmp.l = 18;
			tmp.t1 = 0;
			tmp.t0 = 0;

			int64_t dsun = 0;
			sum.l = 18;
			sum.t1 = 0;
			sum.t0 = 0;
			//
			dst.l = 9;
			MR.l = 18;
			int i = 0;

			strcat(path_str, "software/ip5_in_out_3_10/");
			if (str[strlen(str) - 1] == 0x0A)
			{
				str[strlen(str) - 1] = 0;
			}
			str[strlen(str)] = 0;
			strcat(path_str, str);

			file = fopen(path_str, "r");
			if (file == NULL)
			{
				printf("ERR fopen %s\r\n", path_str);
				return;
			}

			cmd_str_2_trs(cmd, &dst);
			sum = add_trs(sum, dst);
			i += 1;

			debug_print("%s -> [", cmd);
			trs2str(dst);
			debug_print("]");
			if (DEBUG > 0)
			{
				view_short_reg(&inr, " addr");
			}
			else
			{
				printf("\r\n");
			}

			st_fram(inr, dst);
			inr = next_address(inr);
			fclose(file);

			/* Печать контрольных сумм */
			printf("\r\n");
			view_checksum_setun(sum);
		}
		// printf("\r\n i=%i\r\n", i);
		//
	}

	fclose(file_lst);
	printf("fclose: software/ip5_in_out_3_10/00_ip5_in_out_3_10.lst\r\n");
	printf("\r\n --- END TEST #7 --- \r\n");
}

const char *get_file_ext(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return "";
	return dot + 1;
}

void LoadSWSetun(void)
{
	printf("\r\n --- Load software anf DUMP FRAM  for VM SETUN-1958 --- \r\n\r\n");

	FILE *file_lst;

	/* Переменная, в которую поочередно будут помещаться считываемые строки */
	char str[80] = {0};

	/* Указатель, в который будет помещен адрес массива, в который считана */
	/* строка, или NULL если достигнут коней файла или произошла ошибка */
	char *estr;

	file_lst = fopen("software/tests/01-test-fram.lst", "r");
	if (file_lst == NULL)
	{
		printf("ERR fopen software/tests/01-test-fram.lst\r\n");
		return;
	}
	else
	{
		printf("fopen: software/tests/01-test-fram.lst\r\n");

		/* Чтение (построчно) данных из файла в бесконечном цикле */
		while (1)
		{
			/* Чтение одной строки  из файла */
			estr = fgets(str, sizeof(str), file_lst);

			/* Проверка на конец файла или ошибку чтения */
			if (estr == NULL)
			{
				/* Проверяем, что именно произошло: кончился файл */
				/* или это ошибка чтения */
				if (feof(file_lst) != 0)
				{
					/* Если файл закончился, выводим сообщение о завершении */
					/* чтения и выходим из бесконечного цикла */
					printf("\r\nЧтение файла закончено\r\n");
					break;
				}
				else
				{
					/* Если при чтении произошла ошибка, выводим сообщение */
					/* об ошибке и выходим из бесконечного цикла */
					printf("\r\nОшибка чтения из файла\r\n");
					break;
				}
			}

			/* ---------------------------------
			 * Загрузить из файла тест-программу
			 * ---------------------------------
			 */
			printf("\r\n Load software/tests/%s\r\n", str);

			FILE *file;
			char path_str[160] = {0};
			uint8_t cmd[20];
			trs_t inr;
			trs_t dst;

			inr = smtr("0---0"); /* cчетчик адреса коротких слов */
			trs_t sum;
			trs_t tmp;

			tmp.l = 18;
			tmp.t1 = 0;
			tmp.t0 = 0;

			int64_t dsun = 0;
			sum.l = 18;
			sum.t1 = 0;
			sum.t0 = 0;
			//
			dst.l = 9;
			MR.l = 18;
			int i = 0;

			strcat(path_str, "software/tests/");
			if (str[strlen(str) - 1] == 0x0A)
			{
				str[strlen(str) - 1] = 0;
			}
			str[strlen(str)] = 0;
			strcat(path_str, str);

			file = fopen(path_str, "r");
			if (file == NULL)
			{
				printf("ERR fopen %s\r\n", path_str);
				return;
			}

			while (fscanf(file, "%s", cmd) != EOF)
			{
				cmd_str_2_trs(cmd, &dst);
				sum = add_trs(sum, dst);
				i += 1;

				dsun += trs2digit(dst);

				debug_print("%s -> [", cmd);
				trs2str(dst);
				debug_print("]");
				if (DEBUG > 0)
				{
					view_short_reg(&inr, " addr");
				}
				else
				{
					printf("\r\n");
				}

				st_fram(inr, dst);
				inr = next_address(inr);
			}
			fclose(file);

			printf("\r\n i=%i\r\n", i);

			/* Печать контрольных сумм */
			printf("\r\n");
			view_checksum_setun(sum);
		}
	}
	fclose(file_lst);

	// dump_fram_zone(smtr("0"));
	// dump_fram_zone(smtr("+"));
	// dump_fram_zone(smtr("-"));

	printf("\r\n END Load software\r\n");
}

void LoadFileListToPaperTxt(char *pathcataloglst, char *pathfilelst, char *pathfiletxt)
{
	FILE *file_lst;
	FILE *file_txt;

	/* Переменная, в которую поочередно будут помещаться считываемые строки */
	char str[80] = {0};

	/* Указатель, в который будет помещен адрес массива, в который считана */
	/* строка, или NULL если достигнут коней файла или произошла ошибка */
	char *estr;

	file_txt = fopen(pathfiletxt, "w");
	if (file_lst == NULL)
	{
		printf("ERR fopen %s\r\n", pathfiletxt);
		return;
	}

	file_lst = fopen(pathfilelst, "r");
	if (file_lst == NULL)
	{
		printf("ERR fopen %s\r\n", pathfilelst);
		return;
	}
	else
	{
		printf("Read file list: %s\r\n", pathfilelst);

		/* Чтение (построчно) данных из файла в бесконечном цикле */
		while (1)
		{
			/* Чтение одной строки  из файла */
			estr = fgets(str, sizeof(str), file_lst);

			/* Проверка на конец файла или ошибку чтения */
			if (estr == NULL)
			{
				/* Проверяем, что именно произошло: кончился файл */
				/* или это ошибка чтения */
				if (feof(file_lst) != 0)
				{
					/* Если файл закончился, выводим сообщение о завершении */
					/* чтения и выходим из бесконечного цикла */
					// printf("\r\nЧтение файла закончено\r\n");
					break;
				}
				else
				{
					/* Если при чтении произошла ошибка, выводим сообщение */
					/* об ошибке и выходим из бесконечного цикла */
					// printf("\nОшибка чтения из файла\r\n");
					break;
				}
			}

			/* ---------------------------------
			 * Загрузить из файла тест-программу
			 * ---------------------------------
			 */
			printf("Read file: %s", str);

			FILE *file;
			char path_str[160] = {0};
			uint8_t cmd[20];
			trs_t inr;
			trs_t dst;

			inr = smtr("0---0"); /* cчетчик адреса коротких слов */
			trs_t sum;
			trs_t tmp;

			tmp.l = 18;
			tmp.t1 = 0;
			tmp.t0 = 0;

			int64_t dsun = 0;
			sum.l = 18;
			sum.t1 = 0;
			sum.t0 = 0;
			//
			dst.l = 9;
			MR.l = 18;
			int i = 0;

			strcat(path_str, pathcataloglst);
			strcat(path_str, "/");
			if (str[strlen(str) - 1] == 0x0A)
			{
				str[strlen(str) - 1] = 0;
			}
			str[strlen(str)] = 0;
			strcat(path_str, str);

			file = fopen(path_str, "r");
			if (file == NULL)
			{
				printf("ERR fopen %s\r\n", path_str);
				return;
			}

			while (fscanf(file, "%s", cmd) != EOF)
			{
				uint8_t offset4_7 = 4;
				uint8_t prlile[7];
				trs_t dst;
				trs_t trit2;
				//
				cmd_str_2_trs(cmd, &dst);
				trit2.l = 2;
				//
				trit2 = set_trit_setun(trit2, 1, 0);
				trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 1));
				memset(prlile, 0, sizeof(prlile));
				memmove(prlile, tab4_7[trs2digit(trit2) + offset4_7], sizeof(prlile));
				fwrite(prlile, 1, strlen(prlile), file_txt);
				fwrite("\r\n", 1, strlen("\r\n"), file_txt);
				//
				trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 2));
				trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 3));
				memset(prlile, 0, sizeof(prlile));
				memmove(prlile, tab4_7[trs2digit(trit2) + offset4_7], sizeof(prlile));
				fwrite(prlile, 1, strlen(prlile), file_txt);
				fwrite("\r\n", 1, strlen("\r\n"), file_txt);
				//
				trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 4));
				trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 5));
				memset(prlile, 0, sizeof(prlile));
				memmove(prlile, tab4_7[trs2digit(trit2) + offset4_7], sizeof(prlile));
				fwrite(prlile, 1, strlen(prlile), file_txt);
				fwrite("\r\n", 1, strlen("\r\n"), file_txt);
				//
				trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 6));
				trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 7));
				memset(prlile, 0, sizeof(prlile));
				memmove(prlile, tab4_7[trs2digit(trit2) + offset4_7], sizeof(prlile));
				fwrite(prlile, 1, strlen(prlile), file_txt);
				fwrite("\r\n", 1, strlen("\r\n"), file_txt);
				//
				trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 8));
				trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 9));
				memset(prlile, 0, sizeof(prlile));
				memmove(prlile, tab4_7[trs2digit(trit2) + offset4_7], sizeof(prlile));
				fwrite(prlile, 1, strlen(prlile), file_txt);
				fwrite("\r\n", 1, strlen("\r\n"), file_txt);
				//
				memset(prlile, 0, sizeof(prlile));
				memmove(prlile, "O__._O", sizeof(prlile));
				fwrite(prlile, 1, strlen(prlile), file_txt);
				fwrite("\r\n", 1, strlen("\r\n"), file_txt);
			}
			fclose(file);

			/* Промежуток между зонами на перфоленте */
			for (int j = 0; j < 5; j++)
			{
				fwrite("___.__\r\n", 1, strlen("___.__\r\n"), file_txt);
			}
		}
	}

	/* Закрыть файлы */
	fclose(file_lst);
	fclose(file_txt);

	printf("\r\nWrite file: %s\r\n", pathfiletxt);
}

int ConvertSWtoPaper(char *path_lst, char *path_txt)
{

	int res = 0;

	char a_fileName[80];

	printf(" --- ConvertSWtoPaper ---\r\n");

	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(path_lst)) != NULL)
	{
		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL)
		{
			char file_ext[10];
			strcpy(file_ext, get_file_ext(ent->d_name));
			/* Найти список файлов */
			if (strcmp(file_ext, "lst") == 0)
			{
				strcpy(a_fileName, ent->d_name);
				res = 0; /* Ok' */
				break;
			}
		}
		closedir(dir);
	}
	else
	{
		/* could not open directory */
		perror("Could not open directory");
		res = 1; /* Error#1 */
		return EXIT_FAILURE;
	}

	/* Прочитать файлы из каталога и преобразовать в папер */
	if (res == 0)
	{
		/* Проверить каталог */
		if ((dir = opendir(path_txt)) != NULL)
		{
			res = 0; /* Ok' */
		}
		else
		{
			res = 2; /* Error#2 */
		}
	}

	/* Прочитать файлы программы из каталога и преобразовать в paper.txt */
	if (res == 0)
	{
		char pathfile1[255];
		char pathfile2[255];
		// clear
		memset(pathfile1, 0, sizeof(pathfile1));
		memset(pathfile2, 0, sizeof(pathfile2));
		// path file1
		strcat(pathfile1, path_lst);
		strcat(pathfile1, "/");
		strcat(pathfile1, a_fileName);
		// path file2
		strcat(pathfile2, path_txt);
		strcat(pathfile2, "/");
		strcat(pathfile2, "paper.txt");

		LoadFileListToPaperTxt(path_lst, pathfile1, pathfile2);
	}

	return res; /* Ok' */
}

int DumpFileTxs(char *pathfiletxs)
{
	FILE *file_txs;

	file_txs = fopen(pathfiletxs, "r");
	if (file_txs == NULL)
	{
		printf("ERR fopen %s\r\n", pathfiletxs);
		return 1;
	}

	printf("Read commands from file.txs: %s\r\n", pathfiletxs);

	uint8_t cmd[20];
	trs_t inr;
	trs_t dst;

	inr = smtr("0---0"); /* cчетчик адреса коротких слов */
	trs_t sum;
	trs_t tmp;

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;
	//
	dst.l = 9;
	MR.l = 18;
	int i = 0;

	while (fscanf(file_txs, "%s", cmd) != EOF)
	{
		cmd_str_2_trs(cmd, &dst);
		sum = add_trs(sum, dst);
		i += 1;

		dsun += trs2digit(dst);

		st_fram(inr, dst);
		inr = next_address(inr);
	}
	/* Закрыть файл */
	fclose(file_txs);

	dump_fram_zone(smtr("0"));

	printf("\r\n i=%i\r\n", i);

	/* Печать контрольных сумм */
	printf("\r\n");
	view_checksum_setun(sum);

	return 0; /* Ok' */
}

void Test9_Setun_Electrified_Typewriter(void)
{
	printf("\r\n --- TEST #9 electrified_typewriter() and dump --- \r\n");

	trs_t inr;
	trs_t cp;
	clear(&cp);
	cp.l = 3;
	cp = set_trit_setun(cp, 1, -1);
	cp = set_trit_setun(cp, 2, -1);
	cp = set_trit_setun(cp, 3, -1);

	uint8_t cc;

	cp = set_trit_setun(cp, 1, 1);
	cp = set_trit_setun(cp, 2, 1);
	cp = set_trit_setun(cp, 3, 0);
	electrified_typewriter(cp, 0);

	cp = set_trit_setun(cp, 1, -1);
	cp = set_trit_setun(cp, 2, -1);
	cp = set_trit_setun(cp, 3, -1);
	for (cc = 0; cc < 27; cc++)
	{
		if (trs2digit(cp) != 12 || trs2digit(cp) != 11)
		{
			electrified_typewriter(cp, 0);
		}
		inc_trs(&cp);
	}

	cp = set_trit_setun(cp, 1, 1);
	cp = set_trit_setun(cp, 2, 1);
	cp = set_trit_setun(cp, 3, -1);
	electrified_typewriter(cp, 0);

	cp = set_trit_setun(cp, 1, -1);
	cp = set_trit_setun(cp, 2, -1);
	cp = set_trit_setun(cp, 3, -1);
	for (cc = 0; cc < 27; cc++)
	{
		if (trs2digit(cp) != 12 || trs2digit(cp) != 11)
		{
			electrified_typewriter(cp, 0);
		}
		inc_trs(&cp);
	}

	cp = set_trit_setun(cp, 1, 1);
	cp = set_trit_setun(cp, 2, 1);
	cp = set_trit_setun(cp, 3, 0);
	electrified_typewriter(cp, 1);

	cp = set_trit_setun(cp, 1, -1);
	cp = set_trit_setun(cp, 2, -1);
	cp = set_trit_setun(cp, 3, -1);
	for (cc = 0; cc < 27; cc++)
	{
		if (trs2digit(cp) != 12 || trs2digit(cp) != 11)
		{
			electrified_typewriter(cp, 1);
		}
		inc_trs(&cp);
	}

	cp = set_trit_setun(cp, 1, 1);
	cp = set_trit_setun(cp, 2, 1);
	cp = set_trit_setun(cp, 3, -1);
	electrified_typewriter(cp, 1);

	cp = set_trit_setun(cp, 1, -1);
	cp = set_trit_setun(cp, 2, -1);
	cp = set_trit_setun(cp, 3, -1);
	for (cc = 0; cc < 27; cc++)
	{
		if (trs2digit(cp) != 12 || trs2digit(cp) != 11)
		{
			electrified_typewriter(cp, 1);
		}
		inc_trs(&cp);
	}

	printf("\r\n --- smtr() --- \r\n");
	R = smtr("-+0+-");
	view_short_reg(&R, "R");

	R = smtr("---------");
	view_short_reg(&R, "R");

	R = smtr("+++++++++");
	view_short_reg(&R, "R");

	// -----------------------------------------------
	trs_t exK;
	trs_t oper;
	trs_t m0;
	trs_t m1;
	trs_t addr;
	trs_t ad1;
	trs_t ad2;
	uint8_t ret_exec;

	// t19 test Oper=k6..8[+00]: (A*)=>(S)
	printf("\r\nt19: test Oper=k6..8[+00]: (A*)=>(S)\r\n");

	reset_setun_1958();

	F = smtr("000++");

	addr = smtr("000++");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0++++++++");
	st_fram(addr, m0);

	addr = smtr("-000-");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0++++++++0-------");
	st_fram(addr, m0);

	addr = smtr("000-0");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0--------");
	st_fram(addr, m0);

	addr = smtr("00000");
	view_short_reg(&addr, "addr=");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);

	addr = smtr("0000+");
	m1 = smtr("-000-0+000");
	st_fram(addr, m1);

	/* Begin address fram */
	C = smtr("0000+");

	printf("\r\nreg C = 00001\r\n");

	/**
	 * work VM Setun-1958
	 */
	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);
	printf("\r\n");

	addr = smtr("0000+");
	m1 = smtr("-000-+000");
	st_fram(addr, m1);

	/* Begin address fram */
	C = smtr("0000+");

	printf("\r\nreg C = 00001\r\n");

	/**
	 * work VM Setun-1958
	 */
	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);
	printf("\r\n");

	view_short_regs();

	/* Begin address fram */
	C = smtr("000+0");
	printf("\r\nreg C = 00010\r\n");

	addr = smtr("000+0");
	m1 = smtr("00000+0--");
	st_fram(addr, m1);

	/**
	 * work VM Setun-1958
	 */
	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);
	printf("\r\n");

	ad1 = smtr("000-0");
	ad2 = smtr("00+-0");
	view_fram(ad1, ad2);

	view_short_regs();

	/* Begin address fram */
	C = smtr("000++");
	printf("\r\nreg C = 00011\r\n");

	addr = smtr("000++");
	m1 = smtr("00000++00");
	st_fram(addr, m1);

	/**
	 * work VM Setun-1958
	 */
	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);
	printf("\r\n");

	ad1 = smtr("000-0");
	ad2 = smtr("00+-0");
	view_fram(ad1, ad2);

	view_short_regs();

	// t20 test mul_trs()

	trs_t res;
	res.l = 18;
	res.t1 = 0;
	res.t0 = 0;

	S = smtr("0000000-0000000+++");
	R = smtr("000000000-00+000-");
	res = mul_trs(S, R);

	view_short_reg(&S, " S");
	view_short_reg(&R, " R");
	view_short_reg(&res, "res = S*R");

	// t21 test Oper=k6..8[+00]: (A*)=>(S)
	printf("\r\nt19: test Oper=k6..8[+00]: (A*)=>(S)\r\n");

	reset_setun_1958();

	F = smtr("000++");

	addr = smtr("000++");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0++++++++");
	st_fram(addr, m0);

	addr = smtr("000--");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0++++++++");
	st_fram(addr, m0);

	addr = smtr("000-0");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0--------");
	st_fram(addr, m0);

	addr = smtr("00000");
	view_short_reg(&addr, "addr=");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);

	addr = smtr("0000+");
	m1 = smtr("00000++00");
	st_fram(addr, m1);

	S = smtr("000000000-00+000-+");

	/* Begin address fram */
	C = smtr("0000+");

	printf("\r\nreg C = 00001\r\n");

	/**
	 * work VM Setun-1958
	 */
	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);
	printf("\r\n");

	// views
	ad1 = smtr("000-0");
	ad2 = smtr("00+-0");
	view_fram(ad1, ad2);

	view_short_regs();

	printf("\r\nt22: test Oper=k6..8[-+-]: Норм.(S)=>(A*); (N)=>(S)\r\n");

	reset_setun_1958();

	F = smtr("000++");

	addr = smtr("000++");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0++++++++");
	st_fram(addr, m0);

	addr = smtr("000--");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0++++++++");
	st_fram(addr, m0);

	addr = smtr("000-0");
	view_short_reg(&addr, "addr=");
	m0 = smtr("0--------");
	st_fram(addr, m0);

	addr = smtr("00000");
	view_short_reg(&addr, "addr=");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);

	addr = smtr("0000+");
	m1 = smtr("00000-+-0");
	st_fram(addr, m1);

	S = smtr("000000000-00+000-+");
	view_short_reg(&S, "S=");

	/* Begin address fram */
	C = smtr("0000+");

	printf("\r\nreg C = 00001\r\n");

	/**
	 * work VM Setun-1958
	 */
	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);
	printf("\r\n");

	// views
	ad1 = smtr("000-0");
	ad2 = smtr("00+-0");
	// view_fram(ad1, ad2);

	view_short_regs();

	// t22 test DRUM
	printf("t22 test zone for drum()\r\n");
	uint32_t nz;

	printf(" ad1 ='000+'");
	ad1 = smtr("000+");
	nz = zone_drum_to_index(ad1);
	printf(" nz = %i\r\n", nz);

	printf(" ad1 ='00+-2'");
	ad1 = smtr("00+-");
	nz = zone_drum_to_index(ad1);
	printf(" nz = %i\r\n", nz);

	printf(" ad1 ='00+0'");
	ad1 = smtr("00+0");
	nz = zone_drum_to_index(ad1);
	printf(" nz = %i\r\n", nz);

	printf("t23 test zone for view_brum()\r\n");

	printf(" z ='0+--'");
	ad1 = smtr("0+--");
	view_drum_zone(ad1);

	printf(" z ='++++'");
	ad1 = smtr("++++");
	view_drum_zone(ad1);

	printf("\r\nt24 test DRUN fill index and view \r\n");

	inr.l = 9;
	inr = smtr("000000000");

	trs_t zi;
	zi.l = 4;
	zi = smtr("000+"); //-17

	for (uint8_t zz = 0; zz < NUMBER_ZONE_DRUM; zz++)
	{
		for (uint8_t mm = 0; mm < SIZE_ZONE_TRIT_FRAM; mm++)
		{
			st_drum(zi, mm, inr);
			inc_trs(&inr);
		}
		inc_trs(&zi);
	}

	printf("\r\nt25 test oper='-0-' (Мд*)=>(Фа*) \r\n");

	addr = smtr("0000+");
	m1 = smtr("00+---0-0"); //-0-
	st_fram(addr, m1);

	S = smtr("000000000-00+000-+");
	view_short_reg(&S, "S=");

	/* Begin address fram */
	C = smtr("0000+");

	printf("\r\nreg C = 00001\r\n");

	/**
	 * work VM Setun-1958
	 */
	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);

	C = smtr("0000+");
	addr = smtr("0000+");
	m1 = smtr("000+--0+0"); //-0+
	st_fram(addr, m1);

	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);

	printf("BRUM zone='0+--'");
	ad1 = smtr("0+--");
	view_drum_zone(ad1);
	printf("\r\n");
	//
	printf("BRUM zone='0+--'");
	ad1 = smtr("0+--");
	view_drum_zone(ad1);
	printf("\r\n");

	printf("\r\n --- END TEST #9 --- \r\n");
}

// Test10: Вывод "бумажной ленты.
void Test10_Perforatin_Paper_Line(void)
{
	int8_t trit;
	trs_t aa;
	trs_t ad1;
	trs_t ad2;
	trs_t addr;
	trs_t oper;
	trs_t exK;
	trs_t m0;
	trs_t m1;
	int8_t ret_exec;

	printf("\r\n --- TEST #10 Perforatin paper line --- \r\n");

	// t11.1 test Oper=k6..8[+00]: (A*)=>(S)
	printf("\r\nt10.1:  Oper=k6..8[+00]: (A*)=>(S)\r\n");
	//
	reset_setun_1958();

	/* Инициализация таблиц символов ввода и вывода "Сетунь-1958" */
	init_tab4();

	/* ---------------------------------------
	 *  Открыть файлы для виртуальных устройств
	 */
	ptr1 = fopen("ptr1/paper.txt", "r");
	if (ptr1 == NULL)
	{
		printf("Error fopen 'ptr1/paper.txt'\r\n");
		return;
	}

	ptr2 = fopen("ptr2/paper.txt", "r");
	if (ptr2 == NULL)
	{
		printf("Error fopen 'ptr1/paper.txt'\r\n");
		return;
	}

	ptp1 = fopen("ptp1/paper.txt", "w");
	if (ptp1 == NULL)
	{
		printf("Error fopen 'ptp1/paper.txt'\r\n");
		return;
	}

	tty1 = fopen("tty1/printout.txt", "w");
	if (tty1 == NULL)
	{
		printf("Error fopen 'tty1/printout.txt'\r\n");
		return;
	}

	/* ----------------------------------
	 *  Выполнить первый код "Сетунь-1958"
	 */
	printf("\r\n[ Start Setun-1958 ]\r\n");

	/**
	 * Выполение программы в ферритовой памяти "Сетунь-1958"
	 */
	Begin_Read_Commands_from_FT1(ptr1);

	//
	addr = smtr("0000+");
	m0 = smtr("00+00-000");
	st_fram(addr, m0);
	view_elem_fram(addr);
	//

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	// work VM Setun-1958
	K = ld_fram(C);

	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	//
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	//
	view_short_regs();

	trs_t fa;
	fa = smtr("0---0");

	dump_fram_zone(smtr("0"));

	printf("\r\n --- END TEST #11 --- \r\n");
}

int usage(const char *argv0)
{
	printf("usage: %s [options] FILE SCRIPT(s)...\r\n", argv0);
	printf("\t--test : number test setun1958emu\r\n");
	printf("\t--debug : view step  setun1958emu\r\n");
	printf("\t--breakpoint : view stop setun1958emu\r\n");
	printf("\t--load : load software setun1958emu\r\n");
	printf("\t--convert : convert software file.lst to paper.txt setun1958emu\r\n");
	printf("\t--dump : dump zone from file.txs setun1958emu\r\n");
	exit(0);
}

/** -------------------------------
 *  Main
 *  -------------------------------
 */
int main(int argc, char *argv[])
{
	int test = 0;
	int convert = 0;
	int dump = 0;
	char *output = "-";
	int ret = 0;
	DEBUG = 0;

	/* Инициализация таблиц символов ввода и вывода "Сетунь-1958" */
	init_tab4();

        /* Настройки терминала */
        set_conio_terminal_mode();

	while (1)
	{
		int option_index;
		struct option long_options[] = {
			{"help", 0, 0, 0},
			{"load", 0, 0, 0},
			{"convert", 2, 0, 0},
			{"dump", 1, 0, 0},
			{"test", 1, 0, 0},
			{"debug", 0, 0, 0},
			{"breakpoint", 1, 0, 0},
			{"step", 1, 0, 0},
			{0},
		};
		int c = getopt_long(argc, argv, "o:", long_options, &option_index);
		if (c == -1)
			break;
		switch (c)
		{
		case 0:
		{
			const char *name = long_options[option_index].name;
			if (strcmp(name, "help") == 0)
			{
				usage(argv[0]);
			}
			if (strcmp(name, "test") == 0)
			{
				test = atoi(optarg);
			}
			if (strcmp(name, "debug") == 0)
			{
				DEBUG = 1;
			}
			if (strcmp(name, "breakpoint") == 0)
			{
				BREAKPOINT = atoi(optarg);
			}
			if (strcmp(name, "load") == 0)
			{
				LoadSWSetun();
			}
			if (strcmp(name, "convert") == 0)
			{
				/* Проверить парметры в команде */
				if (argv[2] == NULL ||
					argv[3] == NULL)
				{
					convert = 1; /* Error! */
				}
				else
				{
					/* Проверить путь к каталогам,  файл списка lst */
					if (ConvertSWtoPaper(argv[2], argv[3]) == 0)
					{
						convert = 2; /* Done */
					}
					else
					{
						convert = 1; /* Error! */
					}
				}
			}
			if (strcmp(name, "dump") == 0)
			{
				/* Проверить парметры в команде */
				if (argv[2] == NULL)
				{
					dump = 1; /* Error! */
				}
				else
				{
					/* Проверить путь к каталогам,  файл списка lst */
					if (DumpFileTxs(argv[2]) == 0)
					{
						dump = 2; /* Done */
					}
				}
			}
			if (strcmp(name, "step") == 0)
			{
				STEP = atoi(optarg);
			}
		}
		break;
		case 'o':
			output = strdup(optarg);
			break;
		default:
			break;
		}
	}
	if (convert > 0)
	{
		if (convert == 2)
		{
			/* Exit 0. Ok' */
			return 0;
		}
		else
		{
			usage(argv[0]);
		}
	}
	if (dump > 0)
	{
		if (dump == 2)
		{
			/* Exit 0. Ok' */
			return 0;
		}
		else
		{
			usage(argv[0]);
		}
	}
	if (test > 0)
	{
		/* Run number test */
		switch (test)
		{
		case 0:
			break;
		case 1:
			Test1_Ariphmetic_Ternary();
			break;
		case 2:
			Test2_Opers_TRITS_32();
			break;
		case 3:
			Test3_Setun_Opers();
			break;
		case 4:
			Test4_Setun_Opers();
			break;
		case 5:
			Test5_Setun_Load();
			break;
		case 6:
			Test6_Setun_Load();
			break;
		case 7:
			Test7_Setun_Load();
			break;
		case 8:
			// no used
			break;
		case 9:
			Test9_Setun_Electrified_Typewriter();
			break;
		case 10:
			Test10_Perforatin_Paper_Line();
			break;
		default:
			printf("Error: Not Test=%d\r\n", test);
			break;
		}
		/* Exit 0. Ok' */
		return 0;
	}

	/* ------------------------
	 * START emulator Setun-1958
	 */

	uint8_t cmd[20] = {0};
	uint8_t ret_exec = 0;
	uint32_t counter_step = 0;

	trs_t addr;
	trs_t C_cur;
	trs_t oper;

	/* Сброс виртуальной машины "Сетунь-1958" */
	reset_setun_1958();

	/* В режиме отладки вывод регистров */
	if (DEBUG > 0)
	{
		view_short_regs();
	}

	/* ---------------------------------------
	 *  Открыть файлы для виртуальных устройств
	 */
	ptr1 = fopen("ptr1/paper.txt", "r");
	if (ptr1 == NULL)
	{
		printf("Error fopen 'ptr1/paper.txt'\r\n");
		return 0;
	}

	ptr2 = fopen("ptr2/paper.txt", "r");
	if (ptr2 == NULL)
	{
		printf("Error fopen 'ptr1/paper.txt'\r\n");
		return 0;
	}

	ptp1 = fopen("ptp1/paper.txt", "w");
	if (ptp1 == NULL)
	{
		printf("Error fopen 'ptp1/paper.txt'\r\n");
		return 0;
	}

	tty1 = fopen("tty1/printout.txt", "w");
	if (tty1 == NULL)
	{
		printf("Error fopen 'tty1/printout.txt'\r\n");
		return 0;
	}

	/* ----------------------------------
	 *  Выполнить первый код "Сетунь-1958"
	 */
	printf("\r\n[ Start Setun-1958 ]\r\n");

	/**
	 * Выполение программы в ферритовой памяти "Сетунь-1958"
	 */
	Begin_Read_Commands_from_FT1(ptr1);

	/* Begin address fram */
	C = smtr("0000+");

	/**
	 * work VM Setun-1958
	 */
	while (1)
	{
		C_cur = C;
		K = ld_fram(C);
		K = slice_trs_setun(K, 1, 9);

		if (DEBUG > 0)
		{
			view_step_short_reg(&C, "\n С");
		}

		addr = control_trs(K);
		oper = slice_trs_setun(K, 6, 8);

		ret_exec = execute_trs(addr, oper);

		if ((ret_exec == STOP))
		{
			printf("\r\n<STOP>\r\n");
			break;
		}
		else if (ret_exec == STOP_OVER)
		{
			printf("\r\n<STOP_OVER>\r\n");
			break;
		}
		else if (ret_exec == STOP_ERROR)
		{
			printf("\r\nERR#:%i<STOP_ERROR>\r\n", ret_exec);
			break;
		}
		else if (ret_exec == STOP_ERROR_MB_NUMBER)
		{
			printf("\r\nERR#:%i<STOP_ERROR_MB_NUMBER>\r\n", ret_exec);
			break;
		}

		/*
		 *
		 */
		counter_step++;

		if (STEP == counter_step)
		{
			break; // STEP break
		}

		if (BREAKPOINT == trs2digit(C_cur) && (BREAKPOINT != INT32_MAX))
		{

			break; // BREAKPOINT break
		}

                /* Обработчик событий нажатия клавиш */
                if( kbhit() ) {
                    char chkey = getch(); /* consume the character */
                    if(chkey == 'Q' || chkey == 'q' ) {
                      printf("\r\n[ QUIT Work ]\r\n");
                      break; /* EVENT KEY break */
                    }
                }
        } /* while(...) */

		/* Prints REGS */
		view_short_regs();
		printf("\r\n");

		/* Prints REGS and FRAM */
		if (DEBUG > 0)
		{
			// dump_fram_zone(smtr("-"));
			// dump_fram_zone(smtr("0"));
			// dump_fram_zone(smtr("+"));
		}

		/* Печать завершения работы "Setun-1958" */
		if (STEP == 0)
		{
			printf("\r\n[ Stop Setun-1958 ]\r\n");
		}
		else
		{
			printf("\r\n[ Step = %d : Break work Setun-1958 ]\r\n", STEP);
		}

		/* Закрыть файлы виртуальных устройств */
		fclose(ptr1);
		fclose(ptr2);
		fclose(ptp1);
		fclose(tty1);

	} /* 'main.c' */

	/* EOF 'setun_core.c' */
