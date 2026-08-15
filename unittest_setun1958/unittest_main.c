/**
 * unittest_main.c
 *
 * Точка входа для unit-test runner проекта Emulator-Setun-1958.
 *
 * Регистрирует все существующие тестовые функции из emusetun_test.c
 * и запускает их через unittest_framework.
 *
 * Сборка:
 *   make unittest
 *   ./unittest
 *
 * Запуск отдельного теста:
 *   ./unittest Test3_Setun_Opers
 *
 * Create date: 04.08.2026
 */

#include <stdio.h>
#include <string.h>
#include "unittest_framework.h"

/* ============================================================================
 * ПРОТОТИПЫ ТЕСТОВЫХ ФУНКЦИЙ (из emusetun_test.c)
 * ============================================================================ */

void Test1_Ariphmetic_Ternary(void);
void Test2_Opers_TRITS_32(void);
void Test3_Setun_Opers(void);
void Test4_Setun_Opers(void);
void Test5_Setun_Load(void);
void Test6_Setun_Load(void);
void Test7_Setun_Load(void);
void Test8_Setun_Electrified_Typewriter(void);
void Test9_Perforatin_Paper_Line(void);
void Test10_LoadSWSetun(void);

/* ============================================================================
 * РЕГИСТРАЦИЯ ВСЕХ ТЕСТОВ
 * ============================================================================ */

static void register_all_tests(void)
{
    ut_register_test("Test1_Ariphmetic_Ternary",       Test1_Ariphmetic_Ternary);
    ut_register_test("Test2_Opers_TRITS_32",           Test2_Opers_TRITS_32);
    ut_register_test("Test3_Setun_Opers",              Test3_Setun_Opers);
    ut_register_test("Test4_Setun_Opers",              Test4_Setun_Opers);
    ut_register_test("Test5_Setun_Load",               Test5_Setun_Load);
    ut_register_test("Test6_Setun_Load",               Test6_Setun_Load);
    ut_register_test("Test7_Setun_Load",               Test7_Setun_Load);
    ut_register_test("Test8_Setun_Electrified_Typewriter",
                                              Test8_Setun_Electrified_Typewriter);
    ut_register_test("Test9_Perforatin_Paper_Line",    Test9_Perforatin_Paper_Line);
    ut_register_test("Test10_LoadSWSetun",             Test10_LoadSWSetun);
}

/* ============================================================================
 * ТОЧКА ВХОДА
 * ============================================================================ */

int main(int argc, char *argv[])
{
    int rc;

    register_all_tests();

    if (argc > 1) {
        /* Запуск конкретного теста по имени */
        printf("Running single test: %s\n", argv[1]);
        rc = ut_run_test(argv[1]);
    } else {
        /* Запуск всех тестов */
        rc = ut_run_all_tests();
    }

    ut_print_summary();

    return (rc > 0) ? 1 : 0;
}
