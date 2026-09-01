/**
 * Filename: "setun1958_vm.c"
 *
 * Project: Виртуальная машина МЦВМ "Сетунь" 1958 года на языке Си
 *
 * Create date: 01.11.2018
 * Edit date:   31.08.2026
 * Version: 	2.17
 */
#define Version "2.17"

/**
 *  Заголовочные файла
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>

#include "command.h"
#include "setun1958_vm.h"

#define LOGGING_print(...)                \
	do                                    \
	{                                     \
		if (LOGGING)                      \
			fprintf(stdout, __VA_ARGS__); \
	} while (0);

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

/* Максимальная количество тритов троичного числа */
#define SIZE_TRITS_MAX (32)
/* максимальная количество тритов в длинном троичном числе */
#define SIZE_LONG_TRITS_MAX (64)

/** -------------------------------------------
 *  Реализация виртуальной машины "Сетунь-1958"
 *  -------------------------------------------
 */
static void cli_prompt(void);

/** -----------------------------------------
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
/* Короткое слово 9-трит  */
#define SIZE_WORD_SHORT (9)
/*Длинное слово  18-трит */
#define SIZE_WORD_LONG (18)
/* Двойное длинное слово  36-трит */
#define SIZE_WORD_LONG_LONG (36)

/**
 * Описание ферритовой памяти FRAM
 */
/* Количество зон ферритовой памяти */
#define NUMBER_ZONE_FRAM (3)
/* Количнество коротких 9-тритных слов в зоне */
#define SIZE_ZONE_TRIT_FRAM (54)
/* Всего количество коротких 9-тритных слов */
#define SIZE_ALL_TRIT_FRAM (162)

/* Количнество груп (0...2) в FRAM */
#define SIZE_GRFRAM (2)
/* Количнество коротких 9-тритных слов в зоне */
#define SIZE_GR_TRIT_FRAM (81)

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
/* Количество хранения коротких слов из 9-тритов */
#define SIZE_TRIT_DRUM (1944)
/* Количество 9-тритных слов в зоне */
#define SIZE_ZONE_TRIT_DRUM (54)
/* Количество зон на магнитном барабане */
#define NUMBER_ZONE_DRUM (36)
#define ZONE_DRUM_BEG (5)	  /* троичное значение */
#define ZONE_DRUM_END (40)	  /* троичное значение */
#define ZONE_DRUM_OFFSET (-5) /* offset for index */

/**
 * Типы данных для виртуальной троичной машины "Сетунь-1958"
 */

typedef struct trs
{
	uint8_t l;	 /* длина троичного числа в тритах */
	uint32_t t1; /* троичное число FALSE,TRUE */
	uint32_t t0; /* троичное число NIL */
} trs_t;

typedef struct long_trs
{
	uint8_t l;	 /* длина троичного числа в тритах */
	uint64_t t1; /* троичное число FALSE,TRUE */
	uint64_t t0; /* троичное число NIL */
} long_trs_t;

/**
 * Вывод отладочной информации памяти машины "Сетунь-1958"
 */
uint8_t LOGGING = 0;   /* флаг логирование выполнение операций */
uint8_t STEP_FLAG = 0; /* флаг выполнить количество шагов */
uint32_t STEP = 0;	   /* счетчик количества операций */
uint32_t counter_step = 0;
int32_t BREAKPOINT = 0;	   /* режима останова по значению программного счетчика */
trs_t BREAKPOINT_TRS;	   /* режима останова по значению программного счетчика */
int32_t BREAKPOINT_MB = 0; /* режима останова по значению адреса магнитного барабана */
trs_t BREAKPOINT_MB_TRS;   /* режима останова по значению адреса магнитного барабана */

/**
 * Статус выполнения операции  "Сетунь-1958"
 */
enum
{
	OK = 0,				 /* Успешное выполнение операции */
	WORK,				 /* Выполнение операций виртуальной машины */
	STOP,				 /* Успешный останов машины */
	STOP_OVER,			 /* Останов по переполнению результата операции машины */
	STOP_ERROR,			 /* Аварийный останов машины */
	STOP_ERROR_MB_NUMBER /* Номер зоны DRUM не поддерживается */
} status_operation_e;

/**
 * Состояние работы эмулятора
 */
typedef enum
{
	NOREADY_EMU_ST = 0, /* Не готов после сброса питания */
	BEGIN_EMU_ST,		/* Готов к работе */
	LOOP_WORK_EMU_ST,	/* Непрерывная работа */
	STEP_EMU_ST,		/* Пошаговая выполнение операций */
	PAUSE_EMU_ST,		/* Пауза */
	STOP_EMU_ST,		/* Стоп */
	ERROR_EMU_ST,		/* Ошибка */
	ERROR_MB_NUMBER_EMU_ST,
	CLI_WELCOM_EMU_ST,
	WAIT_EMU_ST
} status_control_e;

static status_control_e emu_stat = NOREADY_EMU_ST;

/**
 * Виртуальные устройства Ввода и Вывода "Сетунь-1958"
 */
/* Фотосчитыватель ФТ1 */
FILE *ptr1;
/* Фотосчитыватель ФТ2 */
FILE *ptr2;
/* Перфоратор ПЛ */
FILE *ptp1;
/* Печать Телетайп ТП, Пишущая машинка ПМ */
FILE *tty1;
/* Состояние DRUM */
FILE *drum;

/**
 * Определение памяти машины "Сетунь-1958"
 */
trs_t mem_fram[SIZE_GR_TRIT_FRAM][SIZE_GRFRAM];		   /* оперативное запоминающее устройство на ферритовых сердечниках */
trs_t mem_drum[NUMBER_ZONE_DRUM][SIZE_ZONE_TRIT_DRUM]; /* запоминающее устройство на магнитном барабане */

/** -----------------------------------
 *  Определение регистров "Сетунь-1958"
 *  -----------------------------------
 */
/* Основные регистры в порядке пульта управления */
trs_t K = {9, 0, 0}; /* K(1:9)  код команды (адрес ячейки оперативной памяти) */
trs_t F = {5, 0, 0}; /* F(1:5)  индекс регистр  */
trs_t C = {5, 0, 0}; /* C(1:5)  программный счетчик  */
trs_t W = {1, 0, 0}; /* W(1:1)  знак троичного числа */

trs_t ph1 = {1, 0, 0}; /* ph1(1:1) 1 разряд переполнения */
trs_t ph2 = {1, 0, 0}; /* ph2(1:1) 1 разряд переполнения */
trs_t S = {18, 0, 0};  /* S(1:18) аккумулятор */
trs_t R = {18, 0, 0};  /* R(1:18) регистр множителя */
trs_t MB = {4, 0, 0};  /* MB(1:4) троичное число зоны магнитного барабана */

/* Дополнительные */
trs_t MR = {9, 0, 0};		 /* временный регистр для обмена с FRAM */
long_trs_t TMP = {36, 0, 0}; /* временная переменная для троичного числа */

trs_t BRPNT = {4, 0, 0}; /* BRPNT(1:5) - точка остановки по значению программного счетчика */

trs_t C_new = {5, 0, 0}; /* C_new(1:5) новое значение программного счетчика в скрипт-файле после остовона машины */
char action_post = 'S';	 /*
						  *  action действие после останова
						  *  action = 'S' - оставаться в состоянии <STOP>
						  *  action = 'R' - продолжить работу машины с нового адреса C_new
						  */

/** ------------------------------------------------------
 *  Прототипы функций для виртуальной машины "Сетунь-1958"
 *  ------------------------------------------------------
 */
int32_t pow3(int8_t x);
double t3_to_d10(trs_t t);
trs_t d10_to_t3(double f);
int8_t trit2int(trs_t t);
trs_t bit2trit(int8_t b);

/**  ---------------------------------------
 *   Операции с тритами
 *   trit = {-1,0,1}  - трит значение трита
 */
void and_t(int8_t *a, int8_t *b, int8_t *s);
void xor_t(int8_t *a, int8_t *b, int8_t *s);
void or_t(int8_t *a, int8_t *b, int8_t *s);
void not_t(int8_t *a, int8_t *s);
void sum_t(int8_t *a, int8_t *b, int8_t *p0, int8_t *s, int8_t *p1);

/** -------------------------------------------------------------
 *  Троичные числа
 *
 *  TRITS-1  = [t0]       - обозначение позиции тритов в числе
 *  TRITS-32 = [t31...t0] - обозначение позиции тритов в числе
 */
int8_t get_trit(trs_t t, uint8_t pos);
trs_t set_trit(trs_t t, uint8_t pos, int8_t trit);
trs_t slice_trs(trs_t t, int8_t p1, int8_t p2);
void copy_trs(trs_t *src, trs_t *dst);

/** -------------------------------------------------------------
 *  Троичные числа
 *  TRITS-64 = [t63...t0] - обозначение позиции тритов в числе
 */
void clear_long(long_trs_t *t);
void clear_full_long(long_trs_t *t);
int8_t get_long_trit(long_trs_t t, uint8_t pos);
long_trs_t set_long_trit(long_trs_t t, uint8_t pos, int8_t trit);
int8_t sgn_long_trs(long_trs_t x);
long_trs_t shift_long_trs(long_trs_t t, int8_t s);
void copy_long_trs(long_trs_t *src, long_trs_t *dst);

/** -------------------------------------------------------------
 *  Троичные числа регистров Setun-1958
 *
 *  SETUN-T1  = [s1]      - обозначение позиции тритов в регистре
 *  SETUN-T5  = [s1...s5]
 *  SETUN-T9  = [s1...s9]
 *  SETUN-T18 = [s1...s18]
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

/* Проверка переполнения */
int8_t over_check(void);
trs_t and_trs(trs_t a, trs_t b);
trs_t or_trs(trs_t a, trs_t b);
trs_t xor_trs(trs_t a, trs_t b);
trs_t add_trs(trs_t a, trs_t b);
trs_t sub_trs(trs_t a, trs_t b);
trs_t mul_trs(trs_t a, trs_t b);
trs_t tmul_trs(trs_t a, trs_t b);
trs_t div_trs(trs_t a, trs_t b);
trs_t shift_trs(trs_t t, int8_t s);
int cmp_trs(trs_t a, trs_t b);

/* Long trits */
long_trs_t add_long_trs(long_trs_t a, long_trs_t b);

/* Преобразование тритов в другие типы данных */
int32_t trs2digit(trs_t t);
uint8_t trit2lt(int8_t v);
int8_t symtrs2numb(char c);
int8_t str_symtrs2numb(char *s);
trs_t smtr(char *s);
uint8_t valid_smtr(char *s);
uint8_t numb2symtrs(int8_t v);

/* Операции с ферритовой памятью машины FRAM */
void clean_fram_zone(trs_t z);
void clean_fram(void);
trs_t ld_fram(trs_t ea);
void st_fram(trs_t ea, trs_t v);

/** -------------------------------------
 *  Операции ввода и вывода "Сетунь-1958"
 */

/**
 *  Регист переключения Русский/Латинский
 *  RUS == 0
 *  LAT == 1
 */
uint8_t russian_latin_sw = 0;
/* Регист переключения Буквенный/Цифровой */
uint8_t letter_number_sw = 0;
/* Регист переключения цвета печатающей ленты */
uint8_t color_sw = 0;

uint8_t pl_to_ind(char *perline);
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

/* Читать / Записать зоны магнитного барабана в файл paper.txt */
int Read_Backup_DRUM(char *drum_path);
int Write_Backup_DRUM(char *drum_path);

/* Операции копирования */
void fram_to_drum(trs_t ea);
void drum_to_fram(trs_t ea);

/* Функции троичной машины Сетунь-1958 */
void reset_setun_1958(void);					/* Сброс машины */
trs_t control_trs(trs_t a);						/* Устройство управления */
trs_t next_address(trs_t c);					/* Определить следующий адрес */
int8_t execute_trs_lut(trs_t addr, trs_t oper); /* Выполнение кодов операций */

/* Функции вывода отладочной информации */
void view_short_reg(trs_t *t, char *ch);
void view_short_regs(void);

/**
 * Выполнить скрипт-файл './script/scrint.sst'.
 * Вывод сообщения и продолжить с заданного адреса в
 * ферритовой памяти FRAM.
 */
void view_next_start(void);

int Process_Work_Emulation(void);
void Emu_Stop(void);
int Emu_Step(void);
void Emu_Begin(void);
void Emu_Close_Files(void);
void Emu_Open_Files(void);
void Emu_Close_Files_ptr1_ptr2(void);
void Emu_Open_Files_ptr1_ptr2(void);

int usage(const char *argv0);
int version(const char *argv0);
void print_version(void);
int parser_user(char *cmd, char fields[][80], char ch);
void LoadFileListToPaperTxt(char *pathcataloglst, char *pathfilelst, char *pathfiletxt);
void CopyFileScript(char *path_lst __attribute__((unused)), char *pathfile1, char *pathfile2);
int copy_file(char *file_scr_sst, char *file_dst_sst);
const char *get_file_ext(const char *filename);
uint8_t Begin_Read_Commands_from_FT1(FILE *file);
void electrified_typewriter(trs_t t, uint8_t local);
int16_t Decoder_String_from_Paper_Line(void);
void dump_drum(void);
void view_drum_zone(trs_t zone);
void dump_fram_zone(trs_t z);
void dump_fram(void);
void view_fram(trs_t addr1, trs_t addr2);
void view_elem_fram(trs_t ea);
void view_checksum_setun(trs_t t);
void view_short_long_reg(long_trs_t *t, uint8_t *ch);
void view_step_new_addres(trs_t *t, char *ch);
void view_step_short_reg(trs_t *t, char *ch);
void view_short_reg_fixpoint(trs_t *t, char *ch);
void view_short_reg_long(long_trs_t *t, uint8_t *ch);
void trit2symtrs(trs_t t);



/** ---------------------------------------------------
 *  Реализации функций виртуальной машины "Сетунь-1958"
 *  ---------------------------------------------------
 */

/**
 * Возведение в степень по модулю 3
 */
static const int32_t POW3_TABLE[20] = {
	1, 3, 9, 27, 81, 243, 729, 2187, 6561, 19683,
	59049, 177147, 531441, 1594323, 4782969, 14348907,
	43046721, 129140163, 387420489, 1162261467};

int32_t pow3(int8_t x)
{
	if (x < 0 || x >= 20)
		return 0;		  // защита
	return POW3_TABLE[x]; // 1 обращение к L1
}

static inline int32_t pow3_inline(int8_t x)
{
	return POW3_TABLE[x]; // для горячих циклов
}

/**
 * Преобразование 3 -> 10 (использует pow3)
 */
double ERROR_t3_to_d10(trs_t t)
{
    if (t.l == 0 || (t.t1 == 0 && t.t0 == 0))
        return 0.0;

    double val = 0.0;
    uint32_t mask = 1;
    uint8_t max_pos = (t.l < 32) ? t.l : 32;

    for (uint8_t i = 0; i < max_pos; ++i, mask <<= 1)
    {
        int8_t trit = 0;
        if (t.t0 & mask)
        {
            trit = (t.t1 & mask) ? 1 : -1;
        }
        if (trit != 0)
            val += trit * (double)pow3(i);
    }
    return val;
}

/**
 * Преобразование 3 -> 10
 */
double t3_to_d10(trs_t t)
{
	int i;
	double lf = 0.0;
	double lfp = 0.0;

	lf += 3 * get_trit_setun(t, 1);
	lf += 1 * get_trit_setun(t, 2);

	for (i = 3; i <= 18; i++)
	{
		if (get_trit_setun(t, i) != 0)
		{
			lfp += 1.0 / (double)(pow3(i - 2) * get_trit_setun(t, i));
		}
	}

	return (lf + lfp);
} 

/**
 * Преобразование 10 -> 3 (для 18 трит)
 * Диапазон: от -193710244 до 193710243
 */
trs_t d10_to_t3(double f)
{
    trs_t r;
    int8_t pos = 0;
    int64_t n;
    int64_t rem;
    uint32_t mask;

    /* Быстрая инициализация */
    r.l = 0;
    r.t1 = 0;
    r.t0 = 0;

    /* Округление double до ближайшего целого */
    if (f >= 0.0)
        n = (int64_t)(f + 0.5);
    else
        n = (int64_t)(f - 0.5);

    /* Проверка диапазона для 18 трит */
    if (n > 193710243LL) {
        n = 193710243LL;  /* Максимальное значение для 18 трит */
    } else if (n < -193710244LL) {
        n = -193710244LL; /* Минимальное значение для 18 трит */
    }

    if (n == 0) {
        r.l = 1;
        return r;
    }

    /* Преобразование в троичную систему */
    while (n != 0 && pos < 18)  /* Ограничение 18 трит */
    {
        rem = n % 3;
        if (rem < 0)
            rem += 3;

        mask = (1u << pos);

        if (rem == 0) {
            /* Биты уже нулевые, ничего не делаем */
        } else if (rem == 1) {
            r.t1 |= mask;
            r.t0 |= mask;
            n -= 1;
        } else { /* rem == 2 */
            r.t1 &= ~mask;
            r.t0 |= mask;
            n += 1;
        }

        n /= 3;
        pos++;
    }

    r.l = pos;
    return r;
}

/**
 * Очистить поле битов троичного числа
 */
