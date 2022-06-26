SETUN-1958 VM
=============

A virtual machine of ternary computer Setun, also known as "Small Automatic Digital Machine" (in Russian: МЦВМ "
Сетунь") written on C

# Using

Build and run

```shell
$ make
gcc -o emu -g emusetun.c
```

As a result you will see dump of register and memory structure of VM after run some tests:

```shell

$ ./emu

[ Start Setun-1958 ]
[ Read commands from FT1 ]

i=54
KC:
: [00000000+], 00001, (1)
: [-0-+-0+--], ZZ21W, (-7123)
-KC = 0-KC:
: [00000000-], 0000Z, (-1)
: [+0+-+0-++], 11YZ4, (7123)

[ Read commands from FT1 ]

i=54
KC:
: [0000000-+], 0000Y, (-2)
: [+-+-+-+-0], 1YYYX, (4920)
-KC = 0-KC:
: [0000000+-], 00002, (2)
: [-+-+-+-+0], Z2223, (-4920)

[ Read commands from FT1 ]

i=54
KC:
: [0000000-+], 0000Y, (-2)
: [+0---+-+-], 1ZW22, (5528)
-KC = 0-KC:
: [0000000+-], 00002, (2)
: [-0+++-+-+], Z14YY, (-5528)


<STOP>

  K  : [-+++0+--0], Z432X, (-3387)
  F  : [+0000], 100, (81)
  C  : [+0-+0], 1Z3, (75)
  W  : [-], Z, (-1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [00-0-00+0+000-0--+], 0XX110ZZY, (-15877712)
  R  : [000+00000000000000], 010000000, (4782969)
  MB : [0+--], 1W, (5)


[ Stop Setun-1958 ]

```

## Print  list commands 
```shell
./emu --help

usage: ./emu [options] FILE SCRIPT(s)...
	--test : number test VM Setun-1958)
	--debug : view step VM Setun-1958)
	--load : load software VM Setun-1958)
```

## Run Test #1
```shell
./emu --test 1

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

## Execute a program 'Setun-1958' with debugging information
```shell

./emu --debug
 ./emu --debug --step 100

  K  : [000000000], 00000, (0)
  F  : [00000], 000, (0)
  C  : [00000], 000, (0)
  W  : [0], 0, (0)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [0000], 00, (0)

[ Start Setun-1958 ]
[ Read commands from FT1 ]
01YZ0 -> [01YZ0] addr: [0---0], 0WX, (-39)
0110X -> [0110X] addr: [0---+], 0WY, (-38)
101X0 -> [101X0] addr: [0--00], 0W0, (-36)
11WX4 -> [11WX4] addr: [0--0+], 0W1, (-35)
11WXY -> [11WXY] addr: [0--+0], 0W3, (-33)
003Z0 -> [003Z0] addr: [0--++], 0W4, (-32)
02X30 -> [02X30] addr: [0-0-0], 0XX, (-30)
012Y3 -> [012Y3] addr: [0-0-+], 0XY, (-29)
ZWX31 -> [ZWX31] addr: [0-000], 0X0, (-27)
01XY0 -> [01XY0] addr: [0-00+], 0X1, (-26)
01233 -> [01233] addr: [0-0+0], 0X3, (-24)
012Y3 -> [012Y3] addr: [0-0++], 0X4, (-23)
01YZX -> [01YZX] addr: [0-+-0], 0YX, (-21)
0X01X -> [0X01X] addr: [0-+-+], 0YY, (-20)
0Y413 -> [0Y413] addr: [0-+00], 0Y0, (-18)
004Z0 -> [004Z0] addr: [0-+0+], 0Y1, (-17)
0X000 -> [0X000] addr: [0-++0], 0Y3, (-15)
011Z0 -> [011Z0] addr: [0-+++], 0Y4, (-14)
02Z3Y -> [02Z3Y] addr: [00--0], 0ZX, (-12)
0Z310 -> [0Z310] addr: [00--+], 0ZY, (-11)
0002X -> [0002X] addr: [00-00], 0Z0, (-9)
0W000 -> [0W000] addr: [00-0+], 0Z1, (-8)
010ZX -> [010ZX] addr: [00-+0], 0Z3, (-6)
0WY10 -> [0WY10] addr: [00-++], 0Z4, (-5)
11ZXX -> [11ZXX] addr: [000-0], 00X, (-3)
Z1WXX -> [Z1WXX] addr: [000-+], 00Y, (-2)
1W300 -> [1W300] addr: [00000], 000, (0)
0WX00 -> [0WX00] addr: [0000+], 001, (1)
Z0000 -> [Z0000] addr: [000+0], 003, (3)
Z0100 -> [Z0100] addr: [000++], 004, (4)
0Z000 -> [0Z000] addr: [00+-0], 01X, (6)
00300 -> [00300] addr: [00+-+], 01Y, (7)
00X00 -> [00X00] addr: [00+00], 010, (9)
00000 -> [00000] addr: [00+0+], 011, (10)
0000Y -> [0000Y] addr: [00++0], 013, (12)
1ZW22 -> [1ZW22] addr: [00+++], 014, (13)
00000 -> [00000] addr: [0+--0], 02X, (15)
00000 -> [00000] addr: [0+--+], 02Y, (16)
0000Y -> [0000Y] addr: [0+-00], 020, (18)
1ZW22 -> [1ZW22] addr: [0+-0+], 021, (19)
0000Y -> [0000Y] addr: [0+-+0], 023, (21)
1YYYX -> [1YYYX] addr: [0+-++], 024, (22)
00000 -> [00000] addr: [0+0-0], 03X, (24)
00000 -> [00000] addr: [0+0-+], 03Y, (25)
00000 -> [00000] addr: [0+000], 030, (27)
00000 -> [00000] addr: [0+00+], 031, (28)
00000 -> [00000] addr: [0+0+0], 033, (30)
00000 -> [00000] addr: [0+0++], 034, (31)
00000 -> [00000] addr: [0++-0], 04X, (33)
00000 -> [00000] addr: [0++-+], 04Y, (34)
00000 -> [00000] addr: [0++00], 040, (36)
00000 -> [00000] addr: [0++0+], 041, (37)
00000 -> [00000] addr: [0+++0], 043, (39)
00000 -> [00000] addr: [0++++], 044, (40)

i=54
KC:
: [00000000+], 00001, (1)
: [-0-+-0+--], ZZ21W, (-7123)
-KC = 0-KC:
: [00000000-], 0000Z, (-1)
: [+0+-+0-++], 11YZ4, (7123)


 С: [0000+], 001, (1)		-> zone[0] : 0WX00
A*: [0---0], 0WX, (-39)		-> zone[0] : 01YZ0
 k6..8[000]: A*=>(C)

  K  : [0---00000], 0WX00, (-3159)
  F  : [00000], 000, (0)
  C  : [0---0], 0WX, (-39)
  W  : [0], 0, (0)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [0000], 00, (0)

...

[ Dump FRAM Setun-1958: ]
Zone = 0

WW WX  0 1Y Z0        02 03  Z 00 00
   WY  0 11 0X           04  Z 01 00
WZ W0  1 01 X0        1W 1X  0 Z0 00
   W1  1 1W X4           1Y  0 03 00
W2 W3  1 1W XY        1Z 10  0 0X 00
   W4  0 03 Z0           11  0 03 00
XW XX  0 2X 30        12 13  0 00 0Y
   XY  0 12 Y3           14  Z Y0 1X
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
0Z 00  1 W3 00        KC     0 00 00
   01  0 WX 00               0 Z0 Z0

```

## Execute STEP=3  program 'Setun-1958' with debugging information

```shell
$ ./emu --step 2

[ Start Setun-1958 ]
[ Read commands from FT1 ]

i=54
KC:
: [00000000+], 00001, (1)
: [-0-+-0+--], ZZ21W, (-7123)
-KC = 0-KC:
: [00000000-], 0000Z, (-1)
: [+0+-+0-++], 11YZ4, (7123)


  K  : [00+-+0-00], 01YZ0, (558)
  F  : [000+0], 003, (3)
  C  : [0---+], 0WY, (-38)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [0000], 00, (0)

[ Step = 2 : Break work Setun-1958 ]

```

## Notes

* `ptr1`,`ptr2`,`ptp1`,`tty1` - virtual device files like tty and others
* `Documentation` folder contains collection of documentation and program (`Programming` folder) examples

Device 	Direction 	Description

PTR1: 	source paper tape reader #1
PTR2: 	source paper tape reader #2
PTP1: 	destination paper tape punch #1
TTY1: 	destination hard copy terminal #1

# Links

 1. *Materials on ternary computer science* - <http://ternarycomp.cs.msu.su/>
 2. *Emulator of the first ternary Soviet "Setun"* - <http://trinary.su/projects/>
 3. *Unique Setun based on the ternary code* - <https://habr.com/ru/company/ua-hosting/blog/273929/>
 4. *Group users in social network VK* - <https://vk.com/setunsu/>
 5. *Group users in social network Telegram* - <https://t.me/setun_1958/>

# Contributing

Everybody is invited and welcome to contribute to Setun VM.

# History


- Create date: 01.11.2018
- Edit date:   26.06.2022

- Current version: 1.76
