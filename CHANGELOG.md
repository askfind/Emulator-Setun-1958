# Changelog

## 31.03.2025 ver.2.07.1
 - [Х] Добавлнение исходных файлов троичных библиотек для СЕТУНЬ.

## 29.03.2025 ver.2.07
 - [Х] Edit /* Троичное умножение тритов */
trs_t mul_trs(trs_t a, trs_t b)
 - [Х] Добавлен пример из книги book3 на странице 87.

## 26.03.2025 ver.2.06
 - [Х] . FIX Печать русского символа 'Л'

 /**
 * Печать на электрифицированную пишущую машинку
 * 'An electrified typewriter'
 */
void electrified_typewriter(trs_t t, uint8_t local)


## 25.03.2025 ver.2.04
 - [Х] Отладка библиотеки программ ИП-Т.

## 28.01.2025 ver.2.04
 - [Х] Опции компилятора -std=gnu11

## 26.01.2025 ver.2.03
 - [Х] Добавить программы библиотеки ИП-2 преобразования 10->3, 3->10 и печать цифровых массивов.
      - [Х] ./software/ip2_10_3
      - [Х] ./software/ip2_3_10
      - [Х] ./software/ip2_ppp
      - [Х] ./software/test_print_lat
      - [Х] ./software/test_print_rus
 - [Х] Отладка преобразования 10->3, 3->10.

## 25.01.2025 ver.2.02
 - [Х] Изменён формат хранения магнитного барабана drum.bak.
 - [Х] FIX ERR - Запись и чтение файла хранения магнитного барабана drum.bak.
 
## 24.01.2025 ver.2.01
 - [Х] Добавить в файл makefile сборку файлов тестирования функций эмулятора 'emusetun_test.c', 'emusetun_test.h'.
 - [Х] Изменён формат хранения магнитного барабана drum.bak.
 - [Х] Протестировать DRUM запись и чтение.
 - [Х] FIX ERR - запись и чтение зон в магнитном барабне DRUM.
 - [Х] FIX ERR команда чтения зон из магнитного барабна DRUM в FRAM.
 
## 22.01.2025 ver.2.00
 - [X] Отладка подпрограмм ввода и вывода ИП-2.

## 18.01.2025 ver.2.00
 - [Х}] FIX ERR - закрытие не открытого файла script.sst.
 - [Х}] FIX ERR - сохранение памяти DRUM после команды меню quit.
 - [X] FIX ERR - запись и чтение в память DRUM.
 - [X] В пошаговой отладке с включённым debug=on выводилось два раза
      состояние машины.

## 14.01.2025 ver.1.99
 - [X] Чтение из каталога './script' cкрипт-файла 'script.sst'.
      - [X] Выполнить парсинг строк из cкрипт-файла.
      - [X] Печать примечание при остонове машины.
      - [X ] Печать статуса и следущего адреса для продолжения работы машины (замена оператора).

## 10.01.2025 ver.1.98
 - [X] Добавить копирование Cкрипт-файл 'Filename.sst' для управления дальнейшего запуска машины после останова.

## 09.01.2025 ver.1.97
 - [Х] Удалить текстовый интерсфейс 'term.c','term.h'

## 09.01.2025 ver.1.96
 - [Х] Редактирование вывода help.

## 15.11.2024 ver.1.94
 - [Х] Редактирование стиля исходных файлов проекта.
 - [Х] Перенести все тести из emusetun.c.
 - [Х] Удалить из меню команд запуск тестов.

## 16.09.2023 ver.1.92
 - [Х] Ревизия исходного проекта.

## 24.08.2023 ver.1.91
 - [Х] Добавить останов breakpoint по адресу DRUM магнитного барабана.
 - [Х] Отладка программы пример 3 из книги с ИП-2.

## 23.08.2023 ver.1.90.1
 - [X] Исправлен останов breakpoint по адресу FRAM.

## 22.08.2023 ver.1.90
 - [X] Реализована команда 'debug', 'step'.
 - [X] Изменена команда 'view' и добавлен вывод статуса Setun-1958.
 - [X] Изменен файл описания 'README.md'.

 ## 21.08.2023
- [X] Исправлена ошибка перевод троичного числа в число с фиксированной точкой.

## 20.08.2023
 - [X] Вывод по команде view регистров 'S','R' значения числа с фиксированной точкой.

## 17.04.2023
 - [X] Добавлена команда 'calc' Калькулятор "MK-Setun-01", с использованием ИП-2.

## 30.03.2023
 - [X] Добавлен командный интерпретатор виртуального пульта управления "Сетунь".

## 15.03.2023
 - [X] FIX ошибка чтения символов с перфоленты
      Команда "k6..8[-00]: Ввод в Фа* - Вывод из Фа" чтения символов кодов из FT1,FT2.

## 12.03.2023
 - [X] Команда "k6..8[-00]: Ввод в Фа* - Вывод из Фа" чтения символов кодов из FT1,FT2.
 - [X] Команда "k6..8[-00]: Ввод в Фа* - Вывод из Фа" вывода в файл виртуального телетайпа tty1.
 - [X] добавить параметр командной строки для печати дампа файла Filename.txs

## 11.03.2023
 - [X] Команда "k6..8[-00]: Ввод в Фа* - Вывод из Фа" чтения троичных кодов из FT1,FT2.