inline void clear(trs_t *t)
{
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Операция Mod 3^N
 */
inline void mod_3_n(trs_t *t, uint8_t n)
{
    /* Используем предрасчитанную маску для ускорения */
    static const uint32_t MASKS[] = {
        0x00000000, 0x00000001, 0x00000003, 0x00000007,
        0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
        0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
        0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
        0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
        0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
        0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
        0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
        0xFFFFFFFF
    };
    
    t->t1 &= MASKS[n & 0x1F];  /* n % 32 */
    t->t0 &= MASKS[n & 0x1F];
}

/**
 * Очистить длину троичного числа и поле битов троичного числа
 */
inline void clear_full(trs_t *t)
{
	t->l = 0;
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Очистить поле битов троичного числа
 */
inline void clear_long(long_trs_t *t)
{
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Очистить длину троичного числа и поле битов троичного числа
 */
inline void clear_full_long(long_trs_t *t)
{
	t->l = 0;
	t->t1 = 0;
	t->t0 = 0;
}

/**
 * Преобразование младшего трита в int8 (с таблицей)
 */
inline int8_t trit2int(trs_t t)
{
    static const int8_t TABLE[4] = {0, -1, 0, 1};
    return TABLE[(t.t0 & 1) | ((t.t1 & 1) << 1)];
}

/**  ---------------------------------------
 *   Операции с тритами
 *   trit = {-1,0,1}  - трит значение трита
 */

/* Троичное сложение двух тритов с переносом */
static const int8_t SUM_T_S[3][3][3] = {
	{{0, 1, -1}, {1, -1, 0}, {-1, 0, 1}}, // a = -1
	{{1, -1, 0}, {-1, 0, 1}, {0, 1, -1}}, // a =  0
	{{-1, 0, 1}, {0, 1, -1}, {1, -1, 0}}, // a = +1
};

static const int8_t SUM_T_P1[3][3][3] = {
	{{-1, -1, 0}, {-1, 0, 0}, {0, 0, 0}}, // a = -1
	{{-1, 0, 0}, {0, 0, 0}, {0, 0, 1}},	  // a =  0
	{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}},	  // a = +1
};

void sum_t(int8_t *a, int8_t *b, int8_t *p0, int8_t *s, int8_t *p1)
{
	const int ia = (int)(*a) + 1; // {-1,0,1}
	const int ib = (int)(*b) + 1;
	const int ip = (int)(*p0) + 1;

	*s = SUM_T_S[ia][ib][ip];
	*p1 = SUM_T_P1[ia][ib][ip];
}

/**
 * Троичное умножение тритов (AND) с таблицей
 */
inline void and_t(int8_t *a, int8_t *b, int8_t *s)
{
    static const int8_t TABLE[3][3] = {
        {1, 0, -1},   /* a = -1 */
        {0, 0, 0},    /* a =  0 */
        {-1, 0, 1}    /* a = +1 */
    };
    *s = TABLE[(*a) + 1][(*b) + 1];
}

/**
 * Троичное отрицание трита (NOT) с таблицей
 */
inline void not_t(int8_t *a, int8_t *s)
{
    static const int8_t TABLE[3] = {1, 0, -1};  /* -1 → 1, 0 → 0, 1 → -1 */
    *s = TABLE[(*a) + 1];
}

/**
 * Троичное XOR тритов с таблицей
 */
inline void xor_t(int8_t *a, int8_t *b, int8_t *s)
{
    static const int8_t TABLE[3][3] = {
        {-1, 0, 1},   /* a = -1 */
        {0, 0, 0},    /* a =  0 */
        {1, 0, -1}    /* a = +1 */
    };
    *s = TABLE[(*a) + 1][(*b) + 1];
}

/**
 * Троичное OR тритов с таблицей
 */
inline void or_t(int8_t *a, int8_t *b, int8_t *s)
{
    static const int8_t TABLE[3][3] = {
        {-1, 0, 1},   /* a = -1 */
        {-1, 0, 1},   /* a =  0 */
        {1, 1, 1}     /* a = +1 */
    };
    *s = TABLE[(*a) + 1][(*b) + 1];
}

/**  ----------------------------------------------------------
 *   Троичные числа
 *
 *   TRITS-64 = [t63...t0] - обозначение позиции тритов в числе
 *
 */

/**
 * Получить целое со знаком трита в позиции троичного числа
 */
inline int8_t get_long_trit(long_trs_t t, uint8_t pos)
{
    if (pos >= SIZE_LONG_TRITS_MAX) return 0;
    
    uint64_t mask = 1ULL << pos;
    uint64_t bit0 = (t.t0 & mask) ? 1 : 0;
    uint64_t bit1 = (t.t1 & mask) ? 1 : 0;
    
    return (int8_t)(bit0 * (2 * bit1 - 1));
}

/**
 * Установить трит в троичном числе
 */
inline long_trs_t set_long_trit(long_trs_t t, uint8_t pos, int8_t trit)
{
    if (pos >= SIZE_LONG_TRITS_MAX) return t;
    
    uint64_t mask = 1ULL << pos;
    
    if (trit > 0) {
        t.t1 |= mask;
        t.t0 |= mask;
    } else if (trit < 0) {
        t.t1 &= ~mask;
        t.t0 |= mask;
    } else {
        t.t1 &= ~mask;
        t.t0 &= ~mask;
    }
    return t;
}

/**
 * Операция знак SGN троичного числа
 */
inline int8_t sgn_long_trs(long_trs_t x)
{
    if (x.l == 0 || (x.t0 == 0 && x.t1 == 0)) return 0;
    
    uint8_t max_pos = (x.l < SIZE_LONG_TRITS_MAX) ? x.l : SIZE_LONG_TRITS_MAX;
    
    for (int8_t i = max_pos - 1; i >= 0; i--) {
        uint64_t mask = 1ULL << i;
        if (x.t0 & mask) {
            return (x.t1 & mask) ? 1 : -1;
        }
    }
    return 0;
}

/**
 * Операция сдвига тритов
 */
inline long_trs_t shift_long_trs(long_trs_t t, int8_t d)
{
    if (d > 0) {
        if (d >= 64) {
            t.t1 = 0;
            t.t0 = 0;
        } else {
            t.t1 <<= d;
            t.t0 <<= d;
        }
    } else if (d < 0) {
        int8_t shift = -d;
        if (shift >= 64) {
            t.t1 = 0;
            t.t0 = 0;
        } else {
            t.t1 >>= shift;
            t.t0 >>= shift;
        }
    }
    return t;
}

/**
 * Операция присваивания троичных чисел в регистры
 */
inline void copy_long_trs(long_trs_t *src, long_trs_t *dst)
{
    dst->t1 = src->t1;
    
    if (src->l == dst->l) {
        dst->t0 = src->t0;
    } else if (src->l > dst->l) {
        dst->t0 = src->t0 & (~((uint64_t)0 << dst->l));
    } else {
        dst->t0 = src->t0 & (~((uint64_t)0 >> src->l));
    }
}

/**  ----------------------------------------------------------
 *   Троичные числа
 *
 *   TRITS-32 = [t31...t0] - обозначение позиции тритов в числе
 *
 */

/**
 * Получить целое со знаком трита в позиции троичного числа
 */
inline int8_t get_trit(trs_t t, uint8_t pos)
{
    if (pos >= SIZE_TRITS_MAX) return 0;
    
    uint32_t mask = 1u << pos;
    uint32_t bit0 = (t.t0 & mask) ? 1 : 0;
    uint32_t bit1 = (t.t1 & mask) ? 1 : 0;
    
    return (int8_t)(bit0 * (2 * bit1 - 1));
}

/**
 * Установить трит в троичном числе
 */
inline trs_t set_trit(trs_t t, uint8_t pos, int8_t trit)
{
    if (pos >= SIZE_TRITS_MAX) return t;
    
    uint32_t mask = 1u << pos;
    
    if (trit > 0) {
        t.t1 |= mask;
        t.t0 |= mask;
    } else if (trit < 0) {
        t.t1 &= ~mask;
        t.t0 |= mask;
    } else {
        t.t1 &= ~mask;
        t.t0 &= ~mask;
    }
    return t;
}

/**
 * Операция знак SGN троичного числа
 */
int8_t sgn_trs(trs_t x)
{
    if (x.l == 0 || (x.t0 == 0 && x.t1 == 0)) return 0;
    
    uint8_t max_pos = (x.l < SIZE_TRITS_MAX) ? x.l : SIZE_TRITS_MAX;
    
    for (int8_t i = max_pos - 1; i >= 0; i--) {
        uint32_t mask = 1u << i;
        if (x.t0 & mask) {
            return (x.t1 & mask) ? 1 : -1;
        }
    }
    return 0;
}

/**
 * Операция OR trs
 */
trs_t or_trs(trs_t x, trs_t y)
{
    static const int8_t OR_TABLE[3][3] = {
        {-1, 0, 1},   /* a = -1 */
        {-1, 0, 1},   /* a =  0 */
        {1, 1, 1}     /* a = +1 */
    };
    
    uint8_t max_l = (x.l > y.l) ? x.l : y.l;
    if (max_l > SIZE_TRITS_MAX) max_l = SIZE_TRITS_MAX;
    
    trs_t r;
    r.l = max_l;
    r.t1 = 0;
    r.t0 = 0;
    
    for (uint8_t i = 0; i < max_l; i++) {
        uint32_t mask = 1u << i;
        int8_t a = (x.t0 & mask) ? ((x.t1 & mask) ? 1 : -1) : 0;
        int8_t b = (y.t0 & mask) ? ((y.t1 & mask) ? 1 : -1) : 0;
        int8_t s = OR_TABLE[a + 1][b + 1];
        
        if (s > 0) {
            r.t1 |= mask;
            r.t0 |= mask;
        } else if (s < 0) {
            r.t0 |= mask;
        }
    }
    return r;
}

/**
 * Операция AND trs
 */
trs_t and_trs(trs_t x, trs_t y)
{
    static const int8_t AND_TABLE[3][3] = {
        {1, 0, -1},   /* a = -1 */
        {0, 0, 0},    /* a =  0 */
        {-1, 0, 1}    /* a = +1 */
    };
    
    uint8_t max_l = (x.l > y.l) ? x.l : y.l;
    if (max_l > SIZE_TRITS_MAX) max_l = SIZE_TRITS_MAX;
    
    uint8_t min_l = (x.l < y.l) ? x.l : y.l;
    if (min_l > SIZE_TRITS_MAX) min_l = SIZE_TRITS_MAX;
    
    trs_t r;
    r.l = max_l;
    r.t1 = 0;
    r.t0 = 0;
    
    for (uint8_t i = 0; i < min_l; i++) {
        uint32_t mask = 1u << i;
        int8_t a = (x.t0 & mask) ? ((x.t1 & mask) ? 1 : -1) : 0;
        int8_t b = (y.t0 & mask) ? ((y.t1 & mask) ? 1 : -1) : 0;
        int8_t s = AND_TABLE[a + 1][b + 1];
        
        if (s > 0) {
            r.t1 |= mask;
            r.t0 |= mask;
        } else if (s < 0) {
            r.t0 |= mask;
        }
    }
    return r;
}

/**
 * Операция XOR trs
 */
trs_t xor_trs(trs_t x, trs_t y)
{
    static const int8_t XOR_TABLE[3][3] = {
        {-1, 0, 1},   /* a = -1 */
        {0, 0, 0},    /* a =  0 */
        {1, 0, -1}    /* a = +1 */
    };
    
    uint8_t max_l = (x.l > y.l) ? x.l : y.l;
    if (max_l > SIZE_TRITS_MAX) max_l = SIZE_TRITS_MAX;
    
    uint8_t min_l = (x.l < y.l) ? x.l : y.l;
    if (min_l > SIZE_TRITS_MAX) min_l = SIZE_TRITS_MAX;
    
    trs_t r;
    r.l = max_l;
    r.t1 = 0;
    r.t0 = 0;
    
    for (uint8_t i = 0; i < min_l; i++) {
        uint32_t mask = 1u << i;
        int8_t a = (x.t0 & mask) ? ((x.t1 & mask) ? 1 : -1) : 0;
        int8_t b = (y.t0 & mask) ? ((y.t1 & mask) ? 1 : -1) : 0;
        int8_t s = XOR_TABLE[a + 1][b + 1];
        
        if (s > 0) {
            r.t1 |= mask;
            r.t0 |= mask;
        } else if (s < 0) {
            r.t0 |= mask;
        }
    }
    return r;
}

/**
 * Операция Setun XOR trs
 */
trs_t xor_setun_trs(trs_t x, trs_t y)
{
    uint8_t max_l = (x.l > y.l) ? x.l : y.l;
    if (max_l > SIZE_TRITS_MAX) max_l = SIZE_TRITS_MAX;
    
    uint8_t min_l = (x.l < y.l) ? x.l : y.l;
    if (min_l > SIZE_TRITS_MAX) min_l = SIZE_TRITS_MAX;
    
    uint32_t mask_all = (max_l == 32) ? 0xFFFFFFFF : ((1u << max_l) - 1);
    x.t0 &= mask_all;
    x.t1 &= mask_all;
    y.t0 &= mask_all;
    y.t1 &= mask_all;
    
    trs_t r;
    r.l = max_l;
    r.t1 = 0;
    r.t0 = 0;
    
    for (uint8_t i = 0; i < min_l; i++) {
        uint32_t mask = 1u << i;
        uint32_t x_has = x.t0 & mask;
        uint32_t y_has = y.t0 & mask;
        
        if (x_has && y_has) {
            uint32_t x_pos = x.t1 & mask;
            uint32_t y_pos = y.t1 & mask;
            
            if ((x_pos && y_pos) || (!x_pos && !y_pos)) {
                /* Оба +1 или оба -1 -> -1 */
                r.t0 |= mask;
            } else {
                /* Разные знаки -> +1 */
                r.t1 |= mask;
                r.t0 |= mask;
            }
        }
    }
    return r;
}

/**
 * Операция NOT trs
 */
trs_t not_trs(trs_t x)
{
    uint8_t max_l = (x.l < SIZE_TRITS_MAX) ? x.l : SIZE_TRITS_MAX;
    
    trs_t r;
    r.l = max_l;
    
    if (max_l == 32) {
        r.t1 = ~x.t1;
        r.t0 = ~x.t0;
    } else {
        uint32_t mask = (1u << max_l) - 1;
        r.t1 = (~x.t1) & mask;
        r.t0 = (~x.t0) & mask;
    }
    
    return r;
}

/**
 * Троичное NEG отрицания тритов
 */
trs_t neg_trs(trs_t t)
{
    return not_trs(t);
}

/**
 * Троичный INC trs (инкремент на 1)
 */
void inc_trs(trs_t *t)
{
    trs_t x;
    x.l = 1;
    x.t1 = 1;
    x.t0 = 1;
    
    *t = add_trs(*t, x);
}

/**
 * Операция DEC trs (декремент на 1)
 */
void dec_trs(trs_t *t)
{
    trs_t x;
    x.l = 1;
    x.t1 = 0;
    x.t0 = 1;  /* -1 в троичной записи: t0=1, t1=0 */
    
    trs_t r = add_trs(*t, x);
    *t = r;
}

/**
 *  Троичное сложение тритов
 */
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
	if ((r.t0 & (1 << 19)) > 0)
	{
		if ((r.t1 & (1 << 19)) > 0)
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
	if ((r.t0 & (1 << 18)) > 0)
	{
		if ((r.t1 & (1 << 18)) > 0)
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

/**
 * Троичное сложение тритов для длинных слов
 */
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
 * Троичное вычитание тритов
 */
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
	if ((r.t0 & (1 << 19)) > 0)
	{
		if ((r.t1 & (1 << 19)) > 0)
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
	if ((r.t0 & (1 << 18)) > 0)
	{
		if ((r.t1 & (1 << 18)) > 0)
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

/**
 * Троичное сложение тритов для длинных слов
 */
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

/**
 * Троичное умножение тритов 
 */
trs_t mul_trs(trs_t a, trs_t b)
{
    long_trs_t aa = {a.l, a.t1, a.t0};
    long_trs_t rr = {SIZE_WORD_LONG_LONG, 0, 0};
    
    uint8_t bl = (b.l < SIZE_WORD_LONG_LONG) ? b.l : SIZE_WORD_LONG_LONG;
    
    for (uint8_t i = 0; i < bl; i++) {
        uint64_t mask = 1ULL << i;
        if (b.t0 & mask) {
            long_trs_t shifted = aa;
            shifted.t1 >>= i;
            shifted.t0 >>= i;
            rr = (b.t1 & mask) ? add_long_trs(rr, shifted) : sub_long_trs(rr, shifted);
        }
    }
    
    rr.t1 = (rr.t1 >> 18) & 0x3FFFF;
    rr.t0 = (rr.t0 >> 18) & 0x3FFFF;
    
    trs_t r = {SIZE_WORD_LONG, (uint32_t)rr.t1, (uint32_t)rr.t0};
    return r;
}

/**
 * Троичное деление с остатком
 * 
 * Возвращает структуру с частным и остатком
 */
typedef struct {
    trs_t quotient;   /* Частное */
    trs_t remainder;  /* Остаток */
} div_result_t;

div_result_t div_mod_trs(trs_t a, trs_t b)
{
    div_result_t result;
    
    /* Инициализация полей структуры */
    clear_full(&result.quotient);
    clear_full(&result.remainder);
    result.quotient.l = SIZE_WORD_LONG;
    result.remainder.l = SIZE_WORD_LONG;
    
    /* Проверка на деление на ноль */
    if ((b.t0 == 0 && b.t1 == 0) || b.l == 0) {
        printf("ERROR: Division by zero!\n");
        return result;
    }
    
    /* Проверка на деление нуля */
    if ((a.t0 == 0 && a.t1 == 0) || a.l == 0) {
        return result;
    }
    
    /* Определяем знаки операндов */
    int8_t sign_a = sgn_trs(a);
    int8_t sign_b = sgn_trs(b);
    int8_t sign_result = (sign_a == sign_b) ? 1 : -1;
    int8_t sign_rem = sign_a;  /* Остаток имеет знак делимого */
    
    /* Берем абсолютные значения */
    trs_t abs_a = a;
    trs_t abs_b = b;
    
    abs_a.l = SIZE_WORD_LONG;
    abs_b.l = SIZE_WORD_LONG;
    
    if (sign_a < 0) {
        abs_a = neg_trs(abs_a);
        mod_3_n(&abs_a, SIZE_WORD_LONG);
    }
    if (sign_b < 0) {
        abs_b = neg_trs(abs_b);
        mod_3_n(&abs_b, SIZE_WORD_LONG);
    }
    
    /* Проверка: делимое меньше делителя? */
    int cmp = cmp_trs(abs_a, abs_b);
    if (cmp < 0) {
        /* a < b, частное = 0, остаток = a */
        result.remainder = a;
        return result;
    }
    if (cmp == 0) {
        /* a == b, частное = 1, остаток = 0 */
        clear_full(&result.quotient);
        result.quotient.l = SIZE_WORD_LONG;
        result.quotient.t1 = 1;  /* +1 в троичной системе */
        result.quotient.t0 = 1;
        if (sign_result < 0) {
            result.quotient = neg_trs(result.quotient);
            mod_3_n(&result.quotient, SIZE_WORD_LONG);
        }
        return result;
    }
    
    /* Подготовка к делению */
    clear_full(&result.quotient);
    result.quotient.l = SIZE_WORD_LONG;
    
    trs_t remainder = abs_a;
    trs_t divisor = abs_b;
    
    /* Находим старший значащий разряд для оптимизации */
    int8_t shift = 0;
    trs_t temp = abs_b;
    while (cmp_trs(shift_trs(temp, 1), abs_a) <= 0 && shift < 16) {
        temp = shift_trs(temp, 1);
        shift++;
    }
    
    /* Основной цикл деления */
    for (int8_t i = shift; i >= 0; i--) {
        /* Сдвигаем делитель на i разрядов влево */
        trs_t shifted_divisor = shift_trs(divisor, i);
        mod_3_n(&shifted_divisor, SIZE_WORD_LONG);
        
        /* Пробуем цифру частного: -1, 0, 1 */
        int8_t digit = 0;
        
        /* Проверяем, можно ли вычесть сдвинутый делитель */
        trs_t test_remainder = sub_trs(remainder, shifted_divisor);
        mod_3_n(&test_remainder, SIZE_WORD_LONG);
        
        if (sgn_trs(test_remainder) >= 0) {
            /* Можно вычесть (цифра +1) */
            digit = 1;
            remainder = test_remainder;
        } else {
            /* Проверяем, можно ли прибавить сдвинутый делитель */
            test_remainder = add_trs(remainder, shifted_divisor);
            mod_3_n(&test_remainder, SIZE_WORD_LONG);
            
            if (sgn_trs(test_remainder) <= 0 && sgn_trs(remainder) < 0) {
                /* Можно прибавить (цифра -1) */
                digit = -1;
                remainder = test_remainder;
            }
        }
        
        /* Устанавливаем цифру в частное */
        if (digit != 0) {
            uint32_t mask = 1u << i;
            if (digit > 0) {
                result.quotient.t1 |= mask;
                result.quotient.t0 |= mask;
            } else {
                result.quotient.t1 &= ~mask;
                result.quotient.t0 |= mask;
            }
        }
    }
    
    /* Применяем знак к частному */
    if (sign_result < 0) {
        result.quotient = neg_trs(result.quotient);
        mod_3_n(&result.quotient, SIZE_WORD_LONG);
    }
    
    /* Применяем знак к остатку */
    if (sign_rem < 0) {
        remainder = neg_trs(remainder);
        mod_3_n(&remainder, SIZE_WORD_LONG);
    }
    
    /* Очищаем неиспользуемые разряды */
    result.quotient.t1 &= 0x3FFFF;  /* 18 бит */
    result.quotient.t0 &= 0x3FFFF;
    result.quotient.l = SIZE_WORD_LONG;
    
    remainder.t1 &= 0x3FFFF;
    remainder.t0 &= 0x3FFFF;
    remainder.l = SIZE_WORD_LONG;
    
    result.remainder = remainder;
    
    return result;
}

/**
 * Операция присваивания троичных чисел в регистры
 */
void copy_trs(trs_t *src, trs_t *dst)
{
    dst->t1 = src->t1;
    
    if (src->l == dst->l) {
        dst->t0 = src->t0;
    } else if (src->l > dst->l) {
        dst->t0 = src->t0 & ((1u << dst->l) - 1);
    } else {
        dst->t0 = src->t0 & ((1u << src->l) - 1);
    }
}

/**
 * Троичное сравнение троичных чисел
 */
int cmp_trs(trs_t a, trs_t b)
{
	trs_t r;
	r = sub_trs(a, b);

	if (r.l == 0)
		return 0; /* a == b */

	if (r.l > 0)
	{
		int i;
		int sing = 0;
		for (i = 1; i < r.l + 1; i++)
		{
			sing = get_trit_setun(r, i);
			if (sing > 0)
				return +1; /* a > b */
			if (sing < 0)
				return -1; /* a < b */
		}
	}
	return 0; /* a == b */
}

/** 
 * Получить часть тритов из троичного числа
 */
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

/**
 * Получить целое со знаком трита в позиции троичного регистра
 * SETUN-T1  = [s1]      - обозначение позиции тритов в регистре
 * SETUN-T5  = [s1...s5]
 * SETUN-T9  = [s1...s9]
 * SETUN-T18 = [s1...s18]
 */
int8_t get_trit_setun(trs_t t, uint8_t pos)
{
    if (pos == 0 || pos > t.l || t.l > SIZE_WORD_LONG) return 0;
    
    uint32_t mask = 1u << (t.l - pos);
    return (t.t0 & mask) ? ((t.t1 & mask) ? 1 : -1) : 0;
}

/**
 * Установить трит в троичном регистре
 * SETUN-T1  = [s1]      - обозначение позиции тритов в регистре
 * SETUN-T5  = [s1...s5]
 * SETUN-T9  = [s1...s9]
 * SETUN-T18 = [s1...s18]
 */
trs_t set_trit_setun(trs_t t, uint8_t pos, int8_t trit)
{
    if (pos == 0 || pos > t.l || t.l > SIZE_WORD_LONG) return t;
    
    uint32_t mask = 1u << (t.l - pos);
    
    if (trit > 0) {
        t.t1 |= mask;
        t.t0 |= mask;
    } else if (trit < 0) {
        t.t1 &= ~mask;
        t.t0 |= mask;
    } else {
        t.t1 &= ~mask;
        t.t0 &= ~mask;
    }
    return t;
}

/**
 * Получить часть тритов из троичного числа (Setun порядок: s1...s18)
 * p1 - начальная позиция (1..l), p2 - конечная позиция (1..l)
 * Возвращает число длиной (p2 - p1 + 1)
 */
trs_t slice_trs_setun(trs_t t, int8_t p1, int8_t p2)
{
    if (t.l == 0 || p1 > p2 || p1 < 1 || p2 > t.l) {
        return t; /* Error */
    }
    
    uint8_t len = p2 - p1 + 1;
    uint8_t shift = t.l - p2;
    
    trs_t r;
    r.l = len;
    r.t1 = (t.t1 >> shift) & ((1u << len) - 1);
    r.t0 = (t.t0 >> shift) & ((1u << len) - 1);
    
    return r;
}

/**
 * Операция присваивания троичных чисел в регистры (Setun порядок)
 */
void copy_trs_setun(trs_t *src, trs_t *dst)
{
    if (src->l == dst->l) {
        dst->t1 = src->t1;
        dst->t0 = src->t0;
    } else if (src->l > dst->l) {
        int8_t s = src->l - dst->l;
        dst->t1 = src->t1 >> s;
        dst->t0 = src->t0 >> s;
        dst->t0 &= (1u << dst->l) - 1;
    } else {
        int8_t s = dst->l - src->l;
        dst->t1 = src->t1 << s;
        dst->t0 = src->t0 << s;
        dst->t0 &= (1u << dst->l) - 1;
    }
}

/**
 * Проверить на переполнение 18-тритного числа
 */
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

/**
 *  Проверить на переполнение 18-тритного числа
 */
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

/**
 *  Преобразование трита в номер зоны
 */
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

/**
 * Дешифратор тритов в индекс строки зоны памяти
 */
int16_t addr_trit2addr_index(trs_t t)
{
	t.t1 &= (uint32_t)0x3FFFF;
	return t.t1 >>= 1;
}

/**
 *  Дешифратор тритов в индекс адреса памяти
 *
 *  Зоны магниного барабана
 *  ZW (Non) => -13 (Dec) =  0  (index)
 *  44 (Non) => +40 (Dec) =  35 (index)
 *
 */
uint8_t zone_drum_to_index(trs_t z)
{
	int8_t r;

	r = trs2digit(z) + ZONE_DRUM_OFFSET; /* offset address trits */

	if (r > NUMBER_ZONE_DRUM - 1)
	{
		r = NUMBER_ZONE_DRUM - 1;
	}
	else if (r < 0)
	{
		r = 0;
	}

	return r;
}

/**
 * Дешифратор трита в индекс адреса физической памяти FRAM
 */
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

/**
 * Дешифратор строки 9-тритов в зоне памяти FRAM
 */
uint8_t addr2row_fram(trs_t z)
{
	int8_t r = 40;
	r += get_trit_setun(z, 4) * 1;
	r += get_trit_setun(z, 3) * 3;
	r += get_trit_setun(z, 2) * 9;
	r += get_trit_setun(z, 1) * 27;
	return (uint8_t)r;
}

/**
 * Дешифратор строки 9-тритов в зоне памяти FRAM
 */
trs_t rowzose2addr(uint8_t rind, uint8_t zind)
{
	trs_t r;
	rind -= 40;
	r.l = 5;
	r = set_trit_setun(r, 5, zind);
	return r;
}

/**
 * Новый адрес кода машины
 */
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

/**
 * Новый адрес кода машины
 */
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

/**
 * Очистить зону FRAM
 */
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

/**
 * Операция очистить память ферритовую
 */
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

/**
 * Операция очистить память на магнитном барабане
 */
void clean_drum(void)
{
	int8_t zone; /* физическое хранение в масиве */
	int8_t row;
	for (zone = 0; zone < NUMBER_ZONE_DRUM; zone++)
	{
		for (row = 0; row < SIZE_ZONE_TRIT_DRUM; row++)
		{
			mem_drum[zone][row].l = SIZE_WORD_SHORT;
			mem_drum[zone][row].t1 = 0;
			mem_drum[zone][row].t0 = 0;
		}
	}
}

/**
 * Сохранить зоны машнитного барабана в файл
 */
int Write_Backup_DRUM(char *drum_path)
{
	trs_t fa;

	clear_full(&fa);

	int8_t zone;
	int8_t row;

	FILE *file = fopen(drum_path, "wb");
	if (file == NULL)
	{
		return -1;
	}

	fseek(file, 0L, SEEK_SET);

	for (zone = 0; zone < NUMBER_ZONE_DRUM; zone++)
	{
		for (row = 0; row < SIZE_ZONE_TRIT_DRUM; row++)
		{
			uint32_t t1;
			uint32_t t0;

			t1 = mem_drum[zone][row].t1;
			t0 = mem_drum[zone][row].t0;

			fprintf(file, "%u %u\n", t1, t0);
		}
	}

	fclose(file);

	return 0;
}

/**
 * Восстановить зоны машнитного барабана в файл
 */
int Read_Backup_DRUM(char *drum_path)
{
	trs_t fa;

	clear_full(&fa);

	int8_t zone;
	int8_t row;

	FILE *file = fopen(drum_path, "rb");
	if (file == NULL)
	{
		return -1;
	}

	fseek(file, 0L, SEEK_SET);

	for (zone = 0; zone < NUMBER_ZONE_DRUM; zone++)
	{
		for (row = 0; row < SIZE_ZONE_TRIT_DRUM; row++)
		{
			uint32_t t1;
			uint32_t t0;

			if (fscanf(file, "%u %u\n", &t1, &t0) != EOF)
			{
				mem_drum[zone][row].l = SIZE_WORD_SHORT;
				mem_drum[zone][row].t1 = t1;
				mem_drum[zone][row].t0 = t0;
			}
		}
	}

	fclose(file);

	return 0;
}

/**
 * Функция "Читать троичное число из ферритовой памяти"
 */
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

/**
 *  Функция "Записи троичного числа в ферритовую память"
 */
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

	/*
	viv+ dbg	printf(" ri=%d, zi=%d\r\n",rind,zind);
	*/

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

/**
 * Копировать страницу из память fram на магнитного барабана drum
 */
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

/**
 * Операция чтения в память магнитного барабана
 */
trs_t ld_drum(trs_t ea, uint8_t ind)
{
	uint8_t zind;
	trs_t zr;
	trs_t rr;
	trs_t res;

	clear_full(&rr);

	zr = slice_trs_setun(ea, 1, 4);
	zr.l = 4;
	zind = zone_drum_to_index(zr);

	if (ind > SIZE_ZONE_TRIT_DRUM - 1)
	{
		ind = SIZE_ZONE_TRIT_DRUM - 1;
	}

	mem_drum[zind][ind].l = 9;
	res.l = 9;
	copy_trs_setun(&mem_drum[zind][ind], &res);
	return res;
}

/**
 * Операция записи в память магнитного барабана
 */
void st_drum(trs_t ea, uint8_t ind, trs_t v)
{
	uint8_t zind;
	trs_t zr;
	trs_t rr;

	clear_full(&rr);

	zr = slice_trs_setun(ea, 1, 4);
	zr.l = 4;
	zind = zone_drum_to_index(zr);

	mem_drum[zind][ind].l = 9;
	copy_trs_setun(&v, &mem_drum[zind][ind]);
}

/**
 * Копировать страницу с магнитного барабана в память fram
 */
void drum_to_fram(trs_t ea)
{
	int8_t sng;
	trs_t zram;
	trs_t fram_inc;

	clear_full(&zram);

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
    static const uint8_t TABLE[9] = {
        'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4'
    };
    
    if (v < -4 || v > 4) return '0';
    return TABLE[v + 4];
}

/**
 *  Вид '-','0','+' в десятичный код
 *
 * Преобразование символа '-','0','+' в десятичный код
 */
int8_t symtrs2numb(char c)
{
    static const int8_t TABLE[256] = {
        ['-'] = -1,
        ['0'] = 0,
        ['+'] = 1
    };
    return TABLE[(unsigned char)c];
}

/**
 *  Bp  Вид '-','0','+'
 *
 * Преобразование десятичного кода в символ '-','0','+'
 */
uint8_t numb2symtrs(int8_t v)
{
    static const uint8_t TABLE[3] = {'-', '0', '+'};
    
    if (v < 0) return '-';
    if (v > 0) return '+';
    return '0';
}

/**
 * Вид '-','0','+' строка в десятичный код
 * 
 * Преобразование строки вида '-','0','+' в десятичный код
 */
int8_t str_symtrs2numb(char *s)
{
    if (!s) return 0;
    
    int8_t len = strlen(s);
    int8_t res = 0;
    int32_t pow = 1;
    
    for (int8_t i = 0; i < len; i++) {
        res += symtrs2numb(s[i]) * pow;
        pow *= 3;
    }
    
    return res;
}

/**
 * Преобразование строки вида '-','0','+' в троичное число
 */
trs_t smtr(char *s)
{
    trs_t t;
    t.l = 0;
    t.t1 = 0;
    t.t0 = 0;
    
    if (!s) return t;
    
    int8_t len = strlen(s);
    if (len > SIZE_TRITS_MAX) len = SIZE_TRITS_MAX;
    t.l = len;
    
    for (int8_t i = 0; i < len; i++) {
        char c = s[len - i - 1];
        int8_t trit = (c == '-') ? -1 : (c == '+') ? 1 : 0;
        
        if (trit > 0) {
            t.t1 |= (1u << i);
            t.t0 |= (1u << i);
        } else if (trit < 0) {
            t.t0 |= (1u << i);
        }
    }
    
    return t;
}

/**
 * Валидация строки 
 *
 * Проверка строки на допустимые символы '-','0','+'
 * Возвращает: 0 - строка валидна, 1 - есть недопустимые символы
 */
uint8_t valid_smtr(char *s)
{
    if (!s) return 1;
    
    for (char *p = s; *p; p++) {
        char c = *p;
        if (c != '-' && c != '0' && c != '+') {
            return 1;
        }
    }
    return 0;
}

/**
 *  Девятеричный вид в троичный код
 */
char *lt2symtrs(int8_t v)
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
		return (char *)"  ";
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

 
/**
 *  Преобразовать из десятичного кода
 *  в девятиричную систему
 *
 *  С символами: W, X, Y, Z, 0, 1, 2, 3, 4.
 */
trs_t digit2trs(int8_t n)
{
//viv+ Проверить. Не используется.
	trs_t r;
	clear_full(&r);

	if (n < 3)
	{
		/*
		printf("%d",n);
		*/
	}
	else
	{
		digit2trs(n / 3);
		/*
		printf("%d",n%3);
		*/
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
 * Девятеричный вид 1YX в троичный код
 */
void regC_str_2_trs(char *syms, trs_t *r)
{
	uint8_t i = 0;
	char symtrs_str[40] = {0};

	if (strlen(syms) != 3)
	{
		r->l = 6;
		r->t1 = 0;
		r->t0 = 0;
		return;
	}

	sprintf(symtrs_str, "%2s%2s%2s",
			lt2symtrs(syms[0]),
			lt2symtrs(syms[1]),
			lt2symtrs(syms[2]));

	//viv- dbg   printf("len=%ld, %s - ", strlen(symtrs_str),symtrs_str);

	for (i = 7; i > 1; i--)
	{
		*r = set_trit_setun(*r, i, symtrs2numb(symtrs_str[i - 1]));
	}
}

/**
 * Девятеричный вид в троичный код
 */
void cmd_str_2_trs(char *syms, trs_t *r)
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

	sprintf((char *)symtrs_str, "%2s%2s%2s%2s%2s",
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
	int8_t i, n;
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
	int8_t i, n;
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
		t1 = get_long_trit(t, (uint8_t)i);
		t0 = get_long_trit(t, (uint8_t)i - 1);
		printf("%c", (int8_t)trit2lt(t1 * 3 + t0));
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

	int8_t i;
	uint8_t n;
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
 */
void view_short_reg(trs_t *t, char *ch)
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
	/*
	 printf("\nt1 % 8x\n",t->t1);
	 printf("t2 % 8x\n",t->t0);
	*/
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_trit(tv, (uint8_t)(l - 1 - i));
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");

	tv = *t;
	trs2str(tv);
	printf(", ");
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
 */
void view_short_reg_long(long_trs_t *t, uint8_t *ch)
{
	int8_t i;
	int8_t l;
	int8_t trit;
	long_trs_t tv = *t;

	printf("%s: ", (char *)ch);
	if (tv.l <= 0)
	{
		printf("\n");
		return;
	}

	l = min(tv.l, SIZE_LONG_TRITS_MAX);
	printf("[");
	/*
	 printf("\nt1 % 8x\n",t->t1);
	 printf("t2 % 8x\n",t->t0);
	*/
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_long_trit(tv, (uint8_t)(l - 1 - i));
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");

#if 0
	tv = *t;
	trs2str(tv);
	printf(", ");
	printf("(%ll)", (long long )trs2digit(*t));
#endif

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
 */
void view_short_reg_fixpoint(trs_t *t, char *ch)
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

	/*
	 printf("\nt1 % 8x\n",t->t1);
	 printf("t2 % 8x\n",t->t0);
	*/

	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_trit(tv, (uint8_t)(l - 1 - i));
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");

	tv = *t;
	trs2str(tv);
	printf(", ");
	printf("(%li)", (long int)trs2digit(*t));
	printf(", ");
	double lf = t3_to_d10(*t);
	printf("{%lf}", lf);
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
 */
void view_step_short_reg(trs_t *t, char *ch)
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
	/*
	 printf("\r\nt1 % 8x\r\n",t->t1);
	 printf("t2 % 8x\r\n",t->t0);
	*/
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_trit(tv, (uint8_t)(l - 1 - i));
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");

	tv = *t;
	trs2str(tv);
	printf(", ");
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
		/*
		 printf("\r\n");
		*/
		tv = set_trit_setun(tv, 5, -1);
		MR = ld_fram(tv);
	}
	else
	{
		MR = ld_fram(tv);
		trs2str(MR);
		/*
		 printf("\r\n");
		*/
	}

	printf(", ");
	printf("(%li)", (long int)trs2digit(MR));
	printf("\r\n");

	/*

	 tv = next_address(tv);
	 MR = ld_fram(tv);
	 printf("                                \t");
	 trs2str(MR);

	 printf("\r\n");
	*/
}

/**
 * Печать троичного регистра
 */
void view_step_new_addres(trs_t *t, char *ch)
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
	/*
	 printf("\r\nt1 % 8x\r\n",t->t1);
	 printf("t2 % 8x\r\n",t->t0);
	*/
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_trit(tv, (uint8_t)(l - 1 - i));
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");

	tv = *t;
	trs2str(tv);
	printf(", ");
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
		/*
		 printf("\r\n");
		*/
		tv = set_trit_setun(tv, 5, -1);
		MR = ld_fram(tv);
	}
	else
	{
		MR = ld_fram(tv);
		trs2str(MR);
		/*
		 printf("\r\n");
		*/
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
	/*
	 printf("\r\nt1 %p\r\n",(* long unsigned int)t->t1);
	 printf("t2 %p\r\n",(* long unsigned int)t->t0);
	*/
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_long_trit(tv, (uint8_t)(l - 1 - i));
		printf("%c", numb2symtrs(trit));
	}
	printf("], ");

	tv = *t;
	long_trs2str(tv);
	printf(", ");
	/*
	 printf("(%li)", (long int)trs2digit(*t));
	*/
	printf(", {");
	for (i = 0; i < l; i++)
	{
		tv = *t;
		trit = get_long_trit(tv, (uint8_t)(l - 1 - i));
		printf("%i", trit);
	}
	printf("}");

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

	check = slice_trs_setun(t, 1, 9);
	view_short_reg(&check, "");

	check = slice_trs_setun(t, 10, 18);
	view_short_reg(&check, "");

	printf("-KC = 0-KC:\r\n");
	check = neg_trs(t);
	ncheck = slice_trs_setun(check, 1, 9);
	ncheck.l = 9;
	view_short_reg(&ncheck, "");

	check = slice_trs_setun(check, 10, 18);
	check.l = 9;
	view_short_reg(&check, "");

	printf("\r\n");
}

/**
 * Печать регистров машины Сетунь-1958
 */
void view_short_regs(void)
{

	/*
	 printf("[Registers Setun-1958]\r\n");
	*/
	printf("\r\n");
	view_short_reg(&K, "  K  ");
	view_short_reg(&F, "  F  ");
	view_short_reg(&C, "  C  ");
	view_short_reg(&W, "  W  ");
	view_short_reg(&ph1, "  ph1");
	view_short_reg(&ph2, "  ph2");
	/*
	 printf("TODO Добавить вывод S,R float числа. \r\n");
	 view_short_reg(&S, "  S  ");
	 view_short_reg(&R, "  R  ");
	*/
	view_short_reg_fixpoint(&S, "  S  ");
	view_short_reg_fixpoint(&R, "  R  ");
	view_short_reg(&MB, "  MB ");
}

/**
 * Печать памяти FRAM машины Сетунь-1958
 */
void view_elem_fram(trs_t ea)
{
	uint8_t j;
	trs_t tv;

	clear_full(&tv);

	uint8_t zind;
	uint8_t rind;
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

/**
 * Печать памяти FRAM машины Сетунь-1958 с addr1, addr2
 */
void view_fram(trs_t addr1, trs_t addr2)
{
	trs_t ad1 = addr1;
	trs_t ad2 = addr2;
	int32_t a1 = trs2digit(ad1);
	int32_t a2 = trs2digit(ad2);

	if ((a2 >= a1) && (a2 >= ZONE_M_FRAM_BEG && a2 <= ZONE_P_FRAM_END) && (a2 >= ZONE_M_FRAM_BEG && a2 <= ZONE_P_FRAM_END))
	{
		for (uint16_t i = 0; i < (abs(a2 - a1) + 1); i++)
		{
			/*
			 inc_trs(&ad1);
			*/
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
	uint8_t j;
	trs_t tv;
	trs_t r;

	clear_full(&tv);

	printf("\r\n[ Dump FRAM Setun-1958: ]\r\n");

	for (row = 0; row < SIZE_GR_TRIT_FRAM; row++)
	{
		for (grfram = 0; grfram < SIZE_GRFRAM; grfram++)
		{
			r = mem_fram[row][grfram];
			r.l = 9;

			/* viv+ dbg view_short_reg(&r,"r"); */

			printf("fram[.] (%3d:%2d) : ", row - SIZE_GR_TRIT_FRAM / 2, grfram);
			/*
			 printf("%s",trs2str());
			*/

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

	printf("\r\n");
	printf("FRAM Zone = % 2i\r\n", sng);
	printf("\r\n");

	for (uint8_t i = 0; i < 14; i++)
	{
		/* Print ROW1 */
		/* addr -1 */
		r = slice_trs_setun(fram_row1, 2, 3);
		trs2str(r);
		r = slice_trs_setun(fram_row1, 4, 5);
		trs2str(r);
		printf(" ");
		/* addr 0 */
		fram_row1 = set_trit_setun(fram_row1, 5, 0);
		r = slice_trs_setun(fram_row1, 2, 3);
		trs2str(r);
		r = slice_trs_setun(fram_row1, 4, 5);
		trs2str(r);
		printf("  ");
		/* addr 1 */
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

	printf("\r\n");
	printf("FRAM Zone = % 2i\r\n", sng);
	printf("\r\n");
}

/**
 * Печать короткого слова DRUM машины Сетунь-1958
 */
void view_drum_zone(trs_t zone)
{
	uint8_t j;
	trs_t zr;
	uint8_t zind;
	trs_t tv;

	clear_full(&tv);

	/* Зона памяти DRUM */
	zr = slice_trs_setun(zone, 1, 4);
	zind = zone_drum_to_index(zr);

	printf("\r\n[ Dump DRUM Setun-1958: ]\r\n");

	printf("\r\n");
	printf("Zone  [");
	for (j = 1; j < 5; j++)
	{
		printf("%c", numb2symtrs(get_trit_setun(zr, j)));
	};
	printf("], ");
	trs2str(zr);
	printf("\r\n");
	printf("      index  (%i) \r\n", zind);
	printf("\r\n");

	for (uint8_t i = 0; i < SIZE_ZONE_TRIT_DRUM; i++)
	{
		/* Читать короткое слово */
		trs_t mr = ld_drum(zr, i);
		mr.l = 9;
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

	printf("\r\n");
	printf("Zone  [");
	for (j = 1; j < 5; j++)
	{
		printf("%c", numb2symtrs(get_trit_setun(zr, j)));
	};
	printf("], ");
	trs2str(zr);
	printf("\r\n");
	printf("      index  (%i) \r\n", zind);
	printf("\r\n");
}

/**
 * Печать памяти DRUM машины Сетунь-1958
 */
void dump_drum(void)
{
	int8_t zone; /* физическоне хранение массива */
	int8_t row;
	int8_t j;
	trs_t tv;
	trs_t r;

	clear_full(&tv);
	clear_full(&r);

	printf("\r\n[ DRUM Setun-1958 ]\r\n");

	for (zone = 0; zone < NUMBER_ZONE_DRUM; zone++)
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
uint8_t tab4_0[TAB_SIZE][STRING_PAPER_SIZE + 1];
trs_t tab4_1[TAB_SIZE];
trs_t tab4_2[TAB_SIZE];
uint8_t tab4_3[TAB_SIZE][SYMBOL_SIZE + 1];
uint8_t tab4_4[TAB_SIZE][SYMBOL_SIZE + 1];
uint8_t tab4_5[TAB_SIZE][SYMBOL_SIZE + 1];
uint8_t tab4_6[TAB_SIZE][SYMBOL_SIZE + 1];
uint8_t tab4_7[TAB_MIN_SIZE][STRING_PAPER_SIZE + 1];
trs_t tab4_8[TAB_MIN_SIZE];

/**
 * Инициализация таблицы кодирования
 * информации ввод и вывода
 */
void init_tab4(void)
{

	/* Init tab4_0 */
	uint8_t offset = 13;
	memmove(tab4_0[trs2digit(smtr("---")) + offset], "_OO.O_", sizeof("_OO.O_"));
	memmove(tab4_0[trs2digit(smtr("--0")) + offset], "_O_.OO", sizeof("_O_.OO"));
	memmove(tab4_0[trs2digit(smtr("--+")) + offset], "OO_.OO", sizeof("OO_.OO"));
	memmove(tab4_0[trs2digit(smtr("-0-")) + offset], "_OO.__", sizeof("_OO.__"));
	memmove(tab4_0[trs2digit(smtr("-00")) + offset], "___.OO", sizeof("___.OO"));
	memmove(tab4_0[trs2digit(smtr("-0+")) + offset], "O__.OO", sizeof("O__.OO"));
	memmove(tab4_0[trs2digit(smtr("-+-")) + offset], "_OO._O", sizeof("_OO._O"));
	memmove(tab4_0[trs2digit(smtr("-+0")) + offset], "__O.OO", sizeof("__O.OO"));
	memmove(tab4_0[trs2digit(smtr("-++")) + offset], "O_O.OO", sizeof("O_O.OO"));
	memmove(tab4_0[trs2digit(smtr("0--")) + offset], "_O_.O_", sizeof("_O_.O_"));
	memmove(tab4_0[trs2digit(smtr("0-0")) + offset], "_O_.__", sizeof("_O_.__"));
	memmove(tab4_0[trs2digit(smtr("0-+")) + offset], "_O_._O", sizeof("_O_._O"));
	memmove(tab4_0[trs2digit(smtr("00-")) + offset], "___.O_", sizeof("___.O_"));
	memmove(tab4_0[trs2digit(smtr("000")) + offset], "_OO.OO", sizeof("_OO.OO"));
	memmove(tab4_0[trs2digit(smtr("00+")) + offset], "___._O", sizeof("___._O"));
	memmove(tab4_0[trs2digit(smtr("0+-")) + offset], "__O.O_", sizeof("__O.O_"));

	memmove(tab4_0[trs2digit(smtr("0+0")) + offset], "__O.__", sizeof("__O.__"));
	memmove(tab4_0[trs2digit(smtr("0++")) + offset], "__O._O", sizeof("__O._O"));
	memmove(tab4_0[trs2digit(smtr("+--")) + offset], "OO_.O_", sizeof("OO_.O_"));
	memmove(tab4_0[trs2digit(smtr("+-0")) + offset], "OO_.__", sizeof("OO_.__"));
	memmove(tab4_0[trs2digit(smtr("+-+")) + offset], "OO_._O", sizeof("OO_._O"));
	memmove(tab4_0[trs2digit(smtr("+0-")) + offset], "O__.O_", sizeof("O__.O_"));
	memmove(tab4_0[trs2digit(smtr("+00")) + offset], "O__.__", sizeof("O__.__"));
	memmove(tab4_0[trs2digit(smtr("+0+")) + offset], "O__._O", sizeof("O__._O"));
	memmove(tab4_0[trs2digit(smtr("++-")) + offset], "O_O.O_", sizeof("O_O.O_"));
	memmove(tab4_0[trs2digit(smtr("++0")) + offset], "O_O._O", sizeof("O_O._O"));
	memmove(tab4_0[trs2digit(smtr("+++")) + offset], "O_O.__", sizeof("O_O.__"));

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
	memmove(tab4_3[pl_to_ind("_OO.O_")], "\0", sizeof("\0")); /* стоп */
	memmove(tab4_3[pl_to_ind("_O_.OO")], "-", sizeof("-"));
	memmove(tab4_3[pl_to_ind("OO_.OO")], "_", sizeof("_"));
	memmove(tab4_3[pl_to_ind("_OO.__")], "\n", sizeof("\n"));
	memmove(tab4_3[pl_to_ind("___.OO")], "/", sizeof("/"));
	memmove(tab4_3[pl_to_ind("O__.OO")], ".", sizeof("."));
	memmove(tab4_3[pl_to_ind("_OO._O")], "x", sizeof("x"));
	memmove(tab4_3[pl_to_ind("__O.OO")], "+", sizeof("+"));
	memmove(tab4_3[pl_to_ind("O_O.OO")], "V", sizeof("V"));
	memmove(tab4_3[pl_to_ind("_O_.O_")], "W", sizeof("W"));
	memmove(tab4_3[pl_to_ind("_O_.__")], "X", sizeof("X"));
	memmove(tab4_3[pl_to_ind("_O_._O")], "Y", sizeof("Y"));
	memmove(tab4_3[pl_to_ind("___.O_")], "Z", sizeof("Z"));
	memmove(tab4_3[pl_to_ind("_OO.OO")], "O", sizeof("O"));
	memmove(tab4_3[pl_to_ind("___._O")], "1", sizeof("1"));
	memmove(tab4_3[pl_to_ind("__O.O_")], "2", sizeof("2"));

	memmove(tab4_3[pl_to_ind("__O.__")], "3", sizeof("3"));
	memmove(tab4_3[pl_to_ind("__O._O")], "4", sizeof("4"));
	memmove(tab4_3[pl_to_ind("OO_.O_")], "5", sizeof("5"));
	memmove(tab4_3[pl_to_ind("OO_.__")], "6", sizeof("6"));
	memmove(tab4_3[pl_to_ind("OO_._O")], "7", sizeof("7"));
	memmove(tab4_3[pl_to_ind("O__.O_")], "8", sizeof("8"));
	memmove(tab4_3[pl_to_ind("O__.__")], "9", sizeof("9"));
	memmove(tab4_3[pl_to_ind("O__._O")], " ", sizeof(" "));
	memmove(tab4_3[pl_to_ind("O_O.O_")], " ", sizeof(" ")); /* цифровой регистр */
	memmove(tab4_3[pl_to_ind("O_O._O")], ")", sizeof(")"));
	memmove(tab4_3[pl_to_ind("O_O.__")], " ", sizeof(" ")); /* буквенный регистр */
	memmove(tab4_3[pl_to_ind("___.__")], " ", sizeof(" "));
	memmove(tab4_3[pl_to_ind("OOO.__")], " ", sizeof(" "));
	memmove(tab4_3[pl_to_ind("OOO._O")], " ", sizeof(" "));
	memmove(tab4_3[pl_to_ind("OOO.O_")], " ", sizeof(" "));
	memmove(tab4_3[pl_to_ind("OOO.OO")], " ", sizeof(" ")); /* вычеркивание */

	/* Init tab4_4 */
	memmove(tab4_4[pl_to_ind("_OO.O_")], "\0", sizeof("\0")); /* стоп */
	memmove(tab4_4[pl_to_ind("_O_.OO")], "-", sizeof("-"));
	memmove(tab4_4[pl_to_ind("OO_.OO")], "_", sizeof("_"));
	memmove(tab4_4[pl_to_ind("_OO.__")], "\n", sizeof("\n"));
	memmove(tab4_4[pl_to_ind("___.OO")], "Ю", sizeof("Ю"));
	memmove(tab4_4[pl_to_ind("O__.OO")], ".", sizeof("."));
	memmove(tab4_4[pl_to_ind("_OO._O")], "x", sizeof("x"));
	memmove(tab4_4[pl_to_ind("__O.OO")], "+", sizeof("+"));
	memmove(tab4_4[pl_to_ind("O_O.OO")], "Э", sizeof("Э"));
	memmove(tab4_4[pl_to_ind("_O_.O_")], "Ж", sizeof("Ж"));
	memmove(tab4_4[pl_to_ind("_O_.__")], "Х", sizeof("Х"));
	memmove(tab4_4[pl_to_ind("_O_._O")], "У", sizeof("У"));
	memmove(tab4_4[pl_to_ind("___.O_")], "Ц", sizeof("Ц"));
	memmove(tab4_4[pl_to_ind("_OO.OO")], "O", sizeof("O"));
	memmove(tab4_4[pl_to_ind("___._O")], "1", sizeof("1"));
	memmove(tab4_4[pl_to_ind("__O.O_")], "2", sizeof("2"));

	memmove(tab4_4[pl_to_ind("__O.__")], "3", sizeof("3"));
	memmove(tab4_4[pl_to_ind("__O._O")], "4", sizeof("4"));
	memmove(tab4_4[pl_to_ind("OO_.O_")], "5", sizeof("5"));
	memmove(tab4_4[pl_to_ind("OO_.__")], "6", sizeof("6"));
	memmove(tab4_4[pl_to_ind("OO_._O")], "7", sizeof("7"));
	memmove(tab4_4[pl_to_ind("O__.O_")], "8", sizeof("8"));
	memmove(tab4_4[pl_to_ind("O__.__")], "9", sizeof("9"));
	memmove(tab4_4[pl_to_ind("O__._O")], " ", sizeof(" "));
	memmove(tab4_4[pl_to_ind("O_O.O_")], " ", sizeof(" ")); /* цифровой регистр */
	memmove(tab4_4[pl_to_ind("O_O._O")], "Ф", sizeof("Ф"));
	memmove(tab4_4[pl_to_ind("O_O.__")], " ", sizeof(" ")); /* буквенный регистр */
	memmove(tab4_4[pl_to_ind("___.__")], " ", sizeof(" "));
	memmove(tab4_4[pl_to_ind("OOO.__")], " ", sizeof(" "));
	memmove(tab4_4[pl_to_ind("OOO._O")], " ", sizeof(" "));
	memmove(tab4_4[pl_to_ind("OOO.O_")], " ", sizeof(" "));
	memmove(tab4_4[pl_to_ind("OOO.OO")], " ", sizeof(" ")); /* вычеркивание */

	/* Init tab4_5 */
	memmove(tab4_5[pl_to_ind("_OO.O_")], " ", sizeof(" "));
	memmove(tab4_5[pl_to_ind("_O_.OO")], "F", sizeof("F"));
	memmove(tab4_5[pl_to_ind("OO_.OO")], "_", sizeof("_"));
	memmove(tab4_5[pl_to_ind("_OO.__")], "\n", sizeof("\n"));
	memmove(tab4_5[pl_to_ind("___.OO")], "G", sizeof("G"));
	memmove(tab4_5[pl_to_ind("O__.OO")], "H", sizeof("H"));
	memmove(tab4_5[pl_to_ind("_OO._O")], "=", sizeof("="));
	memmove(tab4_5[pl_to_ind("__O.OO")], "I", sizeof("I"));
	memmove(tab4_5[pl_to_ind("O_O.OO")], "J", sizeof("J"));
	memmove(tab4_5[pl_to_ind("_O_.O_")], "K", sizeof("K"));
	memmove(tab4_5[pl_to_ind("_O_.__")], "L", sizeof("L"));
	memmove(tab4_5[pl_to_ind("_O_._O")], "M", sizeof("M"));
	memmove(tab4_5[pl_to_ind("___.O_")], "N", sizeof("N"));
	memmove(tab4_5[pl_to_ind("_OO.OO")], "P", sizeof("P"));
	memmove(tab4_5[pl_to_ind("___._O")], "Q", sizeof("Q"));
	memmove(tab4_5[pl_to_ind("__O.O_")], "R", sizeof("R"));

	memmove(tab4_5[pl_to_ind("__O.__")], "S", sizeof("S"));
	memmove(tab4_5[pl_to_ind("__O._O")], "T", sizeof("T"));
	memmove(tab4_5[pl_to_ind("OO_.O_")], "U", sizeof("U"));
	memmove(tab4_5[pl_to_ind("OO_.__")], "A", sizeof("A"));
	memmove(tab4_5[pl_to_ind("OO_._O")], "B", sizeof("B"));
	memmove(tab4_5[pl_to_ind("O__.O_")], "C", sizeof("C"));
	memmove(tab4_5[pl_to_ind("O__.__")], "D", sizeof("D"));
	memmove(tab4_5[pl_to_ind("O__._O")], "E", sizeof("E"));
	memmove(tab4_5[pl_to_ind("O_O.O_")], " ", sizeof(" ")); /* цифровой регистр */
	memmove(tab4_5[pl_to_ind("O_O._O")], "(", sizeof("("));
	memmove(tab4_5[pl_to_ind("O_O.__")], " ", sizeof(" ")); /* буквенный регистр */
	memmove(tab4_5[pl_to_ind("___.__")], " ", sizeof(" "));
	memmove(tab4_5[pl_to_ind("OOO.__")], " ", sizeof(" "));
	memmove(tab4_5[pl_to_ind("OOO._O")], " ", sizeof(" "));
	memmove(tab4_5[pl_to_ind("OOO.O_")], " ", sizeof(" "));
	memmove(tab4_5[pl_to_ind("OOO.OO")], " ", sizeof(" ")); /* вычеркивание */

	/* Init tab4_6 */
	memmove(tab4_6[pl_to_ind("_OO.O_")], " ", sizeof(" ")); /* стоп */
	memmove(tab4_6[pl_to_ind("_O_.OO")], "Б", sizeof("Б"));
	memmove(tab4_6[pl_to_ind("OO_.OO")], "_", sizeof("_"));
	memmove(tab4_6[pl_to_ind("_OO.__")], "\n", sizeof("\n"));
	memmove(tab4_6[pl_to_ind("___.OO")], "Щ", sizeof("Щ"));
	memmove(tab4_6[pl_to_ind("O__.OO")], "Н", sizeof("Н"));
	memmove(tab4_6[pl_to_ind("_OO._O")], "=", sizeof("="));
	memmove(tab4_6[pl_to_ind("__O.OO")], "П", sizeof("П"));
	memmove(tab4_6[pl_to_ind("O_O.OO")], "Ы", sizeof("Ы"));
	memmove(tab4_6[pl_to_ind("_O_.O_")], "К", sizeof("К"));
	memmove(tab4_6[pl_to_ind("_O_.__")], "Г", sizeof("Г"));
	memmove(tab4_6[pl_to_ind("_O_._O")], "М", sizeof("М"));
	memmove(tab4_6[pl_to_ind("___.O_")], "И", sizeof("И"));
	memmove(tab4_6[pl_to_ind("_OO.OO")], "Р", sizeof("Р"));
	memmove(tab4_6[pl_to_ind("___._O")], "Й", sizeof("Й"));
	memmove(tab4_6[pl_to_ind("__O.O_")], "Я", sizeof("Я"));

	memmove(tab4_6[pl_to_ind("__O.__")], "Ь", sizeof("Ь"));
	memmove(tab4_6[pl_to_ind("__O._O")], "Т", sizeof("Т"));
	memmove(tab4_6[pl_to_ind("OO_.O_")], "П", sizeof("П"));
	memmove(tab4_6[pl_to_ind("OO_.__")], "А", sizeof("А"));
	memmove(tab4_6[pl_to_ind("OO_._O")], "В", sizeof("В"));
	memmove(tab4_6[pl_to_ind("O__.O_")], "С", sizeof("С"));
	memmove(tab4_6[pl_to_ind("O__.__")], "Д", sizeof("Д"));
	memmove(tab4_6[pl_to_ind("O__._O")], "Е", sizeof("Е"));
	memmove(tab4_6[pl_to_ind("O_O.O_")], " ", sizeof(" ")); /* цифровой регистр */
	memmove(tab4_6[pl_to_ind("O_O._O")], "Ш", sizeof("Ш"));
	memmove(tab4_6[pl_to_ind("O_O.__")], " ", sizeof(" ")); /* буквенный регистр */
	memmove(tab4_6[pl_to_ind("___.__")], " ", sizeof(" "));
	memmove(tab4_6[pl_to_ind("OOO.__")], " ", sizeof(" "));
	memmove(tab4_6[pl_to_ind("OOO._O")], " ", sizeof(" "));
	memmove(tab4_6[pl_to_ind("OOO.O_")], " ", sizeof(" "));
	memmove(tab4_6[pl_to_ind("OOO.OO")], " ", sizeof(" ")); /* вычеркивание */

	/* Init tab4_7 */
	uint8_t offset4_7 = 4;
	memmove(tab4_7[trs2digit(smtr("--")) + offset4_7], "_O_.O_", sizeof("_O_.O_"));
	memmove(tab4_7[trs2digit(smtr("-0")) + offset4_7], "_O_.__", sizeof("_O_.__"));
	memmove(tab4_7[trs2digit(smtr("-+")) + offset4_7], "_O_._O", sizeof("_O_._O"));
	memmove(tab4_7[trs2digit(smtr("0-")) + offset4_7], "___.O_", sizeof("___.O_"));
	memmove(tab4_7[trs2digit(smtr("00")) + offset4_7], "_OO.OO", sizeof("_OO.OO"));
	memmove(tab4_7[trs2digit(smtr("0+")) + offset4_7], "___._O", sizeof("___._O"));
	memmove(tab4_7[trs2digit(smtr("+-")) + offset4_7], "__O.O_", sizeof("__O.O_"));
	memmove(tab4_7[trs2digit(smtr("+0")) + offset4_7], "__O.__", sizeof("__O.__"));
	memmove(tab4_7[trs2digit(smtr("++")) + offset4_7], "__O._O", sizeof("__O._O"));

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

/**
 * Декодирование строки в индекс таблицы
 * 
 * Преобразование строки перфоленты в индекс таблицы
 * Формат: 6 символов с точкой в позиции 3
 * Пример: "_OO.O_"
 */
uint8_t pl_to_ind(char *perline)
{
    if (!perline || strlen(perline) != 6) return 0;
    if (perline[3] != '.') return 0;
    
    uint8_t byte = 0;
    
    /* Позиции: 0->бит4, 1->бит3, 2->бит2, 4->бит1, 5->бит0 */
    if (perline[0] == 'O' || perline[0] == 'o') byte |= (1 << 4);
    if (perline[1] == 'O' || perline[1] == 'o') byte |= (1 << 3);
    if (perline[2] == 'O' || perline[2] == 'o') byte |= (1 << 2);
    if (perline[4] == 'O' || perline[4] == 'o') byte |= (1 << 1);
    if (perline[5] == 'O' || perline[5] == 'o') byte |= (1 << 0);
    
    return byte;
}

/**
 * Декодирование строк из файла paper.txt
 * Возвращает: 1 - успех, 0 - ошибка
 */
int16_t Decoder_String_from_Paper_Line(void)
{
    FILE *filepl = fopen("software/paper.txt", "r");
    if (!filepl) {
        printf("ERROR fopen %s\r\n", "software/paper.txt");
        return 0;
    }
    
    printf("open %s\r\n", "software/paper.txt");
    
    char cmd[20];
    int16_t count = 0;
    
    while (fscanf(filepl, "%19s", cmd) == 1) {
        printf("%s : ", cmd);
        
        /* Проверка длины */
        size_t len = strlen(cmd);
        if (len != 6) {
            printf("ERROR: invalid length %zu\r\n", len);
            fclose(filepl);
            return 0;
        }
        
        /* Проверка точки в позиции 3 */
        if (cmd[3] != '.') {
            printf("ERROR: invalid format\r\n");
            fclose(filepl);
            return 0;
        }
        
        /* Декодирование */
        uint8_t byte = 0;
        if (cmd[0] == 'O' || cmd[0] == 'o') byte |= (1 << 4);
        if (cmd[1] == 'O' || cmd[1] == 'o') byte |= (1 << 3);
        if (cmd[2] == 'O' || cmd[2] == 'o') byte |= (1 << 2);
        if (cmd[4] == 'O' || cmd[4] == 'o') byte |= (1 << 1);
        if (cmd[5] == 'O' || cmd[5] == 'o') byte |= (1 << 0);
        
        printf("byte=%u\r\n", byte);
        count++;
    }
    
    fclose(filepl);
    printf("\r\nr=%i\n", count);
    return 1;
}

/**
 * Декодирование комбинации пробивок в троичное число (команды)
 */
trs_t Decoder_Command_Paper_Line(char *paperline, uint8_t *err)
{
    trs_t res;
    clear_full(&res);
    *err = 1;  /* По умолчанию ошибка */
    
    if (!paperline) return res;
    
    size_t len = strlen(paperline);
    if (len != 6) return res;
    if (paperline[3] != '.') return res;
    
    /* Декодирование */
    uint8_t byte = 0;
    if (paperline[0] == 'O' || paperline[0] == 'o') byte |= (1 << 4);
    if (paperline[1] == 'O' || paperline[1] == 'o') byte |= (1 << 3);
    if (paperline[2] == 'O' || paperline[2] == 'o') byte |= (1 << 2);
    if (paperline[4] == 'O' || paperline[4] == 'o') byte |= (1 << 1);
    if (paperline[5] == 'O' || paperline[5] == 'o') byte |= (1 << 0);
    
    /* Проверка допустимых комбинаций */
    res = tab4_2[byte];
    int r = trs2digit(res);
    
    if (r > 4) {
        *err = 1;
        clear_full(&res);
    } else {
        *err = 0;
    }
    
    return res;
}

/**
 * Декодирование комбинации пробивок в троичное число (символы)
 */
trs_t Decoder_Symbol_Paper_Line(char *paperline, uint8_t *err)
{
    trs_t res;
    clear_full(&res);
    *err = 1;  /* По умолчанию ошибка */
    
    if (!paperline) return res;
    
    size_t len = strlen(paperline);
    if (len != 6 || paperline[3] != '.') return res;
    
    uint8_t byte = 0;
    if (paperline[0] == 'O' || paperline[0] == 'o') byte |= (1 << 4);
    if (paperline[1] == 'O' || paperline[1] == 'o') byte |= (1 << 3);
    if (paperline[2] == 'O' || paperline[2] == 'o') byte |= (1 << 2);
    if (paperline[4] == 'O' || paperline[4] == 'o') byte |= (1 << 1);
    if (paperline[5] == 'O' || paperline[5] == 'o') byte |= (1 << 0);
    
    res = tab4_1[byte];
    *err = 0;  /* Ok' */
    
    return res;
}

/**
 * Печать на электрифицированную пишущую машинку
 * 'An electrified typewriter'
 */
void electrified_typewriter(trs_t t, uint8_t local)
{
	int32_t code;

	russian_latin_sw = local;
	code = trs2digit(t);
	color_sw += 0;

	// Таблицы символов для каждого режима
	static const char *russian_letters[] = {
		/* -12 */ "Б", /* -11 */ "", /* -10 */ "", /* -9 */ "Щ", /* -8 */ "Н",
		/* -7 */ "=", /* -6 */ "Л", /* -5 */ "Ы", /* -4 */ "К", /* -3 */ "Г",
		/* -2 */ "М", /* -1 */ "И", /* 0 */ "Р", /* 1 */ "Й", /* 2 */ "Я",
		/* 3 */ "Ь", /* 4 */ "Т", /* 5 */ "П", /* 6 */ "А", /* 7 */ "В",
		/* 8 */ "С", /* 9 */ "Д", /* 10 */ "Е", /* 11 */ "", /* 12 */ "",
		/* 13 */ "Ш"};

	static const char *russian_numbers[] = {
		/* -12 */ "-", /* -11 */ "", /* -10 */ "", /* -9 */ "Ю", /* -8 */ ",",
		/* -7 */ "х", /* -6 */ "+", /* -5 */ "Э", /* -4 */ "Ж", /* -3 */ "Х",
		/* -2 */ "У", /* -1 */ "Ц", /* 0 */ "О", /* 1 */ "1", /* 2 */ "2",
		/* 3 */ "3", /* 4 */ "4", /* 5 */ "5", /* 6 */ "6", /* 7 */ "7",
		/* 8 */ "8", /* 9 */ "9", /* 10 */ " ", /* 11 */ "", /* 12 */ "",
		/* 13 */ "Ф"};

	static const char *latin_letters[] = {
		/* -12 */ "F", /* -11 */ "?", /* -10 */ "", /* -9 */ "G", /* -8 */ "H",
		/* -7 */ "=", /* -6 */ "I", /* -5 */ "J", /* -4 */ "K", /* -3 */ "L",
		/* -2 */ "M", /* -1 */ "N", /* 0 */ "P", /* 1 */ "Q", /* 2 */ "R",
		/* 3 */ "S", /* 4 */ "T", /* 5 */ "U", /* 6 */ "A", /* 7 */ "B",
		/* 8 */ "C", /* 9 */ "D", /* 10 */ "E", /* 11 */ "", /* 12 */ "",
		/* 13 */ "("};

	static const char *latin_numbers[] = {
		/* -12 */ "-", /* -11 */ "?", /* -10 */ "", /* -9 */ "/", /* -8 */ ".",
		/* -7 */ "x", /* -6 */ "+", /* -5 */ "V", /* -4 */ "W", /* -3 */ "X",
		/* -2 */ "Y", /* -1 */ "Z", /* 0 */ "O", /* 1 */ "1", /* 2 */ "2",
		/* 3 */ "3", /* 4 */ "4", /* 5 */ "5", /* 6 */ "6", /* 7 */ "7",
		/* 8 */ "8", /* 9 */ "9", /* 10 */ " ", /* 11 */ "", /* 12 */ "",
		/* 13 */ ")"};

	/* Преобразование кода в индекс таблицы (код от -12 до 13, массив от 0 до 25) */
	int index = code + 12;

	/* Проверка границ массива */
	if (index < 0 || index > 25)
	{
		return; /* Недопустимый код */
	}

	/* Обработка специальных случаев */
	switch (code)
	{
	case -11: /* Переключение цвета */
		if (russian_latin_sw == 0)
		{   /*
			 * russian
			 * переключить цвет черный/красный - реализация зависит от системы
			 */
		}
		/* В латинском режиме просто печатает "?" */
		break;

	case 12: /* t = 110 - переключение на буквы */
		letter_number_sw = 0;
		return;

	case 11: /* t = 11-1 - переключение на цифры */
		letter_number_sw = 1;
		return;

	case -10: /* Перевод строки */
		printf("%s", "\r\n");
		if (russian_latin_sw == 0)
		{ /* russian */
			fwrite("\n", 1, 1, tty1);
		}
		else
		{ /* latin */
			fwrite("\r\n", 1, 2, tty1);
		}
		return;
	}

	/* Выбор символа из соответствующей таблицы */
	const char *symbol = NULL;

	if (russian_latin_sw == 0)
	{ /* russian */
		if (letter_number_sw == 0)
		{ /* letter */
			symbol = russian_letters[index];
		}
		else
		{ /* number */
			symbol = russian_numbers[index];
		}
	}
	else
	{ /* latin */
		if (letter_number_sw == 0)
		{ /* letter */
			symbol = latin_letters[index];
		}
		else
		{ /* number */
			symbol = latin_numbers[index];
		}
	}

	/* Печать символа, если он существует */
	if (symbol && symbol[0] != '\0')
	{
		printf("%s", symbol);
		/* Для однобайтовых символов (ASCII) */
		if (strlen(symbol) == 1)
		{
			fwrite(symbol, 1, 1, tty1);
		}
		/* Для многобайтовых символов (кириллица) */
		else
		{
			fwrite(symbol, strlen(symbol), 1, tty1);
		}
	}
}

/**
 * Старт машины с фотосчитывателя №1
 */
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
	if (LOGGING > 0)
	{
		printf("[ Read commands from FT1 ]\r\n");
	}

	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	trs_t dst;
	trs_t addr;
	trs_t oper;
	char cmd[20];

	clear_full(&dst);
	clear_full(&addr);
	clear_full(&oper);

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	clear_full(&sum);
	clear_full(&tmp);

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;

	dst.l = 9;
	MR.l = 18;
	int i = 0;

	trs_t tcmd;
	tcmd.l = 9;
	tcmd.t0 = 0;
	tcmd.t1 = 0;

	uint8_t cnt_cmd = 54;
	uint8_t cnt_line = 5;

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
				/*
				view_short_reg(&tcmd, "tcmd");
				*/
				sum = add_trs(sum, tcmd);

				dsun += trs2digit(tcmd);

				/*
				LOGGING_print("%s -> [", tcmd);
				*/
				LOGGING_print(" -> [");

				if (LOGGING > 0)
				{
					trs2str(tcmd);
				}
				LOGGING_print("]");
				if (LOGGING > 0)
				{
					view_short_reg(&fa, " addr");
				}
				else
				{
					if (LOGGING > 0)
					{
						printf("\r\n");
					}
				}

				st_fram(fa, tcmd);
				fa = next_address(fa);
				mod_3_n(&fa, 5);
				tcmd.t0 = 0;
				tcmd.t1 = 0;

				i += 1;
			}
			else
			{
				tcmd = shift_trs(tcmd, 2);
			}
		}
	}

	if (LOGGING > 0)
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
	if (LOGGING > 0)
	{
		printf("[ Read symbols from FT1 ]\r\n");
	}

	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	trs_t dst;
	trs_t addr;
	trs_t oper;
	char cmd[20];

	clear_full(&dst);
	clear_full(&addr);
	clear_full(&oper);

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	clear_full(&tmp);

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;

	dst.l = 9;
	MR.l = 18;
	int i = 0;

	trs_t tcmd;
	tcmd.l = 9;
	tcmd.t0 = 0;
	tcmd.t1 = 0;

	uint8_t cnt_cmd = 54;
	uint8_t cnt_line = 3;

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

				LOGGING_print(" -> [");

				if (LOGGING > 0)
				{
					trs2str(tcmd);
				}
				LOGGING_print("]");
				if (LOGGING > 0)
				{
					view_short_reg(&fa, " addr");
				}
				else
				{
					if (LOGGING > 0)
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

				i += 1;
			}
			else
			{
				tcmd = shift_trs(tcmd, 3);
			}
		}
	}

	if (LOGGING > 0)
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
	char cmd[20];

	clear_full(&dst);
	clear_full(&addr);
	clear_full(&oper);

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	clear_full(&tmp);

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;

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

		LOGGING_print("%s -> [", cmd);

		if (LOGGING > 0)
		{
			trs2str(dst);
		}
		LOGGING_print("]");
		if (LOGGING > 0)
		{
			view_short_reg(&fa, " addr");
		}
		else
		{
			if (LOGGING > 0)
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
	if (LOGGING > 0)
	{
		printf("[ Read symbols from FT2 ]\r\n");
	}

	uint8_t cnt = 0;

	if (file == NULL)
		return 1; /* Error #1 */

	trs_t dst;
	trs_t addr;
	trs_t oper;
	char cmd[20];

	clear_full(&dst);
	clear_full(&addr);
	clear_full(&oper);

	/**
	 * Загрузить
	 */

	trs_t sum;
	trs_t tmp;

	clear_full(&sum);
	clear_full(&tmp);

	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;

	dst.l = 9;
	MR.l = 18;
	int i = 0;

	trs_t tcmd;
	tcmd.l = 9;
	tcmd.t0 = 0;
	tcmd.t1 = 0;

	uint8_t cnt_cmd = 54;
	uint8_t cnt_line = 3;

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
				tcmd = set_trit_setun(tcmd, 7, p1); // viv~ Провверить
				tcmd = set_trit_setun(tcmd, 8, p2);
				tcmd = set_trit_setun(tcmd, 9, p3);

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

				LOGGING_print(" -> [");

				if (LOGGING > 0)
				{
					trs2str(tcmd);
				}
				LOGGING_print("]");
				if (LOGGING > 0)
				{
					view_short_reg(&fa, " addr");
				}
				else
				{
					if (LOGGING > 0)
					{
						printf("\r\n");
					}
				}

				st_fram(fa, tcmd);
				fa = next_address(fa);
				mod_3_n(&fa, 5);
				tcmd.t0 = 0;
				tcmd.t1 = 0;

				i += 1;
			}
			else
			{
				tcmd = shift_trs(tcmd, 3);
			}
		}
	}

	if (LOGGING > 0)
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

	/* Попытка прочитать bacup DRUM */
	if (Read_Backup_DRUM("./drum/drum.bak") != 0)
	{
		/*
		 * Ошибка чтения
		 * Очистить  DRUM
		 */
		// clean_drum();
	}
	else
	{
		/*
		 * Успешное чтение чтение "./drum/drum.bak"
		 */
		// printf("Успешное чтение чтение ./drum/drum.bak\r\n");
	}
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

/* Тип указателя на функцию операции */
typedef int8_t (*operation_func_t)(trs_t addr, trs_t oper);

/* Структура операции */
typedef struct
{
	int8_t code;		   /* Код операции (от -13 до 13) */
	const char *name;	   /* Название операции */
	operation_func_t func; /* Указатель на функцию */
} operation_entry_t;

/* Константы */
#define LUT_SIZE 27 /* 3^3 = 27 возможных комбинаций */
#define MAX_OPERATIONS 39
#define OFFSET_INDEX 13

/* Основные функции */
int8_t execute_trs_lut(trs_t addr, trs_t oper); //
void init_operation_lut(void);
int8_t get_operation_index(int8_t code);

/*------------------------------------------------------------------
 * LUT таблица операций
 *------------------------------------------------------------------
 */
static operation_entry_t operation_lut[LUT_SIZE];
static bool lut_initialized = false;

/*------------------------------------------------------------------
 * Прототипы функций операций
 *------------------------------------------------------------------
 */
static int8_t op_plus00(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plus0plus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plus0minus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plusplus0(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plusplusplus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plusplusminus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plusminus0(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plusminusplus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_plusminusminus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_0plus0(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_0plusplus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_0plusminus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_000(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_00plus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_00minus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_0minus0(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_0minusplus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_0minusminus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_minusplus0(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_minusplusplus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_minusplusminus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_minus00(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_minus0plus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_minus0minus(trs_t addr, trs_t oper __attribute__((unused)));
static int8_t op_error(trs_t addr, trs_t oper);

/** --------------------------
 *  Инициализация LUT
 *  --------------------------
 */
void init_operation_lut(void)
{
	if (lut_initialized)
		return;
	/* Инициализация всех записей как ошибка */
	for (int i = 0; i < LUT_SIZE; i++)
	{
		operation_lut[i].code = -127;
		operation_lut[i].name = "ERROR";
		operation_lut[i].func = op_error;
	}
	/* Заполнение LUT
	 * Коды операций: (+1*9 + 0*3 + 0) = 9
	 */
	operation_lut[9 + OFFSET_INDEX] = (operation_entry_t){9, "+00", op_plus00};
	operation_lut[10 + OFFSET_INDEX] = (operation_entry_t){10, "+0+", op_plus0plus};
	operation_lut[8 + OFFSET_INDEX] = (operation_entry_t){8, "+0-", op_plus0minus};
	operation_lut[12 + OFFSET_INDEX] = (operation_entry_t){12, "++0", op_plusplus0};
	operation_lut[13 + OFFSET_INDEX] = (operation_entry_t){13, "+++", op_plusplusplus};
	operation_lut[11 + OFFSET_INDEX] = (operation_entry_t){11, "++-", op_plusplusminus};
	operation_lut[6 + OFFSET_INDEX] = (operation_entry_t){6, "+-0", op_plusminus0};
	operation_lut[7 + OFFSET_INDEX] = (operation_entry_t){7, "+-+", op_plusminusplus};
	operation_lut[5 + OFFSET_INDEX] = (operation_entry_t){5, "+--", op_plusminusminus};
	operation_lut[3 + OFFSET_INDEX] = (operation_entry_t){3, "0+0", op_0plus0};
	operation_lut[4 + OFFSET_INDEX] = (operation_entry_t){4, "0++", op_0plusplus};
	operation_lut[2 + OFFSET_INDEX] = (operation_entry_t){2, "0+-", op_0plusminus};
	operation_lut[0 + OFFSET_INDEX] = (operation_entry_t){0, "000", op_000};
	operation_lut[1 + OFFSET_INDEX] = (operation_entry_t){1, "00+", op_00plus};
	operation_lut[-1 + OFFSET_INDEX] = (operation_entry_t){-1, "00-", op_00minus};
	operation_lut[-3 + OFFSET_INDEX] = (operation_entry_t){-3, "0-0", op_0minus0};
	operation_lut[-2 + OFFSET_INDEX] = (operation_entry_t){-2, "0-+", op_0minusplus};
	operation_lut[-4 + OFFSET_INDEX] = (operation_entry_t){-4, "0--", op_0minusminus};
	operation_lut[-6 + OFFSET_INDEX] = (operation_entry_t){-6, "-+0", op_minusplus0};
	operation_lut[-5 + OFFSET_INDEX] = (operation_entry_t){-5, "-++", op_minusplusplus};
	operation_lut[-7 + OFFSET_INDEX] = (operation_entry_t){-7, "-+-", op_minusplusminus};
	operation_lut[-9 + OFFSET_INDEX] = (operation_entry_t){-9, "-00", op_minus00};
	operation_lut[-8 + OFFSET_INDEX] = (operation_entry_t){-8, "-0+", op_minus0plus};
	operation_lut[-10 + OFFSET_INDEX] = (operation_entry_t){-10, "-0-", op_minus0minus};
	lut_initialized = true;
}

/** -----------------------------------
 *  Получение индекса по коду операции
 *  -----------------------------------
 */
int8_t get_operation_index(int8_t code)
{
	/*
	 * Маппинг кода на индекс в LUT
	 * Коды: -13..13, но валидные: -10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13
	 * Индекс: 0..26
	 */
	if (code >= -13 && code <= 13)
	{
		return code + 13; /* Смещение для отрицательных чисел */
	}
	return -1; /* Невалидный код */
}

/** ---------------------------------------
 *  Выполнить операцию машины "Сетунь-1958"
 *  ---------------------------------------
 */

/**
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

/** ------------------------------------------------------------------
 *  Основная функция выполнения с LUT
 *  ------------------------------------------------------------------
 */
int8_t execute_trs_lut(trs_t addr, trs_t oper)
{
	/* Инициализация LUT при первом вызове */
	if (!lut_initialized)
	{
		init_operation_lut();
	}
	trs_t fa;
	trs_t k1_5;
	trs_t k6_8;
	int8_t codeoper;
	clear_full(&fa);

	/* Адресная часть */
	k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	/* Код операции */
	k6_8 = oper;
	k6_8.l = 3;
	codeoper = (int8_t) ( get_trit_setun(k6_8, 1) * 9 +
			   			  get_trit_setun(k6_8, 2) * 3 +
			   			  get_trit_setun(k6_8, 3)
						);
	if (LOGGING > 0)
	{
		view_step_new_addres(&k1_5, "A*");
	}

	/* Поиск операции в LUT */
	int8_t index = get_operation_index(codeoper);

	if (index >= 0 && index < LUT_SIZE)
	{
		operation_entry_t entry = operation_lut[index];
		if (LOGGING > 0)
		{
			printf("k6..8[%s]: ", entry.name);
		}
		/* Вызов функции операции */
		int8_t result = entry.func(addr, oper);
		if (LOGGING > 0)
		{
			view_short_regs();
		}
		return result;
	}
	else
	{
		/* Недопустимая команда */
		if (LOGGING > 0)
		{
			printf("k6..8[]: STOP! NO OPERATION\n");
			view_short_reg(&k6_8, "k6..8=");
		}
		return STOP_ERROR;
	}
}

/** ------------------------------
 *  РЕАЛИЗАЦИЯ ОПЕРАЦИЙ
 *  ------------------------------
 */
/* +00 : Посылка в S (A*)=>(S) */
static int8_t op_plus00(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(A*)=>(S)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l); /* очистить неиспользованные триты */
	if (MR.l != 18)
	{
		MR.l = 18;
		MR = shift_trs(MR, 9);
		mod_3_n(&MR, MR.l);
	}
	copy_trs_setun(&MR, &S);
	W = set_trit_setun(W, 1, sgn_trs(S));
	C = next_address(C);
	return OK;
}

/**
 *  +0+ : Сложение в S (S)+(A*)=>(S) 
 */
static int8_t op_plus0plus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(S)+(A*)=>(S)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	if (MR.l != 18)
	{
		MR.l = 18;
		MR = shift_trs(MR, 9);
		mod_3_n(&MR, MR.l);
	}
	S = add_trs(S, MR);
	mod_3_n(&S, S.l);
	W = set_trit_setun(W, 1, sgn_trs(S));
	if (over_check() > 0)
	{
		return STOP_ERROR;
	}
	C = next_address(C);
	return OK;
}

/**
 *  +0- : Вычитание в S (S)-(A*)=>(S) 
 */
static int8_t op_plus0minus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(S)-(A*)=>(S)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	if (MR.l != 18)
	{
		MR = shift_trs(MR, 9);
		MR.l = 18;
		mod_3_n(&MR, MR.l);
	}
	S = sub_trs(S, MR);
	W = set_trit_setun(W, 1, sgn_trs(S));
	if (over_check() > 0)
	{
		return STOP_ERROR;
	}
	C = next_address(C);
	return OK;
}

/**
 *  ++0 : Умножение + (S)=>(R); S=0; (A*)(R)=>(S) 
 */
static int8_t op_plusplus0(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(S)=>(R); S=0; (A*)(R)=>(S)\n");
	copy_trs_setun(&S, &R);
	S.t1 = 0;
	S.t0 = 0;
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	if (MR.l != 18)
	{
		MR = shift_trs(MR, 9);
		MR.l = 18;
		mod_3_n(&MR, MR.l);
	}
	trs_t temp = mul_trs(MR, R);
	mod_3_n(&temp, 18);
	copy_trs_setun(&temp, &S);
	W = set_trit_setun(W, 1, sgn_trs(S));
	if (over_check() > 0)
	{
		return STOP_ERROR;
	}
	C = next_address(C);
	return OK;
}

/**
 *  +++ : Умножение + (S)+(A*)(R)=>(S) 
 */
static int8_t op_plusplusplus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(S)+(A*)(R)=>(S)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	if (MR.l != 18)
	{
		MR = shift_trs(MR, 9);
		MR.l = 18;
		mod_3_n(&MR, MR.l);
	}
	trs_t temp = slice_trs(mul_trs(MR, R), 0, 17);
	S = add_trs(temp, S);
	W = set_trit_setun(W, 1, sgn_trs(S));
	if (over_check() > 0)
	{
		return STOP_ERROR;
	}
	C = next_address(C);
	return OK;
}

/**
 *  ++- : Умножение - (A*)+(S)(R)=>(S) 
 */
static int8_t op_plusplusminus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(A*)+(S)(R)=>(S)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	if (MR.l != 18)
	{
		MR = shift_trs(MR, 9);
		MR.l = 18;
		mod_3_n(&MR, MR.l);
	}
	trs_t temp = mul_trs(S, R);
	S = add_trs(temp, MR);
	W = set_trit_setun(W, 1, sgn_trs(S));
	if (over_check() > 0)
	{
		return STOP_ERROR;
	}
	C = next_address(C);
	return OK;
}

/**
 *  +-0 : Поразрядное умножение (A*)[x](S)=>(S) 
 */
static int8_t op_plusminus0(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(A*)[x](S)=>(S)\n");
	MR = ld_fram(k1_5);
	if (MR.l != 18)
	{
		MR = shift_trs(MR, 9);
		MR.l = 18;
		mod_3_n(&MR, MR.l);
	}
	S = and_trs(MR, S);
	W = set_trit_setun(W, 1, sgn_trs(S));
	C = next_address(C);
	return OK;
}

/**
 *   +-+ : Посылка в R (A*)=>(R) 
 */
static int8_t op_plusminusplus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(A*)=>(R)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	if (MR.l != 18)
	{
		MR = shift_trs(MR, 9);
		MR.l = 18;
		mod_3_n(&MR, MR.l);
	}
	copy_trs_setun(&MR, &R);
	W = set_trit_setun(W, 1, sgn_trs(S));
	C = next_address(C);
	return OK;
}

/**
 *  +-- : Останов (A*)=>(R) 
 */
static int8_t op_plusminusminus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("STOP (A*)=>(R)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	if (MR.l != 18)
	{
		MR = shift_trs(MR, 9);
		MR.l = 18;
		mod_3_n(&MR, MR.l);
	}
	copy_trs_setun(&MR, &R);
	return STOP;
}

/**
 *  0+0 : Условный переход - A*=>(C) при w=0 
 */
static int8_t op_0plus0(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("A*=>(C) при w=0\n");
	int8_t w = sgn_trs(W);
	if (w == 0)
	{
		copy_trs_setun(&k1_5, &C);
	}
	else
	{
		C = next_address(C);
	}
	return OK;
}

/**
 *  0++ : Условный переход - A*=>(C) при w=+1 
 */
static int8_t op_0plusplus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("A*=>(C) при w=+1\n");
	int8_t w = sgn_trs(W);
	if (w > 0)
	{
		copy_trs_setun(&k1_5, &C);
	}
	else
	{
		C = next_address(C);
	}
	return OK;
}

/**
 *  0+- : Условный переход - A*=>(C) при w=-1 
 */
static int8_t op_0plusminus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("A*=>(C) при w=-1\n");
	int8_t w = sgn_trs(W);
	if (w < 0)
	{
		copy_trs_setun(&k1_5, &C);
	}
	else
	{
		C = next_address(C);
	}
	return OK;
}

/**
 *  000 : Безусловный переход A*=>(C) 
 */
static int8_t op_000(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("A*=>(C)\n");
	copy_trs_setun(&k1_5, &C);
	C.l = 5;
	return OK;
}

/**
 *  00+ : Запись из C (C)=>(A*) 
 */
static int8_t op_00plus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(C)=>(A*)\n");
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
	return OK;
}

/**
 *  00- : Запись из F (F)=>(A*) 
 */
static int8_t op_00minus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(F)=>(A*)\n");
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
	return OK;
}

/**
 *  0-0 : Посылка в F (A*)=>(F) 
 */
static int8_t op_0minus0(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(A*)=>(F)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	copy_trs_setun(&MR, &F);
	F = slice_trs_setun(MR, 1, 5);
	F.l = 5;
	W = set_trit_setun(W, 1, sgn_trs(F));
	C = next_address(C);
	return OK;
}

/**
 *  0-+ : Сложение в F c (C) (C)+(A*)=>F 
 */
static int8_t op_0minusplus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(C)+(A*)=>F\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	MR = slice_trs_setun(MR, 1, 5);
	mod_3_n(&MR, 5);
	F = add_trs(C, MR);
	mod_3_n(&F, 5);
	W = set_trit_setun(W, 1, sgn_trs(F));
	C = next_address(C);
	return OK;
}

/**
 *  0-- : Сложение в F (F)+(A*)=>(F) 
 */
static int8_t op_0minusminus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(F)+(A*)=>(F)\n");
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	MR = slice_trs_setun(MR, 1, 5);
	MR.l = 5;
	F = add_trs(F, MR);
	mod_3_n(&F, 5);
	W = set_trit_setun(W, 1, sgn_trs(F));
	C = next_address(C);
	return OK;
}

/**
 *  -+0 : Сдвиг (S) на (A*)=>(S) 
 */
static int8_t op_minusplus0(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("Сдвиг (S) на (A*)=>(S)\n");
	/*
	 * Операция сдвига производит сдвиг содержимого регистра S на N
	 * разрядов, где N рассматривается как 5-разрядный код, хранящийся в
	 * ячейке А*, т. е. N = (А*). Сдвиг производится влево при N > 0 и вправо
	 * при N < 0. При N = 0 содержимое регистра S не изменяется.
	 */
	MR = ld_fram(k1_5);
	mod_3_n(&MR, MR.l);
	S = shift_trs(S, (int8_t)trs2digit(slice_trs_setun(MR, 1, 5)));
	mod_3_n(&S, 18);
	W = set_trit_setun(W, 1, sgn_trs(S));
	C = next_address(C);
	return OK;
}

/**
 *  -++ : Запись из S (S)=>(A*) 
 */
static int8_t op_minusplusplus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(S)=>(A*)\n");
	st_fram(k1_5, S);
	W = set_trit_setun(W, 1, sgn_trs(S));
	C = next_address(C);
	return OK;
}

/**
 *  -+- : Нормализация 
 */
static int8_t op_minusplusminus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("Норм.(S)=>(A*); (N)=>(S)\n");
	/**
	 * Операция нормализации производит сдвиг (S) при (S) != 0 в таком направлении и на такое число
	 * разрядов |N|, чтобы результат, посылаемый в ячейку A*, был но модулю больше 1/2 , но меньше 3/2,
	 * т.е. чтобы в двух старших разрядах результата была записана комбинация троичных цифр 01 или 0-1.
	 * При этом в регистр S посылается число N (5-разрядный код), знак которого определяется
	 * направлением сдвига, а именно: N > 0 при сдвиге вправо и N < 0 при сдвиге влево. При (S) = 0 или при
	 * 1/2 <|(S)| < 3/2 в ячейку А* посылается (S), а в регистр S посылается N = 0.
	 */
	mod_3_n(&S, 18);
	if (S.t0 != 0)
	{
		if (get_trit_setun(S, 1) != 0)
		{
			S = shift_trs(S, -1);
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
		st_fram(k1_5, S);
		S.t1 = 0;
		S.t0 = 0;
	}
	W = set_trit_setun(W, 1, sgn_trs(S));
	C = next_address(C);
	return OK;
}

/**
 *  -00 : Ввод/Вывод
 */
static int8_t op_minus00(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	trs_t fa;
	if (LOGGING > 0)
	{
		printf("Ввод в Фа* - Вывод из Фа*\n");
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
	int codeio = get_trit_setun(k1_5, 2) * 27 +
					get_trit_setun(k1_5, 3) * 9 +
					get_trit_setun(k1_5, 4) * 3 +
					get_trit_setun(k1_5, 5);
	/* Обработка ввода/вывода */
	switch (codeio)
	{
	case (0 * 27 + 0 * 9 + 0 * 3 + 1):
		if (LOGGING > 0)
			printf(" k2..5[000+]: Ввод с ФТ-1 в виде команд\n");
		Read_Commands_from_FT1(ptr1, fa);
		break;
	case (0 * 27 + 0 * 9 + 1 * 3 + 0):
		if (LOGGING > 0)
			printf(" k2..5[00+0]: Ввод с ФТ-2 в виде команд\n");
		Read_Commands_from_FT2(ptr2, fa);
		break;
	case (0 * 27 + 0 * 9 + 0 * 3 - 1):
		if (LOGGING > 0)
			printf(" k2..5[000-]: Ввод с ФТ-1 в виде символов\n");
		Read_Symbols_from_FT1(ptr1, fa);
		break;
	case (0 * 27 + 0 * 9 - 1 * 3 + 0):
		if (LOGGING > 0)
			printf(" k2..5[00-0]: Ввод с ФТ-2 в виде символов\n");
		Read_Symbols_from_FT2(ptr2, fa);
		break;
	case (1 * 27 + 0 * 9 + 0 * 3 + 0):
		if (LOGGING > 0)
			printf(" k2..5[+000]: Перфорация троичных кодов на Перфоратор ПЛ\n");
		for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
		{
			MR = ld_fram(fa);
			trs_t symb;
			symb.l = 2;
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
	case (-1 * 27 + 0 * 9 + 0 * 3 + 0):
		if (LOGGING > 0)
			printf(" k2..5[-000]: Перфорация троичных символов на Перфоратор ПЛ\n");
		for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
		{
			MR = ld_fram(fa);
			trs_t symb;
			symb.l = 3;
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
	case (0 * 27 + 1 * 9 + 0 * 3 + 0):
		if (LOGGING > 0)
			printf(" k2..5[0+00]: Печать в виде команд на Пишущей машинке\n");
		{
			uint8_t current = letter_number_sw;
			letter_number_sw = 1;
			for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
			{
				MR = ld_fram(fa);
				trs_t symb;
				symb.l = 3;
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
			letter_number_sw = current;
		}
		break;
	case (0 * 27 - 1 * 9 + 0 * 3 + 0):
		if (LOGGING > 0)
			printf(" k2..5[0-00]: Печать одним цветом в виде символов\n");
		for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
		{
			MR = ld_fram(fa);
			trs_t symb;
			symb.l = 3;
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
	case (1 * 27 - 1 * 9 + 0 * 3 + 0):
		if (LOGGING > 0)
			printf(" k2..5[+-00]: Печать в виде символов на Пишущей машинке\n");
		for (uint8_t i = 0; i < SIZE_ZONE_TRIT_FRAM; i++)
		{
			MR = ld_fram(fa);
			trs_t symb;
			symb.l = 3;
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
	default:
		/* Ошибка: Не поддерживается устройвтво ввода/вывода */
		return STOP_ERROR;
	}
	if (LOGGING > 0)
		printf("\n");
	C = next_address(C);
	return OK;
}

/**
 *  -0+ : Запись на МБ 
 */
static int8_t op_minus0plus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(Фа*)=>(Мд*)\n");
	int32_t zone = trs2digit(slice_trs_setun(k1_5, 2, 5));
	if ((zone < ZONE_DRUM_BEG) || (zone > ZONE_DRUM_END))
	{
		return STOP_ERROR_MB_NUMBER;
	}
	else
	{
		fram_to_drum(slice_trs_setun(k1_5, 1, 5));
		MB = slice_trs_setun(k1_5, 2, 5);
		mod_3_n(&MB, 4);
	}
	C = next_address(C);
	return OK;
}

/**
 *  -0- : Считывание с МБ 
 */
static int8_t op_minus0minus(trs_t addr, trs_t oper __attribute__((unused)))
{
	trs_t k1_5 = slice_trs_setun(addr, 1, 5);
	k1_5.l = 5;
	if (LOGGING > 0)
		printf("(Мд*)=>(Фа*)\n");
	int32_t zone = trs2digit(slice_trs_setun(k1_5, 2, 5));
	if ((zone < ZONE_DRUM_BEG) || (zone > ZONE_DRUM_END))
	{
		clean_fram_zone(slice_trs_setun(k1_5, 1, 1));
	}
	else
	{
		drum_to_fram(slice_trs_setun(k1_5, 1, 5));
	}
	MB = slice_trs_setun(k1_5, 2, 5);
	mod_3_n(&MB, 4);
	C = next_address(C);
	return OK;
}

/**
 *  ERROR Ошибка операции 
 */
static int8_t op_error(trs_t addr __attribute__((unused)), trs_t oper)
{
	trs_t k6_8 = oper;
	k6_8.l = 3;
	if (LOGGING > 0)
	{
		printf("STOP BREAK\n");
		view_short_reg(&k6_8, "k6..8=");
	}
	return STOP_ERROR;
}

/**
 *  Читать файл
 */
const char *get_file_ext(const char *filename)
{
	const char *dot = strrchr(filename, '.');
	if (!dot || dot == filename)
		return "";
	return dot + 1;
}

/**
 *  Копировать файл
 */
int copy_file(char *file_scr_sst, char *file_dst_sst)
{
	FILE *ptr_src;
	FILE *ptr_dst;

	int a;

	ptr_dst = fopen(file_dst_sst, "wb");
	if (ptr_dst == NULL)
	{
		return -1;
	}

	ptr_src = fopen(file_scr_sst, "rb");
	if (ptr_src == NULL)
	{
		fclose(ptr_dst);
		return -1;
	}

	while (1)
	{
		a = fgetc(ptr_src);

		if (!feof(ptr_src))
			fputc(a, ptr_dst);
		else
			break;
	}

	fclose(ptr_src);
	fclose(ptr_dst);

	return 0;
}

/**
 *  Копировать скрипт файл
 */
void CopyFileScript(char *path_lst __attribute__((unused)), char *pathfile1, char *pathfile2)
{
	printf("Script file source: %s\r\n", pathfile1);

	printf("Script file destation: %s\r\n", pathfile2);

	if (copy_file(pathfile1, pathfile2) == 0)
		printf("Copy file source to file destation.\r\n");
	else
		fprintf(stderr, "ERROR Copy!\r\n");
}

/**
 *  Загрущить файл исходных кодов распечатки программы 
 */
void LoadFileListToPaperTxt(char *pathcataloglst, char *pathfilelst, char *pathfiletxt)
{
	FILE *file_lst;
	FILE *file_txt;

	/* Чтение списка файлов */
	char str[1024] = {0};
	size_t len; 	/* Текущий размер буфера (0 означает, что память будет выделяться динамически) */
	ssize_t read;	/* Возвращаемое значение getline */

	/* Открываем выходной файл */
	file_txt = fopen(pathfiletxt, "w");
	if (file_txt == NULL)
	{
		printf("ERROR fopen %s\r\n", pathfiletxt);
		return;
	}

	/* Открываем файл со списком */
	file_lst = fopen(pathfilelst, "r");
	if (file_lst == NULL)
	{
		printf("ERROR fopen %s\r\n", pathfilelst);
		fclose(file_txt);
		return;
	}

	printf("Read file list: %s\r\n", pathfilelst);
	printf("\r\n");

	while (1)
	{
		/*
		 * Вызываем getline. Она может выделить буфер сама (если line == NULL и len == 0)
		 * или использовать уже выделенный буфер, если он достаточно большой
		 */
		char *line = NULL; // Указатель на буфер (инициализируем NULL)
		read = getline(&line, &len, file_lst);

		/* Проверяем результат */
		if (read != -1)
		{
			/*
			 * Выводим количество прочитанных символов (включая символ новой строки)
			 */ 
			 //viv- dbg  printf("Прочитано %zd символов: %s", read, line);
			  
		}
		else
		{
			/*
			 *  Если getline вернула -1, проверяем, была ли это ошибка или конец файла
			 */ 
			if (read == 0)
			{
				//viv- dbg  printf("Достигнут конец файла или строка пустая.\r\n");
				free(line);
				break;
			}
			else
			{
				//viv- dbg  printf("Ошибка при чтении строки\r\n");
				free(line);
				break;
			}
		}
		memcpy(str, line, (size_t)read);
		free(line); /* освободить память */

		//viv- dbg  printf("Read file: %s\r\n", line);

		/* Удаляем символ новой строки */
		len = strlen(str);
		if (len > 0 && str[len - 1] == '\n')
		{
			str[len - 1] = '\0';
			len--; /* Обновляем длину после удаления \n */
		}

		/* Формируем полный путь */
		char path_str[160] = {0};

		/* Безопасное копирование пути */
		size_t cat_len = strlen(pathcataloglst);
		if (cat_len + 1 < sizeof(path_str)) // +1 для '/'
		{
			strcpy(path_str, pathcataloglst);
			strcat(path_str, "/");
		}
		else
		{
			fprintf(stderr, "Ошибка: путь каталога слишком длинный\n");
			continue;
		}

		/* Безопасное добавление имени файла */
		size_t pos = strlen(path_str);
		if (pos + len + 1 <= sizeof(path_str)) /* +1 для null-терминатора */
		{
			memcpy(path_str + pos, str, len + 1);
		}
		else
		{
			fprintf(stderr, "Ошибка: переполнение буфера path_str\n");
			continue;
		}

		/* Открываем файл с командами */
		FILE *file = fopen(path_str, "r");
		if (file == NULL)
		{
			printf("ERROR fopen %s\r\n", path_str);
			continue; /* Не прерываем функцию */
		}

		/* Обработка команд */
		char cmd[20] = {0};
		trs_t inr, dst, sum, tmp, trit2;
		uint8_t offset4_7 = 4;

		clear_full(&inr);
		clear_full(&sum);
		clear_full(&tmp);
		clear_full(&dst);
		clear_full(&MR);
		clear_full(&trit2);

		inr = smtr("0---0");
		tmp.l = 18;
		sum.l = 18;
		dst.l = 9;
		MR.l = 18;
		trit2.l = 2;

		trit2 = set_trit_setun(trit2, 1, 0);

		while (fscanf(file, "%19s", cmd) == 1) // Лучше проверять на 1
		{
			//viv- dbg  printf(" cmd: %s\r\n", cmd);

			cmd_str_2_trs(cmd, &dst);

			trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 1));
			fwrite(tab4_7[trs2digit(trit2) + offset4_7], 1, STRING_PAPER_SIZE, file_txt);
			fwrite("\r\n", 1, 2, file_txt);

			trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 2));
			trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 3));
			fwrite(tab4_7[trs2digit(trit2) + offset4_7], 1, STRING_PAPER_SIZE, file_txt);
			fwrite("\r\n", 1, 2, file_txt);

			trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 4));
			trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 5));
			fwrite(tab4_7[trs2digit(trit2) + offset4_7], 1, STRING_PAPER_SIZE, file_txt);
			fwrite("\r\n", 1, 2, file_txt);

			trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 6));
			trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 7));
			fwrite(tab4_7[trs2digit(trit2) + offset4_7], 1, STRING_PAPER_SIZE, file_txt);
			fwrite("\r\n", 1, 2, file_txt);

			trit2 = set_trit_setun(trit2, 1, get_trit_setun(dst, 8));
			trit2 = set_trit_setun(trit2, 2, get_trit_setun(dst, 9));
			fwrite(tab4_7[trs2digit(trit2) + offset4_7], 1, STRING_PAPER_SIZE, file_txt);
			fwrite("\r\n", 1, 2, file_txt);

			fwrite("O__._O", 1, STRING_PAPER_SIZE, file_txt);
			fwrite("\r\n", 1, 2, file_txt);
		}

		fclose(file);

		/* Промежуток между зонами на перфоленте */
		for (int j = 0; j < 5; j++)
		{
			fwrite("___.__\r\n", 1, 7, file_txt); // strlen("___.__\r\n") = 7
		}
	}

	fclose(file_lst);
	fclose(file_txt);

	printf("Write file: %s\r\n", pathfiletxt);
}

