/**
 * Filename: "emusetun_test.c " 
 *
 * Project: Виртуальная машина МЦВМ "Сетунь" 1958 года на языке Си
 *
 * Тестирование функционала эмулятора setun1958
 *  
 * Create date: 16.11.2024
 * Edit date:   24.01.2025
 */

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

#include "emusetun_test.h"

/* **********************************************************************************
 * Внешние переменные и вызовы функций
 * ----------------------------------------------------------------------------------
 */
/* Фотосчитыватель ФТ1 */
extern FILE *ptr1;
/* Фотосчитыватель ФТ2 */
extern FILE *ptr2;
/* Перфоратор ПЛ */
extern FILE *ptp1;
/* Печать Телетайп ТП, Пишущая машинка ПМ */
extern FILE *tty1;
/* Состояние DRUM */
extern FILE *drum;

/**
 * Определение памяти машины "Сетунь-1958"
 */
extern trs_t mem_fram[SIZE_GR_TRIT_FRAM][SIZE_GRFRAM];						   /* оперативное запоминающее устройство на ферритовых сердечниках */
extern trs_t mem_drum[NUMBER_ZONE_DRUM + ZONE_DRUM_BEG][SIZE_ZONE_TRIT_DRUM]; /* запоминающее устройство на магнитном барабане */

/* Основные регистры в порядке пульта управления */
extern trs_t K; /* K(1:9)  код команды (адрес ячейки оперативной памяти) */
extern trs_t F; /* F(1:5)  индекс регистр  */
extern trs_t C; /* C(1:5)  программный счетчик  */
extern trs_t W; /* W(1:1)  знак троичного числа */

extern trs_t ph1; /* ph1(1:1) 1 разряд переполнения */
extern trs_t ph2; /* ph2(1:1) 1 разряд переполнения */
extern trs_t S;  /* S(1:18) аккумулятор */
extern trs_t R;  /* R(1:18) регистр множителя */
extern trs_t MB;  /* MB(1:4) троичное число зоны магнитного барабана */

/* Дополнительные */
extern trs_t MR;		 /* временный регистр для обмена с FRAM */
extern long_trs_t TMP; /* временная переменная для троичного числа */

extern trs_t BRPNT; /* BRPNT(1:5) - точка остановки по значению программного счетчика */

extern trs_t C_new; /* C_new(1:5) новое значение программного счетчика в скрипт-файле после остовона машины */
extern char action_post;	 /*
						  *  action действие после останова
						  *  action = 'S' - оставаться в состоянии <STOP>
						  *  action = 'R' - продолжить работу машины с нового адреса C_new
						  */

/** ------------------------------------------------------
 *  Прототипы функций для виртуальной машины "Сетунь-1958"
 *  ------------------------------------------------------
 */
extern int32_t pow3(int8_t x);
extern double t3_to_d10(trs_t t);
extern trs_t d10_to_t3(float f);
extern int8_t trit2int(trs_t t);
extern trs_t bit2trit(int8_t b);

/**  ---------------------------------------
 *   Операции с тритами
 *   trit = {-1,0,1}  - трит значение трита
 */
extern void and_t(int8_t *a, int8_t *b, int8_t *s);
extern void xor_t(int8_t *a, int8_t *b, int8_t *s);
extern void or_t(int8_t *a, int8_t *b, int8_t *s);
extern void not_t(int8_t *a, int8_t *s);
extern void sum_t(int8_t *a, int8_t *b, int8_t *p0, int8_t *s, int8_t *p1);

/**  -------------------------------------------------------------
 *   Троичные числа
 *
 *   TRITS-1  = [t0]       - обозначение позиции тритов в числе
 *   TRITS-32 = [t31...t0] - обозначение позиции тритов в числе
 *
 */
extern int8_t get_trit(trs_t t, uint8_t pos);
extern trs_t set_trit(trs_t t, uint8_t pos, int8_t trit);
extern trs_t slice_trs(trs_t t, int8_t p1, int8_t p2);
extern void copy_trs(trs_t *src, trs_t *dst);

