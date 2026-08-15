/**
 * unittest_framework.h
 *
 * Легковесный unit-test framework для проекта Emulator-Setun-1958.
 *
 * Возможности:
 *   - Регистрация тестовых функций void (*)(void)
 *   - Защита от сегфолтов/abort через setjmp/longjmp + обработчик сигналов
 *   - Макросы ASSERT для проверки условий
 *   - Подсчет passed / failed / skipped
 *   - Цветной вывод в терминал
 *   - Время выполнения каждого теста
 *
 * Create date: 04.08.2026
 */

#ifndef UNITTEST_FRAMEWORK_H
#define UNITTEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * КОНФИГУРАЦИЯ
 * ============================================================================ */

#ifndef UT_MAX_TESTS
#define UT_MAX_TESTS 256
#endif

#ifndef UT_NAME_LEN
#define UT_NAME_LEN 128
#endif

/* ============================================================================
 * ЦВЕТНОЙ ВЫВОД (ANSI escape codes)
 * ============================================================================ */

#define UT_COLOR_RED     "\033[31m"
#define UT_COLOR_GREEN   "\033[32m"
#define UT_COLOR_YELLOW  "\033[33m"
#define UT_COLOR_BLUE    "\033[34m"
#define UT_COLOR_RESET   "\033[0m"

#ifdef NO_COLOR
#undef UT_COLOR_RED
#undef UT_COLOR_GREEN
#undef UT_COLOR_YELLOW
#undef UT_COLOR_BLUE
#undef UT_COLOR_RESET
#define UT_COLOR_RED     ""
#define UT_COLOR_GREEN   ""
#define UT_COLOR_YELLOW  ""
#define UT_COLOR_BLUE    ""
#define UT_COLOR_RESET   ""
#endif

/* ============================================================================
 * ГЛОБАЛЬНОЕ СОСТОЯНИЕ
 * ============================================================================ */

extern jmp_buf ut_jump_buffer;
extern volatile int ut_jmp_ready;
extern volatile int ut_assert_failed;
extern const char *ut_current_test;

/* ============================================================================
 * МАКРОСЫ ASSERT
 * ============================================================================ */

#define UT_FAIL(msg) do {                                                     \
    ut_assert_failed = 1;                                                     \
    fprintf(stderr,                                                           \
        UT_COLOR_RED "[ASSERT FAIL] %s:%d  %s" UT_COLOR_RESET "\n",         \
        __FILE__, __LINE__, msg);                                             \
    if (ut_jmp_ready) longjmp(ut_jump_buffer, 1);                            \
    abort();                                                                  \
} while (0)

#define UT_ASSERT(cond) do {                                                  \
    if (!(cond)) {                                                            \
        UT_FAIL(#cond);                                                       \
    }                                                                         \
} while (0)

#define UT_ASSERT_EQ(expected, actual) do {                                   \
    if ((expected) != (actual)) {                                             \
        fprintf(stderr,                                                       \
            UT_COLOR_RED "[ASSERT EQ FAIL] %s:%d\n"                          \
            "  expected: %lld\n  actual:   %lld\n" UT_COLOR_RESET "\n",      \
            __FILE__, __LINE__,                                               \
            (long long)(expected), (long long)(actual));                      \
        ut_assert_failed = 1;                                                 \
        if (ut_jmp_ready) longjmp(ut_jump_buffer, 1);                         \
        abort();                                                              \
    }                                                                         \
} while (0)

#define UT_ASSERT_NEQ(expected, actual) do {                                  \
    if ((expected) == (actual)) {                                             \
        fprintf(stderr,                                                       \
            UT_COLOR_RED "[ASSERT NEQ FAIL] %s:%d\n"                         \
            "  unexpected: %lld\n  actual:     %lld\n" UT_COLOR_RESET "\n",  \
            __FILE__, __LINE__,                                               \
            (long long)(expected), (long long)(actual));                      \
        ut_assert_failed = 1;                                                 \
        if (ut_jmp_ready) longjmp(ut_jump_buffer, 1);                         \
        abort();                                                              \
    }                                                                         \
} while (0)

#define UT_ASSERT_TRUE(cond)   UT_ASSERT(cond)
#define UT_ASSERT_FALSE(cond)  UT_ASSERT(!(cond))
#define UT_ASSERT_NULL(ptr)    UT_ASSERT_EQ(NULL, (void*)(ptr))
#define UT_ASSERT_NOT_NULL(ptr) UT_ASSERT_NEQ(NULL, (void*)(ptr))

/* ============================================================================
 * РЕГИСТРАЦИЯ И ЗАПУСК ТЕСТОВ
 * ============================================================================ */

/** Тип тестовой функции */
typedef void (*ut_test_func_t)(void);

/**
 * @brief Регистрация тестовой функции.
 *
 * @param name Имя теста (для отчета).
 * @param func Указатель на функцию void (*)(void).
 */
void ut_register_test(const char *name, ut_test_func_t func);

/**
 * @brief Запуск всех зарегистрированных тестов.
 *
 * @return Количество FAILED тестов (0 = все прошли).
 */
int ut_run_all_tests(void);

/**
 * @brief Запуск одного теста по имени.
 *
 * @param name Имя теста.
 * @return 0 если passed, 1 если failed или не найден.
 */
int ut_run_test(const char *name);

/**
 * @brief Вывод сводной статистики.
 */
void ut_print_summary(void);

/**
 * @brief Сброс всех зарегистрированных тестов.
 */
void ut_reset(void);

/* ============================================================================
 * ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
 * ============================================================================ */

/** Установка обработчиков сигналов (SIGSEGV, SIGABRT, SIGFPE, SIGILL) */
void ut_setup_signal_handlers(void);

/** Восстановление обработчиков сигналов по умолчанию */
void ut_restore_signal_handlers(void);

#ifdef __cplusplus
}
#endif

#endif /* UNITTEST_FRAMEWORK_H */