## 04.05.2022
 - [X] FIX ошибка команды k6..8[-+-] : Норм.(S)=>(A*); (N)=>(S)

## 03.05.2022
 - [X] Запуск теста ur0/01-test.txs

## 01.03.2021
 - [X] Отладка операций '++0' : Умножение  0	(S)=>(R); (A*)(R)=>(S)
 - [X] Отладка операций '+++' : Умножение  1	(S)+(A*)(R)=>(S)
 - [X] Отладка операций '++-' : Умножение -1	(A*)+(S)(R)=>(S)")

## 23.02.2021
 - [X] Отладка операций '++-'
 - [X] Отладка операций '-00' : Ввод в Фа* - Вывод из Фа*

## 21.02.2021
 - [x] Уточнение описания магнитного барабана DRUM
 - [x] Уточнение модели памяти барабана mem_drum[]
 - [X] Отладка zone_drum_to_index()
 - [X] Отладка clean_drum()
 - [X] Отладка clean_drum()
 - [X] Отладка ld_drum( trs_t ea );
 - [X] Отладка st_drum( trs_t ea, trs_t v );
 - [X] Отладка st_drum( trs_t ea, trs_t v );
 - [X] Удалить row_drum_to_index
 - [X] Добавить вывод страницы view_drum()
 - [X] Отладка dump_drum()
 - [X] Отладка oper k6..8[-0-] : (Мд*)=>(Фа*)
 - [X] Отладка oper k6..8[-0+] : (Фа*)=>(Мд*)
 - [X] Добавлены разряды переполнения ph1,ph2 для S
 - [X] Изменёна операция сдвига с очисткой ph1,ph2 для S

## 16.02.2021
 - [X] Отладка операций +0-.

## 11.02.2021
 - [X] Отладка операций +00, +0+.
 - [X] Добавить вывод fram по адресам.

## 07.02.2021
 - [X] Исправить ошибку записи в память FRAM.

## 30.01.2021
 - [ ] Исправить интерпретацию K1.9 команда.

27.01.2021
 - [X] Добавить точное описание реализации команд машины "Сетунь".

## 11.07.2020
 - [x] Порядок в проекте и сопровождении.
 - [X] копия документации, книг
 - [X] копия троичных проектов

 - [x] int8_t execute_trs( trs_t addr, trs_t oper )

 - [X] для С(5) = -1 выполнить 2-раза старшей половине A(9:18) и сделать inc C
 - [X] copy_trit(&MR,&S); //TODO исправить копирование
 - copy_trit(&MR,&R); //TODO исправить
 - add S = shift_trs(S,trit2dec(MR));

 - [x] main() ОТЛАДКА

 - [x] проверить необходимость void arithmetic_trs( trs_t k ) {

 - [ ] int8_t execute_trs( trs_t addr, trs_t oper ) { для С(5) = -1 выполнить 2-раза старшей половине A(9:18) и сделать
  inc C

 - [ ] FILE *file - планировать ввод/вывод

## 05.07.2020
 - [X]  читать FRAM k9
 - [X]  выполнить control, execute

## 17.06.2020
 - [X] Запустить. Проверить. Посмотреть коде.

## 22.02.2020
 - [X] Отладка FRAM

## 21.02.2020
 - [X] ~ void or_trs(trs_t *x, trs_t *y, trs_t *r)
 - [X] ~ void xor_trs(trs_t *x, trs_t *y, trs_t *r)
 - [X] ~ int not_trs(trs_t *x)
 - [X] ~ int not_trs(trs_t *x)
 - [X] ~ int neg_trs(trs_t *t)
 - [X] ~ int dec_trs(trs_t *t)
 - [X] ~ int mul_trs(trs_t *a, trs_t *b, trs_t *c)
 - [X] dump_fram
 - [X] dump_drum

## 13.02.2020
 - [X] ver 3 Заменить на тип данных int регистры машины. execute_k

## 10.02.2020
 - [X] Запустить. Проверить. Посмотреть коде.

## 09.02.2020
 - [X] ver 2 Заменить на тип данных int регистры машины. execute_k

## 07.02.2020
 - [X] execute_trs

## 06.02.2020
 - [X] execute_trs - проверить определения регистров машины. - изменить наименование регистров. - Функция читать код
  машина из памяти рег. С -счетчик команд A = FRAM(C). - Выполнить безусловный переход.

## 05.02.2020
 - [X] Конспект книг Сетунь.
 - [X] Выполнение команд K.

## 04.02.2020
 - [X] Опр. Троичный код 9 или 18 троичных разрядов.
 - [X] Опр. Ячейка памяти машины или один 18 длинный код, или два 9 коротких кода.
 - [X] photo input - фотосчитыватель ленточный
 - [X] AlphanumericPrintEUM - Устройство вывода буквенно-цифровой информации на печать
 - [X] Инициализация регистров машины, памяти машины.
 - [X] Выполнение команд K.

## 02.11.2018
 - [X] ссылка на источники
 - [X] Хосе
 - [X] МЦВМ
 - [X] Тесты реализации and_t, or_t, xor_t, and_trs, or+trs, xor_trs
 - [X] В комментарии добавить таблицы истинности для троичных разрядов
 - [X] добавить возврат переполнения операций