/**  -------------------------------------------------------------
 *   Троичные числа
 *   TRITS-64 = [t63...t0] - обозначение позиции тритов в числе
 */
extern void clear_long(long_trs_t *t);
extern void clear_full_long(long_trs_t *t);
extern int8_t get_long_trit(long_trs_t t, uint8_t pos);
extern long_trs_t set_long_trit(long_trs_t t, uint8_t pos, int8_t trit);
extern int8_t sgn_long_trs(long_trs_t x);
extern long_trs_t shift_long_trs(long_trs_t t, int8_t s);
extern void copy_long_trs(long_trs_t *src, long_trs_t *dst);

/**  -------------------------------------------------------------
 *   Троичные числа регистров Setun-1958
 *
 *   SETUN-T1  = [s1]      - обозначение позиции тритов в регистре
 *   SETUN-T5  = [s1...s5]
 *   SETUN-T9  = [s1...s9]
 *   SETUN-T18 = [s1...s18]
 */
extern int8_t get_trit_setun(trs_t t, uint8_t pos);
extern trs_t set_trit_setun(trs_t t, uint8_t pos, int8_t trit);
extern trs_t slice_trs_setun(trs_t t, int8_t p1, int8_t p2);
extern void copy_trs_setun(trs_t *src, trs_t *dst);

/**
 * Общие функции для троичных чисел из тритов
 */
extern void clear(trs_t *t);
extern void mod_3_n(trs_t *t, uint8_t n);
extern void clear_full(trs_t *t);

extern void inc_trs(trs_t *tr);
extern void dec_trs(trs_t *tr);

extern trs_t and_trs(trs_t a, trs_t b);
extern trs_t or_trs(trs_t a, trs_t b);
extern trs_t xor_trs(trs_t a, trs_t b);
extern trs_t add_trs(trs_t a, trs_t b);
extern trs_t sub_trs(trs_t a, trs_t b);
extern trs_t mul_trs(trs_t a, trs_t b);
extern trs_t div_trs(trs_t a, trs_t b);
extern trs_t shift_trs(trs_t t, int8_t s);
extern int cmp_trs(trs_t a, trs_t b);

/* Long trits */
extern long_trs_t add_long_trs(long_trs_t a, long_trs_t b);

/* Преобразование тритов в другие типы данных */
extern int32_t trs2digit(trs_t t);
extern uint8_t trit2lt(int8_t v);
extern int8_t symtrs2numb(uint8_t c);
extern int8_t str_symtrs2numb(uint8_t *s);
extern trs_t smtr(uint8_t *s);
extern uint8_t valid_smtr(uint8_t *s);

/* Операции с ферритовой памятью машины FRAM */
extern void clean_fram_zone(trs_t z);
extern void clean_fram(void);
extern trs_t ld_fram(trs_t ea);
extern void st_fram(trs_t ea, trs_t v);

/* Операции ввода и вывода "Сетунь-1958" */

/* Регист переключения Русский/Латинский */
extern uint8_t russian_latin_sw;
/* Регист переключения Буквенный/Цифровой */
extern uint8_t letter_number_sw;
/* Регист переключения цвета печатающей ленты */
extern uint8_t color_sw;

extern uint8_t pl_to_ind(uint8_t *perline);
extern int ConvertSWtoPaper(char *path_lst, char *path_txt);
extern int DumpFileTxs(char *pathfile);
extern trs_t Decoder_Command_Paper_Line(char *paperline, uint8_t *err);
extern trs_t Decoder_Symbol_Paper_Line(char *paperline, uint8_t *err);
extern uint8_t Read_Commands_from_FT1(FILE *file, trs_t fa);
extern uint8_t Read_Symbols_from_FT1(FILE *file, trs_t fa);
extern uint8_t Read_Commands_from_FT2(FILE *file, trs_t fa);
extern uint8_t Read_Symbols_from_FT1(FILE *file, trs_t fa);
extern uint8_t Read_Symbols_from_FT2(FILE *file, trs_t fa);
extern uint8_t Write_Commands_to_TTY1(FILE *file, trs_t fa);
extern uint8_t Write_Symbols_to_TTY1(FILE *file, trs_t fa);
extern uint8_t Perforation_Commands_to_PTP1(FILE *file, trs_t fa);
extern uint8_t Perforation_Symbols_to_PTP1(FILE *file, trs_t fa);
extern void init_tab4(void);

