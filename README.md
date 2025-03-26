Эмулятор троичной цифровой электронной машины 'СЕТУНЬ' SETUN-1958 VM
====================================================================

A virtual machine of ternary computer Setun, also known as "Small Automatic Digital Machine" written on C.

Виртуальная машина троичной цифровой машина "Сетунь" также известная как МЦВМ "Сетунь"), написанный на языке программиования C.

## 1. История

- Дата создания:            01.11.2018
- Дата редактирования:      26.03.2025
- Версия:                   2.06
- Автор:                    Vladimir V.
- E-mail:                   askfind@ya.ru


## 2. Сборка эмулятора SETUN-1958

```shell
$ cd ./Emulator-Setun-1958
$ make
gcc -v -std=gnu11 -o setun1958emu -g emusetun_test.c emusetun.c

$ ls -al ./setun1958emu
./setun1958emu
```

## 3. Список команд виртуального пудьта управления эмулятора SETUN-1958

```shell
$ ./setun1958emu

 Emulator ternary computer 'Setun-1958':
 Version: 2.06
 Author:  Vladimir V.
 E-mail:  askfind@ya.ru

setun1958emu:
help
 Commands control for setun1958emu:
 dump     [arglist]
 load     [l]    [arglist]
 debug    [d]    [arglist]
 view     [v]
 begin    [b]
 pause    [p]
 run      [r]
 step     [s]    [arglist] 
 break    [br]   [arglist]
 breakmb  [brm]  [arglist]
 reg      [rg]   [arglist]
 fram     [fr]   [arglist]
 drum     [dr]   [arglist]
 help     [h]
 quit     [q]
 calc     [arglist]
 ```
## 3.0. 'calc' - калькулятор 'MK-Setun-01', с использованием подпрограммы ИП-2 для эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
calc 12 / 13

MK-Setun-01
calc: 12.00000000 / 13.00000000 = 0.92307692

TODO: add codes for calculate trits !

setun1958emu:
```

## 3.1. 'dump' -команда печати файла 'test1-fram-zone-0.txs', с исходными кодами программы эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
dump ./software/test1/test1-fram-zone-0.txs

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

## 3.2. 'load' или 'l' - чтение исходных файлов в каталоге './software/test1', с исходными кодами программы, и создание в каталоге './ptr1' файла виртуальной перфоленты 'paper.txt' для эмулятора SETUN-1958 

```shell
$ ./setun1958emu

 Emulator ternary computer 'Setun-1958':
 Version: 1.99
 Author:  Vladimir V.I.
 E-mail:  askfind@ya.ru

setun1958emu:
load ./software/test2 ./ptr1
[ Convert software files to file paper.txt ]

Read file list: ./software/test2/test2.lst

Read file: test2-fram-zone-b.txs

Read file: test2-fram-zone-z.txs

Read file: test2-fram-zone-0.txs

Read file: test2-fram-zone-1.txs


Write file: ./ptr1/paper.txt

Script file source: ./software/test2/script.sst

Script file destation: ./script/script.sst

Copy file source to file destation.

setun1958emu:
begin

setun1958emu:

[ Start Setun-1958 ]

TECT2
<STOP>

[ Script file: 'script.sst' ]
  Addres: [0+++-+], 14Y, (115)
  Note:   Success!
  New address: [000000], 000, (0)
  Action: R

  K  : [0+0+-+--0], 0322X, (2364)
  F  : [-----], ZWW, (-121)
  C  : [+++-+], 14Y, (115)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [+-0-+00+0-+00+++-0], 2Z31Z314X, (82959585), {1.927199}
  R  : [-0+0-00-+0-00-++00], X3XZ3XZ40, (-116427177), {-2.704670}
  MB : [0000], 00, (0)


setun1958emu:
```

## 3.4. 'debug' или 'd' - отладочный режим вывода работы эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
debug 1
switch debug on

setun1958emu:
debug 0
switch debug off
```


## 3.5. 'view' или 'v' - вывод всех регистров и флагов состояния эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
view
[ View registers Setun-1958 ]

  K  : [000000000], 00000, (0)
  F  : [00000], 000, (0)
  C  : [00000], 000, (0)
  W  : [0], 0, (0)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0), {0.000000}
  R  : [000000000000000000], 000000000, (0), {0.000000}
  MB : [0000], 00, (0)

[ Tools ]
  status: wait
  debug: on
  breakpoint: [000+0], 003, (3)
  steps: 0
```

## 3.6. 'pause' или 'p' - остановить выполнение программы эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
pause

setun1958emu:
```

## 3.7. 'run' или 'r' - продолжить работу эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
pause
```

## 3.8. 'step' или 's' - выполнить одну опеарцию  эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
step

setun1958emu:

  K  : [--+++0+00], ZY410, (-7686)
  F  : [+-++0], 1Y3, (66)
  C  : [--+++], ZY4, (-95)
  W  : [0], 0, (0)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [0+-++-+-+--+--+-+-], 1Y222YW22, (34673123)
  MB : [++-0], 4X, (33)
```

## 3.9. 'break' или 'br' - установить адрес останова выполнения программы эмулятора SETUN-1958

```shell
TODO add code 
```

## 3.10. 'reg' или 'rg' - записать троичный код в регистр эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
reg c 0000+
```

## 3.11. 'fram' или 'fr' - вывод зоны ферритовой памяти эмулятора SETUN-1958

 [fram]  [fr] [arglist]
```shell
$ ./setun1958emu

setun1958emu:
fram +

[ Dump FRAM Setun-1958: ]
Zone = 1

WW WX  1 3X X4        02 03  Z 1Y 3X 
   WY  1 13 X4           04  1 1Y 10
WZ W0  1 20 2W        1W 1X  0 10 2X

...

0Z 00  Z 40 03        KC     0 00 0Y
   01  Z 42 YX               1 YY YX 
```

## 3.12. 'drum' или 'dr' - вывод зоны магнитного барабана эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
drum 1x  

[ Dump DRUM Setun-1958: ]

Zone  [0+-0], 1X
      index  (6) 

drum[  6:  0 ]  [--00+0-00], ZX1Z0
drum[  6:  1 ]  [-0+-0-0+0], Z1XX3
drum[  6:  2 ]  [-0+---0-0], Z1WXX
drum[  6:  3 ]  [0000+0000], 00100
drum[  6:  4 ]  [-++++0-00], Z44Z0
drum[  6:  5 ]  [--0-+0+00], ZXY10
drum[  6:  6 ]  [00000-0++], 000X4
drum[  6:  7 ]  [-+--000+0], Z2X03
drum[  6:  8 ]  [--+0+0000], ZY100
...
```

## 3.13. 'help' или 'h' - вывод списка команд виртуального пульта управления эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
help

Commands control for setun1958emu:
 [dump]       [arglist]
 ...
```

## 3.14. 'quit' или 'q' - прервать и завершить работу эмулятора SETUN-1958

```shell
$ ./setun1958emu

setun1958emu:
quit
 K  : [---+00++0], ZW313, (-9222)
  F  : [0+0-0], 03X, (24)
  C  : [---+0], ZW3, (-114)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [0+-++-+-+--+--+-+-], 1Y222YW22, (34673123)
  MB : [+000], 30, (27)


[ Stop Setun-1958 ]
```

# 4. Каталоги виртуальных устройств ввода и вывода эмулятора SETUN-1958

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

Приглашаем всех желающих внести свой посильный вклад в эмулятор SETUN-1958.