/**
 *  Convert software files to file paper.txt
 */ 
int ConvertSWtoPaper(char *path_lst, char *path_txt)
{

	int res = 0;

	char a_fileName[1024];

	printf("[ Convert software files to file paper.txt ]\r\n");
	printf("\r\n");

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

	/* Прочитать файлы из каталога и преобразовать в paper.txt */
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
		char pathfile1[1024];
		char pathfile2[1024];
		/* clear */
		memset(pathfile1, 0, sizeof(pathfile1));
		memset(pathfile2, 0, sizeof(pathfile2));
		/* path file1 */
		strcat(pathfile1, path_lst);
		strcat(pathfile1, "/");
		strcat(pathfile1, a_fileName);
		/* path file2 */
		strcat(pathfile2, path_txt);
		strcat(pathfile2, "/");
		strcat(pathfile2, "paper.txt");

		LoadFileListToPaperTxt(path_lst, pathfile1, pathfile2);

		/* clear */
		memset(pathfile1, 0, sizeof(pathfile1));
		memset(pathfile2, 0, sizeof(pathfile2));

		/* path file1 */
		strcat(pathfile1, path_lst);
		strcat(pathfile1, "/");
		strcat(pathfile1, "script.sst");
		/* path file2 */
		strcat(pathfile2, "./script");
		strcat(pathfile2, "/");
		strcat(pathfile2, "script.sst");

		CopyFileScript(path_lst, pathfile1, pathfile2);
	}

	return res; /* Ok' */
}