/* Очистить память магнитного барабана DRUM */
extern void clean_drum(void);
extern trs_t ld_drum(trs_t ea, uint8_t ind);
extern void st_drum(trs_t ea, uint8_t ind, trs_t v);

/* Читать / Записать зоны магнитного барабана в файл paper.txt */
extern int Read_Backup_DRUM(char * drum_path);
extern int Write_Backup_DRUM(char * drum_path);

/* Операции копирования */
extern void fram_to_drum(trs_t ea);
extern void drum_to_fram(trs_t ea);

/* Функции троичной машины Сетунь-1958 */
extern void reset_setun_1958(void);				/* Сброс машины */
extern trs_t control_trs(trs_t a);					/* Устройство управления */
extern trs_t next_address(trs_t c);				/* Определить следующий адрес */
extern int8_t execute_trs(trs_t addr, trs_t oper); /* Выполнение кодов операций */

/* Функции вывода отладочной информации */
extern void view_short_reg(trs_t *t, uint8_t *ch);
extern void view_short_regs(void);

extern uint8_t Begin_Read_Commands_from_FT1(FILE *file);
extern void dump_fram_zone(trs_t z);
extern uint8_t zone_drum_to_index(trs_t z);
extern void view_drum_zone(trs_t zone);
extern void electrified_typewriter(trs_t t, uint8_t local);
extern void view_checksum_setun(trs_t t);
extern void trs2str(trs_t t);
extern void cmd_str_2_trs(uint8_t *syms, trs_t *r);
extern void view_elem_fram(trs_t ea);
extern void view_fram(trs_t addr1, trs_t addr2);
extern void view_short_long_reg(long_trs_t *t, uint8_t *ch);
extern int8_t sgn_trs(trs_t x);


/**
 * Вывод отладочной информации памяти машины "Сетунь-1958"
 */
static uint8_t LOGGING = 0;	  /* флаг логирование выполнение операций */
static uint8_t STEP_FLAG = 0; /* флаг выполнить количество шагов */
static uint32_t STEP = 0;	  /* счетчик количества операций */
static uint32_t counter_step = 0;
static int32_t BREAKPOINT = 0;	  /* режима останова по значению программного счетчика */
static trs_t BREAKPOINT_TRS;	  /* режима останова по значению программного счетчика */
static int32_t BREAKPOINT_MB = 0; /* режима останова по значению адреса магнитного барабана */
static trs_t BREAKPOINT_MB_TRS;	  /* режима останова по значению адреса магнитного барабана */

static emustatus_e emu_stat = NOREADY_EMU_ST;


