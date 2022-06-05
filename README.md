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
--- START emulator Setun-1958 ---
--- Reset Setun-1958 ---
--- Load 'test1_z01.txs' ---
 i=162

KC:
: [0000000-0], 0000X, (-3)
: [+----00++], 1WW04, (3325)

-KC = 0-KC:
: [0000000+0], 00003, (3)
: [-++++00--], Z440W, (-3325)

[ Start Setun-1958 ]
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

-- START emulator Setun-1958 ---

 --- Reset Setun-1958 ---

  K  : [000000000], 00000, (0)
  F  : [00000], 000, (0)
  C  : [00000], 000, (0)
  W  : [0], 0, (0)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [0000], 00, (0)

 --- Load 'test1_z01.txs' ---
1xxz0 -> [1XXZ0] addr: [----0], ZWX, (-120)
z4x0x -> [Z4X0X] addr: [----+], ZWY, (-119)
1wwxy -> [1WWXY] addr: [---00], ZW0, (-117)
z1xz0 -> [Z1XZ0] addr: [---0+], ZW1, (-116)
z4z30 -> [Z4Z30] addr: [---+0], ZW3, (-114)
...
z10zx -> [Z10ZX] addr: [++++0], 143, (120)
1411x -> [1411X] addr: [+++++], 144, (121)

 i=162

KC:
: [0000000-0], 0000X, (-3)
: [+----00++], 1WW04, (3325)

-KC = 0-KC:
: [0000000+0], 00003, (3)
: [-++++00--], Z440W, (-3325)

[ Dump FRAM Setun-1958: ]
Zone =-1

WW WX  1 XX Z0        02 03  0 WY 4W
   WY  Z 4X 0X           04  0 YZ X4
WZ W0  1 WW XY        1W 1X  1 00 00
   W1  Z 1X Z0           1Y  0 Y2 00
W2 W3  Z 4Z 30        1Z 10  0 01 00
...
0W 0X  1 3Y 13        42 43  0 03 00
   0Y  1 Z0 X0           44  0 03 00
0Z 00  1 W3 00        KC     0 00 0Y
   01  0 Y0 X1               1 ZW 22

...

[ Start Setun-1958 ]

 С: [0000+], 001, (1)           -> zone[0] : 0WX00
A*: [0---0], 0WX, (-39)         -> zone[0] : 01YZ0
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

  С: [0---0], 0WX, (-39)         -> zone[0] : 01YZ0
A*: [00+-+], 01Y, (7)           -> zone[0] : 00300
 k6..8[0-0]: (A*)=>(F)

  K  : [00+-+0-00], 01YZ0, (558)
  F  : [000+0], 003, (3)
  C  : [0---+], 0WY, (-38)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [0000], 00, (0)
...
```

## Execute STEP=3  program 'Setun-1958' with debugging information

```shell
$ ./emu --step 3

 --- START emulator Setun-1958 ---

 --- Reset Setun-1958 ---

 --- Load 'test1_z01.txs' ---

 i=162

KC:
: [0000000-0], 0000X, (-3)
: [+----00++], 1WW04, (3325)

-KC = 0-KC:
: [0000000+0], 00003, (3)
: [-++++00--], Z440W, (-3325)


[ Start Setun-1958 ]

STEP=3 <STOP>

  K  : [00+0+00-0], 0110X, (807)
  F  : [000+0], 003, (3)
  C  : [0--00], 0W0, (-36)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [0000], 00, (0)

```

## Notes

* `lpt0`, `ptp0` ... `ur0`, `ur1` folders - virtual device files like tty and others
* `Documentation` folder contains collection of documentation and program (`Programming` folder) examples

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
- Edit date:   05.06.2022

- Current version: 1.68