/** -----------------------------------------------
 *  1. Ввод и вывод бинарного файла троичных данных
 *  -----------------------------------------------
 */
int DumpFileTxs(char *pathfiletxs)
{
	FILE *file_txs;

	file_txs = fopen(pathfiletxs, "r");
	if (file_txs == NULL)
	{
		printf("ERROR fopen %s\r\n", pathfiletxs);
		return 1;
	}

	printf("Read commands from file.txs: %s\r\n", pathfiletxs);

	char cmd[20];
	trs_t inr;
	trs_t dst;

	inr = smtr("0---0"); /* cчетчик адреса коротких слов */
	trs_t sum;
	trs_t tmp;

	clear_full(&tmp);
	tmp.l = 18;
	tmp.t1 = 0;
	tmp.t0 = 0;

	int64_t dsun = 0;
	sum.l = 18;
	sum.t1 = 0;
	sum.t0 = 0;

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

/**
 *  Парсер
 */
int parser_user(char *cmd, char fields[][80], char ch)
{
	int ind = 0;
	char *p1 = cmd;
	char *p2 = cmd;
	char *pend = cmd + strlen(cmd);		   /* Используем strlen для определения конца строки */
	size_t field_size = sizeof(fields[0]); /* Размер одной строки (80 байт) */

	/* Проверка на NULL */
	if (!cmd || !fields)
	{
		return -1;
	}

	/* Очищаем все поля перед использованием */
	for (int i = 0; i < 6; i++)
	{
		memset(fields[i], 0, field_size);
	}

	/* Если строка пустая */
	if (*cmd == '\0')
	{
		return 0;
	}

	/* Разбиваем строку на поля */
	while (p2 <= pend && ind < 6)
	{
		if (*p2 == ch || *p2 == '\0')
		{
			/* Копируем поле */
			size_t len = (size_t)(p2 - p1);
			if (len > 0 && len < field_size - 1)
			{
				memcpy(fields[ind], p1, len);
				fields[ind][len] = '\0'; /* Добавляем нуль-терминатор */
			}
			ind++;
			p1 = p2 + 1; /* Пропускаем разделитель */
		}
		p2++;
	}

	return ind; /* Возвращаем количество найденных полей */
}

/**
 * Выполнить скрипт-файл './script/scrint.sst'.
 * Вывод сообщения и продолжить с заданного адреса в
 * ферритовой памяти FRAM.
 *
 * Пример останова машины, если при чтение виртуальной перфоленты
 * не совпала с контрольной суммы:
 *  +----------------+-----------+--------------------------+----------------------+---------------+-----------------------------+
 *  | Field 1        | Field 2   | Field 3                  |Field 4               | Field 5       | Field 6                     |
 *  +----------------+-----------+--------------------------+----------------------+---------------+-----------------------------+
 *  | 0Z0            |,          |,'Error CRC input paper.' |,S                    |,              |,<LF><CR> или <CR>           |
 *  +----------------+-----------+--------------------------+----------------------+---------------+-----------------------------+
 *  | Адрес останова | Команда   | Коммантарий              | Условие продолежения | Адрес запуска | Терминальные символы строки |
 *  +----------------+-----------+--------------------------+----------------------+---------------+-----------------------------+
 */
void view_next_start(void)
{
	size_t i = 0;
	long int res = 0;

	/* Файл-скрипт для исполнения  */
	FILE *script_txt;

	script_txt = fopen("./script/script.sst", "r");
	if (script_txt == NULL)
	{ /*
	  + Нет файла. Продолжить работу.
	  */
		printf("ERROR: fopen('./script/script.sst', 'r')\r\n");
		return;
	}

	fseek(script_txt, 0L, SEEK_END);

	/* Calculating the size of the file  */
	res = ftell(script_txt);

	if (res == 0)
	{

		printf("res = ftell(script_txt)\r\n");

		/* Закрыть файл */
		fclose(script_txt);
		return;
	}

	printf("\r\n[ Script file: 'script.sst' ]\r\n");

	fseek(script_txt, 0L, SEEK_SET);

	/* Массив с полями строки скрипта */

	char fields[5][80] = {0};
	char cmd[5 * 80] = {0};
	size_t lenfld = sizeof(fields) / sizeof(fields[0]);
	/* Строка команды из скрипт-файла */

	int n = 0;
	/* Пока не дойдет до конца, считываем */

	while ((fgets(cmd, 5 * 80, script_txt)) != NULL)
	{
		/* Массив строк после разбора */
		char token_array[80] = {0};
		char *token = (char *)token_array;

		if (lenfld > 0) /* массив доступен ? */
		{
			size_t j = 0;			  /* счетчик полей разбора */
			token = strtok(cmd, ","); /* парсинг */
			memset(fields[j], 0, sizeof(token_array));
			memcpy(fields[j], token, strlen(token));
			j++;	  /* следующее поле */
			while (1) /* цикл */
			{
				token = strtok(NULL, ","); /* парсинг */
				if (token == NULL)
				{ /* Завершить парсинг */
					break;
				}
				memset(fields[j], 0, sizeof(token_array));
				memcpy(fields[j], token, strlen(token));
				j++; /* следующее поле */
				if (j > lenfld)
				{ /* Завершить парсинг */
					break;
				}
			}
		}

		/*  */
		//viv- dbg  printf("n=%d",n);
		trs_t C_cond;
		/* Адрес С остонова машины для проверки */
		C_cond.l = 6;
		C_cond.t0 = 0;
		C_cond.t1 = 0;
		/* Адрес для старта машины при условии fields[3]=R */
		C_new.l = 6;
		C_new.t0 = 0;
		C_new.t1 = 0;

		/*
		 * Символ действия
		 * S = STOP
		 * R = RUN
		 */
		action_post = 'S';

		/* Цикл проверки строки из скрипта-файла */
		for (i = 0; i < lenfld; i++)
		{
			//viv- dbg   printf("\t%d: %s\r\n", i, fields[i]);

			switch (i)
			{

			case 0:
			{

				regC_str_2_trs(fields[0], &C_cond);

				int sing = cmp_trs(C, C_cond);
				if (sing == 0)
				{
					view_short_reg(&C_cond, "  Addres");
					printf("  Note:   %s\r\n", fields[2]);
				}
			}
			break;
			case 3:
			{
				regC_str_2_trs(fields[3], &C_new);
			};
			break;
			case 4:
			{
				sscanf(fields[4], "%c", &action_post);
			};
			break;
			default:
			{
			}
			}
		}

		n++;
	}

	char cond_symb;
	if (action_post == 'S')
	{
		cond_symb = 'S';
	}
	else if (action_post == 'R')
	{
		view_short_reg(&C_new, "  New address");
		cond_symb = 'R';
	}
	else
	{
		cond_symb = 'S';
	}
	printf("  Action: %c\r\n", cond_symb);

	/* Закрыть файл */
	fclose(script_txt);
}

/** 
 *  Печать версии
 */
void print_version(void)
{
	printf(" Emulator ternary computer 'Setun-1958':\r\n");
	printf(" Version: %s\r\n", Version);
	printf(" Author:  Vladimir V.I.\r\n");
	printf(" E-mail:  askfind@ya.ru\r\n");
	printf("\r\n");
}

/** 
 *  Печать описание виртуальной машины
 */
int version(const char __attribute__((unused)) *argv0)
{
	printf(" Emulator ternary computer 'Setun-1958': ver.%s\r\n", Version);
	printf(" Author: Vladimir V.I.\r\n");
	printf(" E-mail: askfind@ya.ru\r\n");
	printf("\r\n");

	fflush(stdout);

	return 0;
}

/** 
 *  Печать подсказка для оператора машины
 */
int usage(const char *argv0)
{
	printf("usage: %s [options] FILE SCRIPT(s)...\r\n", argv0);
	printf("\t--version : version software setun1958emu\r\n");
	printf("\t--load : load software setun1958emu\r\n");
	printf("\t--convert : convert software file.lst to paper.txt setun1958emu\r\n");
	printf("\t--dump : dump zone from file.txs setun1958emu\r\n");
	printf("\t--LOGGING : view step  setun1958emu\r\n");
	printf("\t--breakpoint : view stop setun1958emu\r\n");
	printf("\t--test : number test setun1958emu\r\n");
	printf("\r\n");

	fflush(stdout);

	return 0;
}

/** 
 *  Открыть файл с виртуальной перфолентой машины "Сетунь"
 */
void Emu_Open_Files_ptr1_ptr2(void)
{
	/* Открыть файлы для виртуальных устройств	*/
	ptr1 = fopen("ptr1/paper.txt", "w");
	if (ptr1 == NULL)
	{
		printf("ERROR fopen 'ptr1/paper.txt'\r\n");
	}

	ptr2 = fopen("ptr2/paper.txt", "w");
	if (ptr2 == NULL)
	{
		printf("ERROR fopen 'ptr1/paper.txt'\r\n");
	}
}

/** 
 *  Закрыть файлы с виртуальной перфолентой машины "Сетунь"
 */
void Emu_Close_Files_ptr1_ptr2(void)
{
	/* Закрыть файлы виртуальных устройств */
	fclose(ptr1);
	fclose(ptr2);
}

/** 
 *  Открыть файлы с виртуальными устройстваит ввода и вывода
 */
void Emu_Open_Files(void)
{
	/* Открыть файлы для виртуальных устройств	*/
	ptr1 = fopen("ptr1/paper.txt", "r");
	if (ptr1 == NULL)
	{
		printf("ERROR fopen 'ptr1/paper.txt'\r\n");
	}

	ptr2 = fopen("ptr2/paper.txt", "r");
	if (ptr2 == NULL)
	{
		printf("ERROR fopen 'ptr1/paper.txt'\r\n");
	}

	ptp1 = fopen("ptp1/paper.txt", "w");
	if (ptp1 == NULL)
	{
		printf("ERROR fopen 'ptp1/paper.txt'\r\n");
	}

	tty1 = fopen("tty1/printout.txt", "w");
	if (tty1 == NULL)
	{
		printf("ERROR fopen 'tty1/printout.txt'\r\n");
	}
}

/** 
 *  Закрыть файлы с виртуальными устройстваит ввода и вывода
 */
void Emu_Close_Files(void)
{
	/* Закрыть файлы виртуальных устройств */
	fclose(ptr1);
	fclose(ptr2);
	fclose(ptp1);
	fclose(tty1);
}

/** 
 *  Эмуляция кнопки "ПУСК" машины
 */
void Emu_Begin(void)
{
	/* Инициализация таблиц символов ввода и вывода "Сетунь-1958" */
	init_tab4();

	/* В режиме отладки вывод регистров */
	if (LOGGING > 0)
	{
		view_short_regs();
	}

	Emu_Open_Files();

	/*  Выполнить первый код "Сетунь-1958" */
	printf("\r\n[ Start Setun-1958 ]\r\n");

	/* Выполение программы в ферритовой памяти "Сетунь-1958" */
	Begin_Read_Commands_from_FT1(ptr1);

	/* Begin address fram */
	C = smtr("0000+");
}

/** 
 *  Эмуляция кнопки "ШАГ" машины
 */
int Emu_Step(void)
{
	trs_t addr;
	trs_t C_cur;
	trs_t oper;

	clear_full(&C_cur);
	C_cur = C;
	K = ld_fram(C);
	K = slice_trs_setun(K, 1, 9);

	if (LOGGING > 0)
	{
		view_step_short_reg(&C, "\n С");
	}

	addr = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);

	return execute_trs_lut(addr, oper);
}