/* **********************************************************************************
 * Тестирование функций операций с тритами
 * ----------------------------------------------------------------------------------
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
	int8_t trit; /* трит */
	trs_t tr1;	 /* троичное число */
	trs_t tr2;	 /* троичное число */

	printf("\r\n --- TEST #2  Operations TRITS-32 for VM SETUN-1958 --- \r\n");

	/* t2.1 POW3 */
	printf("\r\nt2.1 --- POW3(...)\r\n");
	printf("pow3(3)  = %d\r\n", (int32_t)pow3(3));
	printf("pow3(18) = %d\r\n", (int32_t)pow3(18));

	/* t2.2 Point address */
	printf("\r\nt2.2 --- Point address\r\n");
	addr pMem;
	pMem = 0xffffffff;
	printf("pMem = 0xffffffff [%u]\r\n", pMem);

	/* t2.3 */
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

	/* t2.4 */
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

	/* t2.4 */
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

	/* t2.5 */
	printf("\r\nt2.5 --- input TRITS-32s\r\n");
	printf("input tr1=[+000000000000000000000000000000-] \r\n");
	tr1 = smtr("+000000000000000000000000000000-");
	view_short_reg(&tr1, "tr1=");
	printf("input tr2=[-000000000+] \r\n");
	tr2 = smtr("-000000000+");
	view_short_reg(&tr2, "tr2=");

	/* t2.6 */
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

	/* t2.7 */
	printf("\r\nt2.7 --- printf long int\r\n");
	long int ll = 0x00000001lu;
	printf("long int ll = 0x00000001lu\r\n");
	printf("ll = %lu\r\n", ll);
	ll <<= 1;
	printf("ll <<= 1; ll = %lu\r\n", ll);
	ll <<= 1;
	printf("ll <<= 1; ll = %lu\r\n", ll);
	printf("\r\n");

	/* t2.8 */
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

	/* t2.9 */
	printf("\r\nt2.9 --- view_short_reg()\r\n");
	tr1 = smtr("+-0+-0+-0");
	view_short_reg(&tr1, "tr1 =");

	/* t2.10 */
	printf("\r\nt2.10 --- slice_trs(...)\r\n");
	tr1 = smtr("+000000000000000000000000000000-");
	printf("tr1 := [+000000000000000000000000000000-]\r\n");
	tr2 = slice_trs(tr1, 0, 15);
	view_short_reg(&tr2, "tr2[15,0] =");
	printf("tr1 := [+000000000000000000000000000000-]\r\n");
	tr2 = slice_trs(tr1, 16, 31);
	view_short_reg(&tr2, "tr2[31,16] =");

	/* t2.11 */
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

	/* t2.12 */
	printf("\r\nt2.12 --- inc_trs(...)\r\n");
	tr1 = smtr("+0-0+---");
	view_short_reg(&tr1, "tr1 =");
	inc_trs(&tr1);
	printf("inc_trs(&tr1)\r\n");
	view_short_reg(&tr1, "tr1 =");

	/* t2.13 */
	printf("\r\nt2.13 --- dec_trs(...)\r\n");
	tr1 = smtr("+0-0-+0-");
	view_short_reg(&tr1, "tr1 =");
	dec_trs(&tr1);
	printf("dec_trs(&tr1)\r\n");
	view_short_reg(&tr1, "tr1 =");

	/* t2.14 */
	printf("\r\nt2.14 --- add_trs(...)\r\n");
	tr1 = smtr("+---+");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("+0-+0");
	view_short_reg(&tr2, "tr2 =");
	tr1 = add_trs(tr1, tr2);
	printf("tr1 = add_trs(tr1,tr2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	/* t2.14.1 */
	printf("\r\nt2.14.1 --- add_trs(...)\r\n");
	tr1 = smtr("-+0000000++++++++");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("0+0000000");
	view_short_reg(&tr2, "tr2 =");
	tr1 = add_trs(tr1, tr2);
	printf("tr1 = add_trs(tr1,tr2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	/* t2.14.2 */
	printf("\r\nt2.14.2 --- add_trs(...)\r\n");
	tr1 = smtr("0+0000000+++++++++");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("0+0000000000000000");
	view_short_reg(&tr2, "tr2 =");
	tr1 = add_trs(tr1, tr2);
	printf("tr1 = add_trs(tr1,tr2)\n");
	view_short_reg(&tr1, "tr1 =");

	/* t2.15 */
	printf("\r\nt2.15 --- sub_trs(...)\r\n");
	tr1 = smtr("+0-+00--");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("+0-0+--+");
	view_short_reg(&tr2, "tr2 =");
	tr1 = sub_trs(tr1, tr2);
	printf("tr1 = sub_trs(tr1,tr2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	/* t2.15 */
	printf("\r\nt2.15 --- shift_trs(...)\r\n");
	tr1 = smtr("0000+0000");
	view_short_reg(&tr1, "tr1 =");
	tr1 = shift_trs(tr1, -2);
	printf("tr1 = shift_trs(tr1,-2)\r\n");
	view_short_reg(&tr1, "tr1 =");
	tr1 = shift_trs(tr1, -2);
	printf("tr1 = shift_trs(tr1,-2)\r\n");
	view_short_reg(&tr1, "tr1 =");

	/* t2.16 */
	printf("\r\nt2.16 --- mul_trs(...)\r\n");
	tr1 = smtr("+0000000000000000+");
	tr1.l = 18;
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("0-0000000000000000");
	tr2.l = 18;
	view_short_reg(&tr2, "tr2 =");

	tr2 = mul_trs(tr1, tr2);
	view_short_reg(&tr2, "tr2 =");

	/* t2.17 */
	printf("\r\nt2.17 --- mul_trs(...)\r\n");
	tr1 = smtr("0000000++");
	view_short_reg(&tr1, "tr1 =");
	tr2 = smtr("000000000000000---");
	view_short_reg(&tr2, "tr2 =");
	tr2 = mul_trs(tr1, tr2);
	view_short_reg(&tr2, "tr2 =");

	/* t2.18 */
	printf("\r\nt2.18 --- shift_long_trs(...)\r\n");
	long_trs_t ltr1;
	ltr1.l = 64;
	tr1 = smtr("0-00+0000");
	ltr1.t1 = tr1.t1;
	ltr1.t0 = tr1.t0;

	/*
	 printf("ltr1.t1=%p\r\n",(long int)ltr1.t1);
	 printf("ltr1.t0=%p\r\n",(long int)ltr1.t0);
	*/
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

	/* t3.1 */
	printf("\r\nt3.1 --- get_trit_setun(...)\r\n");
	S = smtr("+000000-000000000-");
	view_short_reg(&S, "S");
	trit = get_trit_setun(S, 1);
	ps(trit, 1);
	trit = get_trit_setun(S, 8);
	ps(trit, 8);
	trit = get_trit_setun(S, 18);
	ps(trit, 18);

	/* t3.2 */
	printf("\r\nt3.2 --- set_trit_setun(...)\r\n");
	S = smtr("000000000000000000");
	S = set_trit_setun(S, 2, -1);
	ps(-1, 2);
	S = set_trit_setun(S, 5, 1);
	ps(1, 5);
	S = set_trit_setun(S, 18, -1);
	ps(-1, 18);
	view_short_reg(&S, "S");

	/* t3.3 */
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

	/* t3.4 */
	printf("\r\nt3.4 --- slice_trs_setun(...)\r\n");
	S = smtr("+0000000000000000-000000");
	K = slice_trs_setun(S, 1, 9);
	view_short_reg(&S, "S");
	view_short_reg(&K, "K[1,9] =");
	K = slice_trs_setun(S, 10, 18);
	view_short_reg(&S, "S");
	view_short_reg(&K, "K[10,18] =");

	/* t3.5 */
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

	/* t3.7 */
	printf("\r\nt3.7 --- next_address(...)\r\n");
	C = smtr("000--");
	view_short_reg(&C, "beg  C");
	for (int8_t i = 0; i < 10; i++)
	{
		C = next_address(C);
		view_short_reg(&C, "next C");
	}

	/* t3.8 */
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

	/* t3.8 */
	printf("\r\nt3.8 --- st_fram(...)\r\n");
	
	aa = smtr("00000");
	K = smtr("+0000000-");
	st_fram(aa, K);
	view_short_reg(&aa, "aa");
	view_short_reg(&K, "K");
	
	aa = smtr("0000+");
	K = smtr("-0000000+");
	st_fram(aa, K);
	view_short_reg(&aa, "aa");
	view_short_reg(&K, "K");
	
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	/* t3.9 */
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

	/* t3.10 test Oper=k6..8[+00]: (A*)=>(S) */
	printf("\r\nt3.10:  Oper=k6..8[+00]: (A*)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	addr = smtr("0000+");
	m1 = smtr("00000+000");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);

	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	/* t3.11 test Oper=k6..8[-++]: (S)=>(A*) */
	printf("\r\nt3.11:  Oper=k6..8[-++]: (S)=>(A*)\r\n");
	
	reset_setun_1958();
	
	S = smtr("-0000000+000-+0000");
	
	addr = smtr("0000+");
	m1 = smtr("0000--++0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]r\\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();
	
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	/* t3.12 test Oper=k6..8[+-+]: (A*)=>(R) */
	printf("\r\nt3.12:  Oper=k6..8[+-+]: (A*)=>(R)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	addr = smtr("0000+");
	m1 = smtr("00000+-+0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	/* t3.13 test Oper=k6..8[0-0]: (A*)=>(F) */
	printf("\r\nt3.12:  Oper=k6..8[0-0]: (A*)=>(F)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("+0-0+0-00");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	addr = smtr("0000+");
	m1 = smtr("000000-00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	/* t3.14 test Oper=k6..8[00-]: (F)=>(A*) */
	printf("\r\nt3.14:  Oper=k6..8[00-]: (F)=>(A*)\r\n");
	
	reset_setun_1958();
	
	F = smtr("-000+");
	
	addr = smtr("0000+");
	m1 = smtr("0000000-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();
	
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	/* t3.15 test Oper=k6..8[00+]: (C)=>(A*) */
	printf("\r\nt3.15:  Oper=k6..8[00+]: (C)=>(A*)\r\n");
	
	reset_setun_1958();
	
	F = smtr("-000+");
	
	addr = smtr("0000+");
	m1 = smtr("0000000+0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();
	
	ad1 = smtr("000--");
	ad2 = smtr("000++");
	view_fram(ad1, ad2);

	/* t3.16 test Oper=k6..8[+0+]: (S)+(A*)=>(S) */
	printf("\r\nt3.16:  Oper=k6..8[+0+]: (S)+(A*)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("0000000+0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("00000000000000000+");
	
	addr = smtr("0000+");
	m1 = smtr("00000+0+0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	/* t3.17 test Oper=k6..8[+0-]: (S)-(A*)=>(S) */
	printf("\r\nt3.17:  Oper=k6..8[+0-]: (S)-(A*)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("0000000++");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("000000000000000+0+");
	view_short_reg(&S, "S=");
	
	addr = smtr("0000+");
	m1 = smtr("00000+0-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	/* t3.18 test Oper=k6..8[++0]: (S)=>(R); (A*)(R)=>(S) */
	printf("\r\nt3.18:  Oper=k6..8[++0]: (S)=>(R); (A*)(R)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("0+0000000");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("00000000+++0000000");
	view_short_reg(&S, "S=");
	
	addr = smtr("0000+");
	m1 = smtr("00000++00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	/* t3.19 test Oper=k6..8[+++]: (S)+(A*)(R)=>(S) */
	printf("\r\nt3.19:  Oper=k6..8[+++]: (S)+(A*)(R)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("000000--0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	
	R = smtr("0000000000000000+0");
	view_short_reg(&R, "R=");
	
	addr = smtr("0000+");
	m1 = smtr("00000+++0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	/* t3.21 test Oper=k6..8[++-]: (A*)+(S)(R)=>(S)=>(S) */
	printf("\r\nt3.21:  Oper=k6..8[++-]: (A*)+(S)(R)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("000000--0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	
	R = smtr("0000000000000000+0");
	view_short_reg(&R, "R=");
	
	addr = smtr("0000+");
	m1 = smtr("00000++-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	printf("   \r\n");

	/* 3.22 test Oper=k6..8[+-0]: (A*)[x](S)=>(S) */
	printf("\r\nt3.22:  Oper=k6..8[+-0]: (A*)[x](S)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("000000--0");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	
	R = smtr("0000000000000000+0");
	view_short_reg(&R, "R=");
	
	addr = smtr("0000+");
	m1 = smtr("00000+-00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* ork VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	/* view_short_regs(); */

	/* t3.23 test Oper=k6..8[-+0]: S сд. (A*)=>(S) */
	printf("\r\nt3.23:  Oper=k6..8[-+0]: S сд. (A*)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("-000-");
	m0 = smtr("+0000000-+000-000-");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	/*
	 R = smtr("0000000000000000+0");
	 view_short_reg(&R, "R=");
	*/
	
	addr = smtr("0000+");
	m1 = smtr("-000-0-+00");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	view_short_reg(&S, "S=");
	/* view_short_regs(); */

	/* t3.24 test Oper=k6..8[-+-]: Норм.(S)=>(A*); (N)=>(S) */
	printf("\r\nt3.24:  Oper=k6..8[-+-]:  Норм.(S)=>(A*); (N)=>(S)\r\n");
	
	reset_setun_1958();
	
	addr = smtr("00000");
	m0 = smtr("00000000-");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	S = smtr("000000000000000+00");
	view_short_reg(&S, "S=");
	/*
	 R = smtr("0000000000000000+0");
	 view_short_reg(&R, "R=");
	*/
	addr = smtr("0000+");
	m1 = smtr("00000-+-0");
	st_fram(addr, m1);
	view_elem_fram(addr);

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);
	view_short_reg(&K, "K=");
	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	printf("S.l=%d\r\n", S.l);
	view_short_reg(&S, "S=");

	addr = smtr("00000");
	view_elem_fram(addr);

	/* view_short_regs(); */

	/* t3.25 test Oper=k6..8[-00]: */
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

	/* t4.1 test Oper=k6..8[+00]: (A*)=>(S) */
	printf("\r\nt4.1:  Oper=k6..8[+00]: (A*)=>(S)\r\n");
	
	reset_setun_1958();

	printf("\r\n");
	/* FRAM(-1) test */
	addr = smtr("-0000");
	m0 = smtr("00000000+");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
	addr = smtr("-000+");
	m0 = smtr("-0000000+");
	st_fram(addr, m0);
	view_elem_fram(addr);
	
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

	/* work VM Setun-1958 */
	C = smtr("0000+");
	K = ld_fram(C);
	view_short_reg(&K, "C(0000+)");

	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
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
	char str[1024] = {0};

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

				LOGGING_print("%s -> [", cmd);
				trs2str(dst);
				LOGGING_print("]");
				if (LOGGING > 0)
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
	char str[1024] = {0};

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

				LOGGING_print("%s -> [", cmd);
				trs2str(dst);
				LOGGING_print("]");
				if (LOGGING > 0)
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
	printf("fclose: software/ip5_in_out_10_3/00_ip5_in_out.lst\r\n");

	printf("\r\n --- END TEST #6 --- \r\n");
}

void Test7_Setun_Load(void)
{
	printf("\r\n --- TEST #7  Load program FT1,FT2 for VM SETUN-1958 --- \r\n\r\n");

	FILE *file_lst;

	/* Переменная, в которую поочередно будут помещаться считываемые строки */
	char str[1024] = {0};

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

			LOGGING_print("%s -> [", cmd);
			trs2str(dst);
			LOGGING_print("]");
			if (LOGGING > 0)
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
		/*
		 printf("\r\n i=%i\r\n", i);
		*/
	}

	fclose(file_lst);
	printf("fclose: software/ip5_in_out_3_10/00_ip5_in_out_3_10.lst\r\n");
	printf("\r\n --- END TEST #7 --- \r\n");
}

void Test8_Setun_Electrified_Typewriter(void)
{
	printf("\r\n --- TEST #8 electrified_typewriter() and dump --- \r\n");

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
			//viv- error fall electrified_typewriter(cp, 0);
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
			//viv- error fall electrified_typewriter(cp, 0);
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
			//viv- error fall electrified_typewriter(cp, 1);
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
			//viv- error fall  electrified_typewriter(cp, 1);
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

	/* ----------------------------------------------- */
	trs_t exK;
	trs_t oper;
	trs_t m0;
	trs_t m1;
	trs_t addr;
	trs_t ad1;
	trs_t ad2;
	uint8_t ret_exec;

	/* t19 test Oper=k6..8[+00]: (A*)=>(S) */
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

	/* t20 test mul_trs() */

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

	/* t21 test Oper=k6..8[+00]: (A*)=>(S) */
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

	/* views */
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

	/* views */
	ad1 = smtr("000-0");
	ad2 = smtr("00+-0");
	
	/* view_fram(ad1, ad2); */

	view_short_regs();

	/* t22 test DRUM */
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


	printf("t23 test zone for view_drum()\r\n");

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
	zi = smtr("0---"); /* -13 (Dec) */

	for (uint8_t zz = 0; zz < NUMBER_ZONE_DRUM; zz++)
	{
		for (uint8_t mm = 0; mm < SIZE_ZONE_TRIT_FRAM; mm++)
		{
			st_drum(zi, mm, inr);
			inc_trs(&inr);
		}
		inc_trs(&zi);
	}
	
	zi.l = 4;
	zi = smtr("0---"); /* -17 */	
	view_drum_zone(zi);
	inc_trs(&zi);
	view_drum_zone(zi);


	printf("\r\nt25 test oper='-0-' (Мд*)=>(Фа*) \r\n");

	addr = smtr("0000+");
	m1 = smtr("00+---0-0"); /* -0- */
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
	m1 = smtr("000+--0+0"); /* -0+ */
	st_fram(addr, m1);

	K = ld_fram(C);
	exK = control_trs(K);
	view_short_reg(&K, "K=");
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	printf("ret_exec = %i\r\n", ret_exec);

	printf("DRUM zone='0+--'");
	ad1 = smtr("0+--");
	view_drum_zone(ad1);
	printf("\r\n");
	
	printf("DRUM zone='0+--'");
	ad1 = smtr("0+--");
	view_drum_zone(ad1);
	printf("\r\n");

	printf("\r\n --- END TEST #8 --- \r\n");
}

/* Вывод "бумажной ленты. */
void Test9_Perforatin_Paper_Line(void)
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

	printf("\r\n --- TEST #9 Perforatin paper line --- \r\n");

	/* t11.1 test Oper=k6..8[+00]: (A*)=>(S) */
	printf("\r\nt11.1:  Oper=k6..8[+00]: (A*)=>(S)\r\n");
	
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

	drum = fopen("drum/paper.txt", "wr");
	if (drum == NULL)
	{
		printf("Error fopen 'drum/paper.txt'\r\n");
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

	
	addr = smtr("0000+");
	m0 = smtr("00+00-000");
	st_fram(addr, m0);
	view_elem_fram(addr);
	

	/* Begin address fram */
	C = smtr("0000+");
	printf("\r\nreg C = 00001\r\n");

	/* work VM Setun-1958 */
	K = ld_fram(C);

	exK = control_trs(K);
	oper = slice_trs_setun(K, 6, 8);
	ret_exec = execute_trs(exK, oper);
	
	if (ret_exec == 0)
		printf("[status: OK']\r\n");
	if (ret_exec != 0)
		printf("[status: ERR#%d]\r\n", ret_exec);
	printf("\r\n");
	
	view_short_regs();

	trs_t fa;
	fa = smtr("0---0");

	dump_fram_zone(smtr("0"));

	printf("\r\n --- END TEST #9 --- \r\n");
}

void Test10_LoadSWSetun(void)
{
	printf("\r\n --- Load software anf DUMP FRAM  for VM SETUN-1958 --- \r\n\r\n");

	FILE *file_lst;

	/* Переменная, в которую поочередно будут помещаться считываемые строки */
	char str[1024] = {0};

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

				LOGGING_print("%s -> [", cmd);
				trs2str(dst);
				LOGGING_print("]");
				if (LOGGING > 0)
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

	/*
	 dump_fram_zone(smtr("0"));
	 dump_fram_zone(smtr("+"));
	 dump_fram_zone(smtr("-"));
	*/

	printf("\r\n END TEST#10 Load software\r\n");
}

/*----------------- END TESTS ---------------*/

/* EOF 'emusetun_test.c' */
