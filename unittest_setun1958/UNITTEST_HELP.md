# Unit Test Framework для эмулятора МЦВМ «Сетунь-1958»

Легковесный C-фреймворк для модульного тестирования эмулятора троичной машины «Сетунь-1958», реализованный без внешних зависимостей (zero-dependency).

---

## Содержание

- [Быстрый старт](#быстрый-старт)
- [Структура проекта](#структура-проекта)
- [Сборка](#сборка)
- [Запуск тестов](#запуск-тестов)
- [API фреймворка](#api-фреймворка)
- [Написание новых тестов](#написание-новых-тестов)
- [Интеграция с основным Makefile](#интеграция-с-основным-makefile)
- [Выполненные оптимизации](#выполненные-оптимизации)
- [Лицензия](#лицензия)

---

## Быстрый старт

```bash
# 1. Скопируйте файлы фреймворка в каталог с исходниками эмулятора
cp unittest_framework.h unittest_framework.c unittest_main.c Makefile.unittest ./

# 2. Соберите тестовый runner
make -f Makefile.unittest unittest

# 3. Запустите все тесты
./unittest

# 4. Запустите конкретный тест
./unittest Test3_Setun_Opers
```

---

## Структура проекта

```
.
├── emusetun.c              # Основной код эмулятора
├── emusetun.h              # Заголовок эмулятора
├── emusetun_test.c         # Существующие тестовые функции (printf-based)
├── emusetun_test.h         # Заголовок тестов
│
├── unittest_framework.h    # ← Заголовок фреймворка (макросы assert, API)
├── unittest_framework.c    # ← Реализация runner (setjmp, сигналы, отчёт)
├── unittest_main.c         # ← Точка входа (регистрация 10 Test* функций)
├── Makefile.unittest       # ← Makefile для сборки runner
│
├── sum_t_optimized.c       # Оптимизированная табличная версия sum_t
├── pow3_optimized.c        # Табличная pow3() + t3_to_d10_optimized
├── d10_to_t3_optimized.c   # Реализация d10_to_t3 (была заглушка)
└── integration_example.c   # Примеры интеграции оптимизаций
```

---

## Сборка

### Вариант A: отдельный Makefile

```bash
make -f Makefile.unittest unittest
```

### Вариант B: интеграция в основной Makefile

Добавьте в ваш `Makefile`:

```makefile
# --- Unit Test Runner ---
UT_SRC = unittest_framework.c unittest_main.c
UT_TARGET = unittest

.PHONY: unittest test test1 test2 test3 test4 test5 test6 test7 test8 test9 test10

unittest: $(SOURCES) $(UT_SRC) unittest_framework.h
	$(CC) $(CFLAGS) $(SOURCES) $(UT_SRC) -o $(UT_TARGET) $(LDFLAGS)

test: unittest
	./$(UT_TARGET)

test1: unittest
	./$(UT_TARGET) Test1_Ariphmetic_Ternary
test2: unittest
	./$(UT_TARGET) Test2_Opers_TRITS_32
test3: unittest
	./$(UT_TARGET) Test3_Setun_Opers
test4: unittest
	./$(UT_TARGET) Test4_Setun_Opers
test5: unittest
	./$(UT_TARGET) Test5_Setun_Load
test6: unittest
	./$(UT_TARGET) Test6_Setun_Load
test7: unittest
	./$(UT_TARGET) Test7_Setun_Load
test8: unittest
	./$(UT_TARGET) Test8_Setun_Electrified_Typewriter
test9: unittest
	./$(UT_TARGET) Test9_Perforatin_Paper_Line
test10: unittest
	./$(UT_TARGET) Test10_LoadSWSetun
```

### Вариант C: ручная сборка

```bash
gcc -Wall -Wextra -O2 -std=gnu11 \
    emusetun.c emusetun_test.c \
    unittest_framework.c unittest_main.c \
    -o unittest -lm
```

---

## Запуск тестов

### Все тесты

```bash
./unittest
```

Пример вывода:

```
========================================
  UNIT TEST RUNNER  (total: 10)
========================================

  [PASS] Test1_Ariphmetic_Ternary              (0.52 ms)
  [PASS] Test2_Opers_TRITS_32                  (1.23 ms)
  [PASS] Test3_Setun_Opers                     (2.87 ms)
  [PASS] Test4_Setun_Opers                     (0.41 ms)
  [PASS] Test5_Setun_Load                      (5.12 ms)
  [PASS] Test6_Setun_Load                      (4.98 ms)
  [PASS] Test7_Setun_Load                      (5.03 ms)
  [FAIL] Test8_Setun_Electrified_Typewriter    (0.00 ms)  [signal/assert]
  [PASS] Test9_Perforatin_Paper_Line           (1.87 ms)
  [PASS] Test10_LoadSWSetun                    (3.45 ms)

----------------------------------------
  SUMMARY
----------------------------------------
  Total:    10
  Passed:    9
  Failed:    1
----------------------------------------

  SOME TESTS FAILED!
```

### Один тест по имени

```bash
./unittest Test3_Setun_Opers
```

### Через Makefile

```bash
make test3        # Запуск Test3_Setun_Opers
make test         # Запуск всех тестов
```

---

## API фреймворка

### Макросы assert

Все макросы при срабатывании выводят файл, строку, ожидаемое/фактическое значение и выполняют `longjmp` обратно в runner (тест помечается FAILED, runner продолжает со следующего теста).

| Макрос | Описание | Пример |
|--------|----------|--------|
| `UT_ASSERT(cond)` | Проверка условия | `UT_ASSERT(x > 0)` |
| `UT_ASSERT_TRUE(cond)` | Псевдоним `UT_ASSERT` | `UT_ASSERT_TRUE(ptr != NULL)` |
| `UT_ASSERT_FALSE(cond)` | Проверка ложности | `UT_ASSERT_FALSE(err)` |
| `UT_ASSERT_EQ(expected, actual)` | Равенство целых | `UT_ASSERT_EQ(5, result)` |
| `UT_ASSERT_NEQ(expected, actual)` | Неравенство | `UT_ASSERT_NEQ(0, len)` |
| `UT_ASSERT_NULL(ptr)` | Указатель == NULL | `UT_ASSERT_NULL(bad_ptr)` |
| `UT_ASSERT_NOT_NULL(ptr)` | Указатель != NULL | `UT_ASSERT_NOT_NULL(mem)` |
| `UT_FAIL(msg)` | Принудительный fail | `UT_FAIL("unreachable code")` |

### Программный API

```c
#include "unittest_framework.h"

/* Регистрация теста */
ut_register_test("MyTest", my_test_func);

/* Запуск всех зарегистрированных тестов */
int failed = ut_run_all_tests();

/* Запуск одного теста по имени */
ut_run_test("MyTest");

/* Вывод сводной статистики */
ut_print_summary();

/* Сброс всех регистраций */
ut_reset();
```

### Защита от сигналов

Каждый тест выполняется в контексте `setjmp`. Перехватываемые сигналы:

- `SIGSEGV` — segmentation fault
- `SIGABRT` — abort
- `SIGFPE`  — floating point exception
- `SIGILL`  — illegal instruction
- `SIGBUS`  — bus error

Если тест вызывает любой из этих сигналов, runner отмечает его как `[FAIL] ... [signal/assert]` и **продолжает** выполнение оставшихся тестов. Это критически важно для тестирования низкоуровневого кода эмулятора.

---

## Написание новых тестов

### Шаг 1: создайте тестовую функцию

```c
#include "unittest_framework.h"
#include "emusetun.h"

void Test_My_New_Feature(void)
{
    trs_t a = smtr("+000000000");
    trs_t b = smtr("-000000000");
    trs_t sum;

    sum = add_trs(a, b);

    /* Проверяем, что +1 + (-1) = 0 */
    UT_ASSERT_EQ(0, get_trit(sum, 0));
    UT_ASSERT_EQ(0, sgn_trs(sum));
}
```

### Шаг 2: зарегистрируйте в `unittest_main.c`

```c
void Test_My_New_Feature(void);  /* прототип */

static void register_all_tests(void)
{
    /* ... существующие тесты ... */
    ut_register_test("Test_My_New_Feature", Test_My_New_Feature);
}
```

### Шаг 3: соберите и запустите

```bash
make unittest
./unittest Test_My_New_Feature
```

---

## Интеграция assert в существующие тесты

Тесты из `emusetun_test.c` сейчас используют `printf` для ручной проверки. Чтобы превратить их в автоматические unit-тесты, замените `printf` на `UT_ASSERT_*`:

**Было:**
```c
void Test1_Ariphmetic_Ternary(void)
{
    int8_t a, c;
    a = 1;
    not_t(&a, &c);
    printf("NOT(+1) = %d\n", c);  /* глазами смотрим, что -1 */
}
```

**Стало:**
```c
void Test1_Ariphmetic_Ternary(void)
{
    int8_t a, c;
    a = 1;
    not_t(&a, &c);
    UT_ASSERT_EQ(-1, c);  /* автоматическая проверка */
}
```

При срабатывании:
```
[ASSERT EQ FAIL] emusetun_test.c:275
  expected: -1
  actual:   0
```

---

## Выполненные оптимизации

В рамках подготовки unit-test framework были оптимизированы критические функции эмулятора:

### 1. `sum_t` — таблица переходов 3×3×3

- **Было:** `switch` по 7 case, O(ветвей), непредсказуемые branch-miss.
- **Стало:** 2 обращения к константным массивам `SUM_T_S[3][3][3]` и `SUM_T_P1[3][3][3]`, 0 условных переходов, O(1).
- **Файл:** `sum_t_optimized.c`

### 2. `pow3` — таблица поиска

- **Было:** цикл с умножением, O(n).
- **Стало:** `POW3_TABLE[20]`, 1 обращение к L1-кэшу.
- **Файл:** `pow3_optimized.c`

### 3. `t3_to_d10` — накопление степени

- **Было:** вызов `pow3(i)` на каждой итерации → O(n²).
- **Стало:** переменная `p *= 3.0` в цикле → O(n), 0 вызовов функций.
- **Файл:** `pow3_optimized.c`

### 4. `d10_to_t3` — реализация заглушки

- **Было:** `return smtr("000000000000000000");` — всегда возвращала 0.
- **Стало:** полноценное преобразование float → balanced ternary через нормализованный остаток, прямая манипуляция битами `t1`/`t0` без вызова `set_trit()`.
- **Файл:** `d10_to_t3_optimized.c`

---

## Конфигурация

### Максимальное количество тестов

```c
/* Перед #include "unittest_framework.h" */
#define UT_MAX_TESTS 512
#include "unittest_framework.h"
```

### Отключение цветного вывода

```bash
./unittest            # цветной вывод (по умолчанию)
./unittest 2>&1 | cat # через pipe — авто-отключение (если не TTY)
```

Или при компиляции:
```bash
gcc -DNO_COLOR ...
```

---

## Требования

- **Компилятор:** GCC, Clang (C11/C99)
- **OS:** Linux, macOS, любая POSIX-совместимая
- **Зависимости:** только стандартная библиотека C (`stdio.h`, `setjmp.h`, `signal.h`, `time.h`)
- **Внешние библиотеки:** не требуются

---

## Лицензия

Фреймворк и оптимизации распространяются на тех же условиях, что и оригинальный проект эмулятора «Сетунь-1958».

---

**Автор:** AI-ассистент (оптимизация и unit-test framework для проекта Emulator-Setun-1958)  
**Дата:** 04.08.2026