/** 
 *  Эмуляция кнопки "СТОП" машины
 */
void Emu_Stop(void)
{
	/*
	 dump_fram_zone(smtr("-"));
	 dump_fram_zone(smtr("0"));
	 dump_fram_zone(smtr("+"));
	 */

	/* Prints REGS */
	view_short_regs();
	printf("\r\n");

	/* Prints REGS and FRAM */
	if (LOGGING > 0)
	{
		/*
		 dump_fram_zone(smtr("-"));
		 dump_fram_zone(smtr("0"));
		 dump_fram_zone(smtr("+"));
		*/
	}

	/* Печать завершения работы "Setun-1958" */
	if (STEP == 0)
	{
		/* Новое состояние */
		emu_stat = PAUSE_EMU_ST;

		printf("\r\n[ Stop Setun-1958 ]\r\n");
	}
	else
	{
		printf("\r\n[ Step = %d : Stop Setun-1958 ]\r\n", STEP);
	}

	/* Закрыть файлы виртуальных устройств */
	Emu_Close_Files();
}

/** 
 *  Эмуляция работы машины
 */
int Process_Work_Emulation(void)
{

	/* ------------------------------
	 *  Переменные emulator Setun-1958
	 */
	int ret_exec = 0;

	/* Состояние после включения питания */
	if (emu_stat == NOREADY_EMU_ST)
	{

		Emu_Open_Files();

		/* Инициализация таблиц символов ввода и вывода "Сетунь-1958" */
		init_tab4();

		/* Новое состоняие */
		emu_stat = WAIT_EMU_ST;
	}

	if (emu_stat == BEGIN_EMU_ST)
	{
		/* Начальный старт */
		Emu_Begin();

		/* Новое состоняие */
		emu_stat = LOOP_WORK_EMU_ST;
	}

	if (emu_stat == STEP_EMU_ST)
	{
		ret_exec = Emu_Step();

		if ((ret_exec == STOP))
		{
			printf("\r\n<STOP>\r\n");
			emu_stat = STOP_EMU_ST;
		}
		else if (ret_exec == STOP_OVER)
		{
			printf("\r\n<STOP_OVER>\r\n");
			emu_stat = STOP_EMU_ST;
		}
		else if (ret_exec == STOP_ERROR)
		{
			printf("\r\nERR#:%i<STOP_ERROR>\r\n", ret_exec);
			emu_stat = ERROR_EMU_ST;
		}
		else if (ret_exec == STOP_ERROR_MB_NUMBER)
		{
			printf("\r\nERR#:%i<STOP_ERROR_MB_NUMBER>\r\n", ret_exec);
			cli_prompt();
			emu_stat = ERROR_MB_NUMBER_EMU_ST;
		}

		counter_step++;
		if (STEP > 0)
			STEP--;
		if (STEP == 0)
		{
			/* Новое состояние */
			emu_stat = PAUSE_EMU_ST;
		}

		if (BREAKPOINT == trs2digit(C) && (BREAKPOINT <= INT32_MAX))
		{
			emu_stat = PAUSE_EMU_ST;
		}

		if (BREAKPOINT_MB == trs2digit(MB) && (BREAKPOINT_MB <= INT32_MAX))
		{
			emu_stat = PAUSE_EMU_ST;
		}

		// viv+ test  view_short_regs();

		/* Новое состояние */
		emu_stat = WAIT_EMU_ST;
	}

	if (emu_stat == PAUSE_EMU_ST)
	{
		emu_stat = WAIT_EMU_ST;
	}

	/* Состояние готов к работе */
	if (emu_stat == LOOP_WORK_EMU_ST)
	{

		if (BREAKPOINT_TRS.l != 0)
		{
			if (BREAKPOINT == trs2digit(C) && (BREAKPOINT <= INT32_MAX))
			{
				view_short_regs();
				view_short_reg(&BREAKPOINT_TRS, "\r\n[ BREAK ] ");
				emu_stat = PAUSE_EMU_ST;
			}
		}

		if (BREAKPOINT_MB_TRS.l != 0)
		{
			if (BREAKPOINT_MB == trs2digit(MB) && (BREAKPOINT_MB <= INT32_MAX))
			{
				view_short_regs();
				view_short_reg(&BREAKPOINT_MB_TRS, "\r\n[ BREAK ADDR DRUM] ");
				emu_stat = PAUSE_EMU_ST;
			}
		}

		ret_exec = Emu_Step();

		counter_step++;

		if ((ret_exec == STOP))
		{
			printf("\r\n<STOP>\r\n");
			emu_stat = STOP_EMU_ST;
			/*
			break;
			*/

			view_next_start();
		}
		else if (ret_exec == STOP_OVER)
		{
			printf("\r\n<STOP_OVER>\r\n");
			emu_stat = STOP_EMU_ST;
			/*
			 break;
			 */

			view_next_start();
		}
		else if (ret_exec == STOP_ERROR)
		{
			printf("\r\nERR#:%i<STOP_ERROR>\r\n", ret_exec);
			emu_stat = ERROR_EMU_ST;
			/*
			 break;
			*/

			view_next_start();
		}
		else if (ret_exec == STOP_ERROR_MB_NUMBER)
		{
			printf("\r\nERR#:%i<STOP_ERROR_MB_NUMBER>\r\n", ret_exec);
			cli_prompt();
			emu_stat = ERROR_MB_NUMBER_EMU_ST;
			/*
			 break;
			*/

			view_next_start();
		}
	}

	/* Состояние  */
	if (emu_stat == STOP_EMU_ST)
	{
		emu_stat = CLI_WELCOM_EMU_ST;
	} /* if(...) */

	/* Состояние  */
	if (emu_stat == ERROR_EMU_ST)
	{
		emu_stat = CLI_WELCOM_EMU_ST;
	} /* if(...) */

	/* Состояние  */
	if (emu_stat == ERROR_MB_NUMBER_EMU_ST)
	{
		emu_stat = CLI_WELCOM_EMU_ST;
	} /* if(...) */

	/* Состояние  */
	if (emu_stat == CLI_WELCOM_EMU_ST)
	{
		/* Prints REGS */
		view_short_regs();
		printf("\r\n");

		cli_prompt();
		emu_stat = WAIT_EMU_ST;
	} /* if(...) */

	/* Состояние  */
	if (emu_stat == WAIT_EMU_ST)
	{

	} /* if(...) */

	return 0;
}

