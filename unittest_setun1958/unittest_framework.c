/**
 * unittest_framework.c
 *
 * Реализация легковесного unit-test framework.
 */

#include "unittest_framework.h"
#include <unistd.h>

/* ============================================================================
 * ГЛОБАЛЬНОЕ СОСТОЯНИЕ
 * ============================================================================ */

jmp_buf ut_jump_buffer;
volatile int ut_jmp_ready = 0;
volatile int ut_assert_failed = 0;
const char *ut_current_test = NULL;

static struct {
    char name[UT_NAME_LEN];
    ut_test_func_t func;
} ut_tests[UT_MAX_TESTS];

static int ut_test_count = 0;
static int ut_passed = 0;
static int ut_failed = 0;
static int ut_skipped = 0;

/* ============================================================================
 * ОБРАБОТЧИК СИГНАЛОВ
 * ============================================================================ */

static void ut_signal_handler(int sig)
{
    const char *sig_name = "UNKNOWN";
    switch (sig) {
    case SIGSEGV: sig_name = "SIGSEGV (Segmentation fault)"; break;
    case SIGABRT: sig_name = "SIGABRT (Abort)"; break;
    case SIGFPE:  sig_name = "SIGFPE  (Floating point exception)"; break;
    case SIGILL:  sig_name = "SIGILL  (Illegal instruction)"; break;
    case SIGBUS:  sig_name = "SIGBUS  (Bus error)"; break;
    }

    fprintf(stderr,
        UT_COLOR_RED "\n[SIGNAL] %s caught during test '%s'" UT_COLOR_RESET "\n",
        sig_name, ut_current_test ? ut_current_test : "<unknown>");

    if (ut_jmp_ready) {
        longjmp(ut_jump_buffer, sig + 100);  /* уникальный код для сигнала */
    }
    /* fallback: если jmp не готов, восстанавливаем обработчик и умираем */
    signal(sig, SIG_DFL);
    raise(sig);
}

void ut_setup_signal_handlers(void)
{
    signal(SIGSEGV, ut_signal_handler);
    signal(SIGABRT, ut_signal_handler);
    signal(SIGFPE,  ut_signal_handler);
    signal(SIGILL,  ut_signal_handler);
    signal(SIGBUS,  ut_signal_handler);
}

void ut_restore_signal_handlers(void)
{
    signal(SIGSEGV, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE,  SIG_DFL);
    signal(SIGILL,  SIG_DFL);
    signal(SIGBUS,  SIG_DFL);
}

/* ============================================================================
 * РЕГИСТРАЦИЯ ТЕСТОВ
 * ============================================================================ */

void ut_register_test(const char *name, ut_test_func_t func)
{
    if (ut_test_count >= UT_MAX_TESTS) {
        fprintf(stderr, UT_COLOR_YELLOW
            "[WARNING] Max tests (%d) reached, skipping '%s'\n" UT_COLOR_RESET,
            UT_MAX_TESTS, name);
        return;
    }
    if (name == NULL || func == NULL) {
        fprintf(stderr, UT_COLOR_YELLOW
            "[WARNING] Cannot register NULL test\n" UT_COLOR_RESET);
        return;
    }
    strncpy(ut_tests[ut_test_count].name, name, UT_NAME_LEN - 1);
    ut_tests[ut_test_count].name[UT_NAME_LEN - 1] = '\0';
    ut_tests[ut_test_count].func = func;
    ut_test_count++;
}

void ut_reset(void)
{
    ut_test_count = 0;
    ut_passed = 0;
    ut_failed = 0;
    ut_skipped = 0;
}

/* ============================================================================
 * ЗАПУСК ТЕСТОВ
 * ============================================================================ */

