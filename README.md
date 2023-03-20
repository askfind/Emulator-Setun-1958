Эмулятор троичной машины 'СЕТУНЬ' SETUN-1958 VM
===============================================

A virtual machine of ternary computer Setun, also known as "Small Automatic Digital Machine" written on C.

Виртуальная машина троичной цифровой машина "Сетунь" также известная как МЦВМ "Сетунь"), написанный на языке программиования C.

# История
- Дата создания:            01.11.2018
- Дата редактирования:      20.03.2022
- Версия:                   1.85
-
- Автор:                    Vladimir V.
- E-mail:                   askfind@ya.ru

# Использование

## Обработка событий нажатия клавиш клавиатуры


**При нажатии клавиши 'Q' или 'q' работа эмулятора останавливается**

Вывод дампов зон ферритовой памяти и состояние регистров машины "СЕТУНЬ".


```shell
$ ./setun1958emu

[ Start Setun-1958 ]

[ QUIT Work ]

[ Step = 1128271 ]

[ Dump FRAM Setun-1958: ]
                         Zone = 1

WW WX  Z 11 Z0        02 03  Z 31 0X
   WY  Z 0W 31           04  Z 22 30
WZ W0  Z 2Z Y3        1W 1X  Z 13 Z0
   W1  Z 13 20           1Y  1 WW Y2
W2 W3  Z 30 Y3        1Z 10  Z 2Y ZX
   W4  Z 34 0X           11  1 1Y 13
XW XX  Z 11 Z0        12 13  Z 31 Z0
   XY  Z 0W 31           14  0 04 X4
XZ X0  Z 22 Y3        2W 2X  Z 40 Z0
   X1  Z 13 20           2Y  Z 2X ZX
X2 X3  Z 44 Y3        2Z 20  Z 40 0X
   X4  Z 4X 0X           21  Z 41 0X
YW YX  Z 13 Z0        22 23  Z 10 Z0
   YY  Z 2Z 30           24  Z 33 0X
YZ Y0  1 WW Y2        3W 3X  0 04 XY
   Y1  Z 2Y ZX           3Y  Z 41 Z0
Y2 Y3  1 Y0 13        3Z 30  Z 14 ZX
   Y4  Z 11 Z0           31  Z 41 0X
ZW ZX  Z 4Y 0X        32 33  1 4Y 1X
   ZY  Z 10 Z0           34  Z 22 30
ZZ Z0  1 WW X2        4W 4X  1 40 00
   Z1  Z 14 ZX           4Y  Z 2Z 30
Z2 Z3  1 Z0 13        4Z 40  Z 3W Y3
   Z4  Z 4Y Z0           41  Z 13 Z0
0W 0X  Z 14 ZX        42 43  1 WW 3W
   0Y  1 ZX 13           44  Z X1 10
0Z 00  Z 40 0X        KC     0 00 Z1
   01  Z 10 Z0               1 WZ W3

  K  : [-+--+0--0], Z2YZX, (-5277)
  F  : [00+-0], 01X, (6)
  C  : [--0++], ZX4, (-104)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [------------------], WWWWWWWWW, (-193710244)
  R  : [000000000000000000], 000000000, (0)
  MB : [+00-], 3Z, (26)


[ Stop Setun-1958 ]

```


## Сборка эмулятора SETUN-1958

```shell
$ make
gcc -o emu -g emusetun.c
```

## Запуск проверочной программы ТЕСТ1 в эмуляторе SETUN-1958

```shell

$ ./setun1958emu

[ Start Setun-1958 ]

TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
...


```

## Вывод списка команд эмулятора SETUN-1958

```shell
./setun1958emu --help

usage: ./setun1958emu [options] FILE SCRIPT(s)...
	--version : version software setun1958emu
	--load : load software setun1958emu
	--convert : convert software file.lst to paper.txt setun1958emu
	--dump : dump zone from file.txs setun1958emu
	--debug : view step  setun1958emu
	--breakpoint : view stop setun1958emu
	--test : number test setun1958emu

```

## Печать файла 'test1-fram-zone-0.txs' исходного кода программы машины "Сетунь"

```shell
./setun1958emu --dump ./software/test1/test1-fram-zone-0.txs


Read commands from file.txs: ./software/test1/test1-fram-zone-0.txs

[ Dump FRAM Setun-1958: ]
Zone = 0

WW WX  0 1Y Z0        02 03  Z 00 00
   WY  0 11 0X           04  Z 01 00
WZ W0  1 01 X0        1W 1X  0 Z0 00
   W1  1 1W X4           1Y  0 03 00
W2 W3  1 1W XY        1Z 10  0 0X 00
   W4  0 03 Z0           11  0 00 00
XW XX  0 2X 30        12 13  0 00 0Y
   XY  0 12 Y3           14  1 ZW 22
XZ X0  Z WX 31        2W 2X  0 00 00
   X1  0 1X Y0           2Y  0 00 00
X2 X3  0 12 33        2Z 20  0 00 0Y
   X4  0 12 Y3           21  1 ZW 22
YW YX  0 1Y ZX        22 23  0 00 0Y
   YY  0 X0 1X           24  1 YY YX
YZ Y0  0 Y4 13        3W 3X  0 00 00
   Y1  0 04 Z0           3Y  0 00 00
Y2 Y3  0 X0 00        3Z 30  0 00 00
   Y4  0 11 Z0           31  0 00 00
ZW ZX  0 2Z 3Y        32 33  0 00 00
   ZY  0 Z3 10           34  0 00 00
ZZ Z0  0 00 2X        4W 4X  0 00 00
   Z1  0 W0 00           4Y  0 00 00
Z2 Z3  0 10 ZX        4Z 40  0 00 00
   Z4  0 WY 10           41  0 00 00
0W 0X  1 1Z XX        42 43  0 00 00
   0Y  Z 1W XX           44  0 00 00
0Z 00  1 W3 00        KC     0 00 01
   01  0 WX 00               Z Z2 1W

 i=54

KC:
: [00000000+], 00001, (1)
: [-0-+-0+--], ZZ21W, (-7123)
-KC = 0-KC:
: [00000000-], 0000Z, (-1)
: [+0+-+0-++], 11YZ4, (7123)

```