/** ------------------------------------------------------------------------
 *  Командный интепретатор виртуального пульта управления МЦВМ "Сетунь" 1958
 */

/** --------------------------------------------------------
 *  CLI командный интерпретатор (порт с C++ Command class)
 */
static void help_print(void)
{
	printf(" Commands control for setun1958emu:\r\n");
	printf(" dump     [arglist]\r\n");
	printf(" load     [l]    [arglist]\r\n");
	printf(" prn      [p]    [arglist]\r\n");
	printf(" debug    [d]    [arglist]\r\n");
	printf(" view     [v]\r\n");
	printf(" begin    [b]\r\n");
	printf(" pause    [p]\r\n");
	printf(" run      [r]\r\n");
	printf(" step     [s]    [arglist] \r\n");
	printf(" break    [br]   [arglist]\r\n");
	printf(" breakmb  [brm]  [arglist]\r\n");
	printf(" reg      [rg]   [arglist]\r\n");
	printf(" fram     [fr]   [arglist]\r\n");
	printf(" drum     [dr]   [arglist]\r\n");
	printf(" help     [h]\r\n");
	printf(" quit     [q]\r\n");
	printf(" calc     [arglist]\r\n");
}

static void prn_cmd(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("prn: ERR#1\r\n");
		return;
	}
	int par2_numb = 0;
	sscanf(argv[1], "%i", &par2_numb);
	if (par2_numb > 0)
	{
		russian_latin_sw = 1; /* Печать LAT */
		printf("switch print 'lat'\r\n");
	}
	else
	{
		russian_latin_sw = 0; /* Печать RUS */
		printf("switch print 'rus'\r\n");
	}
}