static int ut_execute_single(const char *name, ut_test_func_t func)
{
    clock_t start, end;
    double elapsed_ms;
    int jmp_result;

    ut_current_test = name;
    ut_assert_failed = 0;
    ut_jmp_ready = 1;

    start = clock();
    jmp_result = setjmp(ut_jump_buffer);

    if (jmp_result == 0) {
        /* Первый вход: запускаем тест */
        func();
        end = clock();
        elapsed_ms = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;

        if (ut_assert_failed) {
            fprintf(stderr,
                UT_COLOR_RED "  [FAIL] %-45s  (%.2f ms)" UT_COLOR_RESET "\n",
                name, elapsed_ms);
            ut_failed++;
            return 1;
        }

        printf(UT_COLOR_GREEN "  [PASS] %-45s  (%.2f ms)" UT_COLOR_RESET "\n",
               name, elapsed_ms);
        ut_passed++;
        return 0;
    }
    else {
        /* Возврат из longjmp (сигнал или assert) */
        end = clock();
        elapsed_ms = 1000.0 * (double)(end - start) / CLOCKS_PER_SEC;

        fprintf(stderr,
            UT_COLOR_RED "  [FAIL] %-45s  (%.2f ms)  [signal/assert]" UT_COLOR_RESET "\n",
            name, elapsed_ms);
        ut_failed++;
        return 1;
    }
}

int ut_run_all_tests(void)
{
    int i;
    int failed_count = 0;

    if (ut_test_count == 0) {
        printf(UT_COLOR_YELLOW "[WARNING] No tests registered\n" UT_COLOR_RESET);
        return 0;
    }

    printf("\n" UT_COLOR_BLUE "========================================" UT_COLOR_RESET "\n");
    printf(UT_COLOR_BLUE "  UNIT TEST RUNNER  (total: %d)" UT_COLOR_RESET "\n", ut_test_count);
    printf(UT_COLOR_BLUE "========================================" UT_COLOR_RESET "\n\n");

    ut_setup_signal_handlers();

    for (i = 0; i < ut_test_count; i++) {
        failed_count += ut_execute_single(ut_tests[i].name, ut_tests[i].func);
    }

    ut_restore_signal_handlers();
    ut_jmp_ready = 0;

    return failed_count;
}

int ut_run_test(const char *name)
{
    int i;
    for (i = 0; i < ut_test_count; i++) {
        if (strcmp(ut_tests[i].name, name) == 0) {
            ut_setup_signal_handlers();
            int rc = ut_execute_single(ut_tests[i].name, ut_tests[i].func);
            ut_restore_signal_handlers();
            ut_jmp_ready = 0;
            return rc;
        }
    }
    fprintf(stderr,
        UT_COLOR_YELLOW "[SKIP] Test '%s' not found\n" UT_COLOR_RESET, name);
    ut_skipped++;
    return 1;
}

void ut_print_summary(void)
{
    int total = ut_passed + ut_failed + ut_skipped;

    printf("\n" UT_COLOR_BLUE "----------------------------------------" UT_COLOR_RESET "\n");
    printf(UT_COLOR_BLUE "  SUMMARY" UT_COLOR_RESET "\n");
    printf(UT_COLOR_BLUE "----------------------------------------" UT_COLOR_RESET "\n");
    printf("  Total:   %3d\n", total);
    printf(UT_COLOR_GREEN "  Passed:  %3d" UT_COLOR_RESET "\n", ut_passed);
    printf(UT_COLOR_RED   "  Failed:  %3d" UT_COLOR_RESET "\n", ut_failed);
    if (ut_skipped > 0)
        printf(UT_COLOR_YELLOW "  Skipped: %3d" UT_COLOR_RESET "\n", ut_skipped);
    printf(UT_COLOR_BLUE "----------------------------------------" UT_COLOR_RESET "\n\n");

    if (ut_failed == 0 && ut_skipped == 0) {
        printf(UT_COLOR_GREEN "  ALL TESTS PASSED!" UT_COLOR_RESET "\n\n");
    } else if (ut_failed > 0) {
        printf(UT_COLOR_RED "  SOME TESTS FAILED!" UT_COLOR_RESET "\n\n");
    }
}
