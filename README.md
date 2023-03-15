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

## Print  list commands 
```shell
./setun1958emu --help

usage: ./setun1958emu [options] FILE SCRIPT(s)...
        --test : number test VM Setun-1958)
        --debug : view step VM Setun-1958)
        --breakpoint : view stop VM Setun-1958)
        --load : load software VM Setun-1958)
        --convert : convert software file.lst to paper.txt VM Setun-1958)
```

## Dump commands filename.txs

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

## Run Test #1
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

## Execute a program 'Setun-1958' with debugging information
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
- Edit date:   15.03.2022

- Current version: 1.84