static void dump_cmd(int argc, char *argv[])
{
	if (argc > 2)
	{
		printf("dbg: ERR#1\r\n");
		return;
	}
	char *arg = (argc > 1) ? argv[1] : "";
	if (DumpFileTxs(arg) != 0)
		printf("no path file\r\n");
}

static void load_cmd(int argc, char *argv[])
{
	if (argc > 3)
	{
		printf("ERROR load_cmd!\r\n");
		return;
	}
	char *a1 = (argc > 1) ? argv[1] : "";
	char *a2 = (argc > 2) ? argv[2] : "";
	if (ConvertSWtoPaper(a1, a2) != 0)
		printf("no path file\r\n");
}

static void debug_cmd(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("dbg: ERR#1\r\n");
		return;
	}
	int par2_numb = 0;
	sscanf(argv[1], "%i", &par2_numb);
	if (par2_numb > 0)
	{
		LOGGING = 1;
		printf("switch debug on\r\n");
	}
	else
	{
		LOGGING = 0;
		printf("switch debug off\r\n");
	}
}

static void begin_cmd(int argc, char __attribute__((unused)) * argv[])
{
	if (argc > 1)
	{
		printf("ERROR begin_cmd!\r\n");
		return;
	}
	counter_step = 0;
	emu_stat = BEGIN_EMU_ST;
}