## Запуск теста №1 для проверки функций эмулятора

```shell
./setun1958emu --test 1

1.4. Логическая операция SUM FULL
 C, P1 = A + B + P0
 a,b,p0,c,p1 {-1,-1,-1}-> 0,-1
 a,b,p0,c,p1 {-1,-1, 0}-> 1,-1
 a,b,p0,c,p1 {-1,-1, 1}->-1, 0
 a,b,p0,c,p1 {-1, 0,-1}-> 1,-1
 a,b,p0,c,p1 {-1, 0, 0}->-1, 0
 a,b,p0,c,p1 {-1, 0, 1}-> 0, 0
 a,b,p0,c,p1 {-1, 1,-1}->-1, 0
 a,b,p0,c,p1 {-1, 1, 0}-> 0, 0
 a,b,p0,c,p1 {-1, 1, 1}-> 1, 0
 a,b,p0,c,p1 { 0,-1,-1}-> 1,-1
 a,b,p0,c,p1 { 0,-1, 0}->-1, 0
 a,b,p0,c,p1 { 0,-1, 1}-> 0, 0
 a,b,p0,c,p1 { 0, 0,-1}->-1, 0
 a,b,p0,c,p1 { 0, 0, 0}-> 0, 0
 a,b,p0,c,p1 { 0, 0, 1}-> 1, 0
 a,b,p0,c,p1 { 0, 1,-1}-> 0, 0
 a,b,p0,c,p1 { 0, 1, 0}-> 1, 0
 a,b,p0,c,p1 { 0, 1, 1}->-1, 1
 a,b,p0,c,p1 { 1,-1,-1}->-1, 0
 a,b,p0,c,p1 { 1,-1, 0}-> 0, 0
 a,b,p0,c,p1 { 1,-1, 1}-> 1, 0
 a,b,p0,c,p1 { 1, 0,-1}-> 0, 0
 a,b,p0,c,p1 { 1, 0, 0}-> 1, 0
 a,b,p0,c,p1 { 1, 0, 1}->-1, 1
 a,b,p0,c,p1 { 1, 1,-1}-> 1, 0
 a,b,p0,c,p1 { 1, 1, 0}->-1, 1
 a,b,p0,c,p1 { 1, 1, 1}-> 0, 1

--- END TEST #1 ---

```

## Выполнение программы в эмуляторе SETUN-1958 в режиме вывода отладочной информации

```shell

./setun1958emu --debug --step 100

...
С: [0-000], 0X0, (-27)         -> zone[ 0] : ZWX31, (-9692)
A*: [+0+-0], 11X, (87)          -> zone[ 1] : 0102X, (744)
 k6..8[+00]: (A*)=>(S)

  K  : [----0+00+], ZWX31, (-9692)
  F  : [0--00], 0W0, (-36)
  C  : [0-00+], 0X1, (-26)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [00], 0, (0)
  S  : [00+00+--0000000000], 031W00000, (14644152)
  R  : [000000000000000000], 000000000, (0)
  MB : [0+0-], 1Z, (8)

  K  : [----0+00+], ZWX31, (-9692)
  F  : [0--00], 0W0, (-36)
  C  : [0-00+], 0X1, (-26)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [00], 0, (0)
  S  : [00+00+--0000000000], 031W00000, (14644152)
  R  : [000000000000000000], 000000000, (0)
  MB : [0+0-], 1Z, (8)


[ Step = 100 : Break work Setun-1958 ]
```


# Примечание

*`ptr1`,`ptr2`,`ptp1`,`tty1` - это виртуальные  устройства ввода и вывода*

Device 	Direction 	Description

PTR1: 	устройство ввода бумажной перфоленты, фототрансмиттер №1,
PTR2: 	устройство ввода бумажной перфоленты, фототрансмиттер №2,
PTP1: 	устройство вывода на перфоленту
TTY1: 	устройство вывода на телетайп или печатующее устройство

# Ссылки

 1. *Materials on ternary computer science* - <http://ternarycomp.cs.msu.su/>
 2. *Emulator of the first ternary Soviet "Setun"* - <http://trinary.su/projects/>
 3. *Unique Setun based on the ternary code* - <https://habr.com/ru/company/ua-hosting/blog/273929/>
 4. *Group users in social network VK* - <https://vk.com/setunsu/>
 5. *Group users in social network Telegram* - <https://t.me/setun_1958/>

# Contributing

Everybody is invited and welcome to contribute to Setun VM.

Приглашаем всех желающих внести свой посильный вклад в эмулятор SETUN-1958
