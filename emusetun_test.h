/**
  ******************************************************************************
  * @file    emusetun_test.h
  *
  * @brief
  ******************************************************************************
  */

#ifndef EMUSETUN_TEST_H_
#define EMUSETUN_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>


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
#define ZONE_DRUM_BEG (5)  /* 01-- */
#define ZONE_DRUM_END (40) /* ++++ */

 /* Add code */
#define LOGGING_print(...)            \
do                                    \
{                                     \
		if (LOGGING)                      \
			fprintf(stdout, __VA_ARGS__);   \
} while (0);

/**
 * Типы данных для виртуальной троичной машины "Сетунь-1958"
 */
typedef unsigned int addr;

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


/* Состояние работы эмулятора */
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
} emustatus_e;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* EMUSETUN_TEST_H_ */