static void pause_cmd(int argc, char __attribute__((unused)) * argv[])
{
	if (argc > 1)
	{
		printf("ERROR pause_cmd!\r\n");
		return;
	}
	emu_stat = PAUSE_EMU_ST;
}

static void run_cmd(int argc, char __attribute__((unused)) * argv[])
{
	if (argc > 1)
	{
		printf("ERROR run_cmd!\r\n");
		return;
	}
	emu_stat = LOOP_WORK_EMU_ST;
}

static void step_cmd(int argc, char *argv[])
{
	if (argc > 2)
	{
		printf("ERROR step_cmd!\r\n");
		return;
	}
	if (argc == 2)
		sscanf(argv[1], "%ul", &STEP);
	else
		STEP = 0;
	emu_stat = STEP_EMU_ST;
}

static void break_cmd(int argc, char *argv[])
{
	if (argc > 2)
	{
		printf("ERROR break_cmd!\r\n");
		return;
	}
	if (argc == 1)
	{
		BREAKPOINT = 0;
		BREAKPOINT_TRS.l = 0;
	}
	else
	{
		BREAKPOINT = trs2digit(smtr(argv[1]));
		BREAKPOINT_TRS = smtr(argv[1]);
	}
}

static void break_drum_cmd(int argc, char *argv[])
{
	if (argc > 2)
	{
		printf("ERROR break_drum_cmd!\r\n");
		return;
	}
	if (argc == 1)
	{
		BREAKPOINT_MB = 0;
		BREAKPOINT_MB_TRS.l = 0;
	}
	else
	{
		BREAKPOINT_MB = trs2digit(smtr(argv[1]));
		BREAKPOINT_MB_TRS = smtr(argv[1]);
	}
}

static void reg_cmd(int argc, char *argv[])
{
	if (argc < 2 || argc > 3)
	{
		printf("ERROR reg_cmd!\r\n");
		return;
	}
	int rg = toupper(argv[1][0]);
	trs_t trs = (argc > 2) ? smtr(argv[2]) : smtr("");
	switch (rg)
	{
	case 'K':
		copy_trs(&trs, &K);
		break;
	case 'F':
		copy_trs(&trs, &F);
		break;
	case 'C':
		copy_trs(&trs, &C);
		break;
	case 'W':
		copy_trs(&trs, &W);
		break;
	case 'S':
		copy_trs(&trs, &S);
		break;
	case 'R':
		copy_trs(&trs, &R);
		break;
	case 'M':
		copy_trs(&trs, &MB);
		break;
	default:
		break;
	}
}

static void view_cmd(int argc, char __attribute__((unused)) * argv[])
{
	if (argc > 1)
	{
		printf("ERROR view_cmd!\r\n");
		return;
	}
	printf("[ View registers Setun-1958 ]\r\n");
	view_short_regs();
	printf("\r\n");
	printf("[ Tools ]\r\n");

	switch (emu_stat)
	{
	case NOREADY_EMU_ST:
		printf("  status: noready\r\n");
		break;
	case BEGIN_EMU_ST:
		printf("  status: begin\r\n");
		break;
	case LOOP_WORK_EMU_ST:
		printf("  status: loop\r\n");
		break;
	case STEP_EMU_ST:
		printf("  status: step\r\n");
		break;
	case ERROR_EMU_ST:
		printf("  status: error\r\n");
		break;
	case ERROR_MB_NUMBER_EMU_ST:
		printf("  status: error drum\r\n");
		break;
	case CLI_WELCOM_EMU_ST:
		printf("  status: welcom\r\n");
		break;
	case WAIT_EMU_ST:
		printf("  status: wait\r\n");
		break;
	default:
		break;
	}

	if (russian_latin_sw)
		printf("  print: 'lat'\r\n");
	else
		printf("  print: 'rus'\r\n");

	if (LOGGING)
		printf("  debug: on\r\n");
	else
		printf("  debug: off\r\n");

	if (BREAKPOINT_TRS.l != 0)
		view_short_reg(&BREAKPOINT_TRS, "  breakpoint");
	else
		printf("  breakpoint fram: no used\r\n");

	if (BREAKPOINT_MB_TRS.l != 0)
		view_short_reg(&BREAKPOINT_MB_TRS, "  breakpoint drum");
	else
		printf("  breakpoint drum: no used\r\n");

	printf("  steps: %d\r\n", counter_step);
}

static void fram_cmd(int argc, char *argv[])
{
	if (argc > 2)
	{
		printf("ERROR fram_cmd!\r\n");
		return;
	}
	const char *arg = (argc > 1) ? argv[1] : "";
	if (valid_smtr((char *)arg) == 0)
	{
		dump_fram_zone(smtr((char *)arg));
	}
	else
	{
		size_t len;
		char pr[2] = {0};
		char sm[1] = {0};
		len = strlen(arg);
		if (len < 1 || len > 1)
		{
			printf("ERROR fram_cmd!\r\n");
			return;
		}
		memcpy(pr, lt2symtrs(*arg), 2);
		sm[0] = pr[1];
		dump_fram_zone(smtr(sm));
	}
}

static void drum_cmd(int argc, char *argv[])
{
	if (argc > 2)
	{
		printf("ERROR drum_cmd!\r\n");
		return;
	}
	const char *arg = (argc > 1) ? argv[1] : "";
	if (valid_smtr((char *)arg) == 0)
	{
		view_drum_zone(smtr((char *)arg));
	}
	else
	{
		size_t len;
		char sm[4] = {0};
		len = strlen(arg);
		if (len < 1 || len > 2)
		{
			printf("ERROR drum_cmd!\r\n");
			return;
		}
		switch (len)
		{
		case 1:
			memcpy(sm, lt2symtrs(*(arg + 0)), 2);
			break;
		case 2:
			memcpy(sm, lt2symtrs(*(arg + 0)), 2);
			memcpy(sm + 2, lt2symtrs(*(arg + 1)), 2);
			break;
		}
		view_drum_zone(smtr(sm));
	}
}

static void help_cmd(int argc, char __attribute__((unused)) * argv[])
{
	if (argc > 1)
	{
		printf("ERROR help_cmd!\r\n");
		return;
	}
	help_print();
}

static void quit_cmd(int argc, char __attribute__((unused)) * argv[])
{
	if (argc > 1)
	{
		printf("ERROR quit_cmd!\r\n");
		return;
	}
	Emu_Stop();
	Write_Backup_DRUM("./drum/drum.bak");
	fflush(stdout);
	exit(0);
}

static void calc_cmd(int argc, char *argv[])
{
	printf("\r\nMK-Setun-01\r\n");
	printf("calc: ");
	if (argc < 2 || argc > 4)
	{
		printf("\r\ndbg: ERR#1\r\n");
		return;
	}
	double num1 = 0, num2 = 0, result;
	char operator = '+';
	if (argc >= 2)
		sscanf(argv[1], "%lf", &num1);
	if (argc >= 3)
		sscanf(argv[2], "%c", &operator);
	if (argc >= 4)
		sscanf(argv[3], "%lf", &num2);

	switch (operator)
	{
	case '+':
		result = num1 + num2;
		break;
	case '-':
		result = num1 - num2;
		break;
	case '*':
		result = num1 * num2;
		break;
	case '/':
		result = num1 / num2;
		break;
	default:
		printf("ERROR: Invalid operator\n");
		return;
	}
	printf("%.8lf %c %.8lf = %.8lf\n", num1, operator, num2, result);
	printf("\r\nTODO: add codes for calculate trits !\r\n\r\n");
}

static void register_commands(command_t *cmd)
{
	command_add(cmd, "dump", NULL, dump_cmd);
	command_add(cmd, "load", "l", load_cmd);
	command_add(cmd, "prn", NULL, prn_cmd);
	command_add(cmd, "debug", "d", debug_cmd);
	command_add(cmd, "begin", "b", begin_cmd);
	command_add(cmd, "pause", "p", pause_cmd);
	command_add(cmd, "run", "r", run_cmd);
	command_add(cmd, "step", "s", step_cmd);
	command_add(cmd, "break", "br", break_cmd);
	command_add(cmd, "breakmb", "brm", break_drum_cmd);
	command_add(cmd, "reg", "rg", reg_cmd);
	command_add(cmd, "view", "v", view_cmd);
	command_add(cmd, "fram", "fr", fram_cmd);
	command_add(cmd, "drum", "dr", drum_cmd);
	command_add(cmd, "help", "h", help_cmd);
	command_add(cmd, "quit", "q", quit_cmd);
	command_add(cmd, "calc", NULL, calc_cmd);
}

/** -------------------
 *   Func 'cli_prompt'
 */
void cli_prompt(void)
{
	printf("\r\n");
	printf("setun1958emu:\r\n");
}

/** -----------------------------------------------
 *  main()
 */

#if 1 // On: Для сборки setun1958emu использовать main(...)

int main(void)
{
	/* Печать версии приложение */
	print_version();

	/* Строка приглашение */
	cli_prompt();

	/* Инициализация таблиц выполнения операций "Сетунь-1958" */
	init_operation_lut();

	/* Инициализация таблиц символов ввода и вывода "Сетунь-1958" */
	init_tab4();

	/*
	 * Сброс виртуальной машины "Сетунь-1958"
	 */
	reset_setun_1958();

	/*
	 * Инициализация командного интерпретатора
	 */
	command_t cmd;
	command_init(&cmd, CMD_MAX_ARGS);
	register_commands(&cmd);

	/*
	 * Loop work CLI and setun1958emu
	 */
	static char cli_buf[CMD_INPUT_BUF_SIZE];
	static int cli_pos = 0;

	while (1)
	{
		char bufin[1024];

		fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

		ssize_t numRead = read(0, bufin, 1);
		if (numRead > 0)
		{
			char ch = bufin[0];

			if (ch == '\r' || ch == '\n')
			{
				printf("\r\n");
				cli_buf[cli_pos] = '\0';
				if (cli_pos > 0)
				{
					command_save_history(&cmd, cli_buf);
					if (!command_execute(&cmd, cli_buf))
					{
						printf("no command\r\n");
					}
				}
				cli_pos = 0;
				cli_prompt();
			}
			else if (ch == '\b' || ch == 127) /* Backspace */
			{
				if (cli_pos > 0)
				{
					cli_pos--;
					printf("\b \b");
				}
			}
			else if (cli_pos < CMD_INPUT_BUF_SIZE - 1)
			{
				cli_buf[cli_pos++] = ch;
				putchar(ch);
			}
			fflush(stdout);
		}
		else
		{
			/*
			 * Работа виртуальной машины "Сетунь" 
			 */
			Process_Work_Emulation();
		}
	}

	printf("\r\n");
	return 0;
}

#endif /* On/Off main(...) */

/* EOF 'setun1958_vm.c' */
