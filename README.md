SETUN-1958 VM
=============

A virtual machine of ternary computer Setun, also known as "Small Automatic Digital Machine" (in Russian: МЦВМ "
Сетунь") written on C

# Using

Build and run

```shell
make
./emu
```

As a result you will see dump of register and memory structure of VM after run some tests:

```shell
...
$ ./emu

 --- START emulator Setun-1958 ---

 --- Reset Setun-1958 ---

 --- Load 'ur0/01-test.txs' ---
01YZ00110X101X011WX411WXY003Z002X30012Y3ZWX3101XY001233012Y301YZX0X01X0Y413004Z00X000011Z002Z3Y0Z3100002X0W000010ZX0WY1011ZXXZ1WXX1W3000WX00Z0000Z01000Z0000030000X00000000000Y1ZW2200000000000000Y1ZW220000Y1YYYX00000000000000000000000000000000000000000000000000000000000013XX4113X41202W00000ZWYZ31XY100W02XZ1XZ0Z0430Z0133Z033X1YY1000X2XZ2230Z2W40Z3Z3XZ323XZ3W23Z2243Z224XZ223X1Z410Z432X1ZX30Z0420Z1YY0Z4003Z42YXZ1Y3X11Y100102XZ1X30Z403X114100032XZ4230Z10Y0Z1X33123100042X1YXZX1X01311XZ0Z4Y0XZ223ZZ4ZY3Z1XZ01WWY21YXZX134131XXZ01WWX4Z10ZX1411X1XXZ0Z4X0X1WWXYZ1XZ0Z4Z301WW3WZY410Z42Y3Z4223Z4X30Z1133ZY3Y3Z4Z30Z1X20ZY333ZY3Y3000001YXZXZW313Z4XZ0Z10ZXZWY1XZ4YZ01YXZX13Y131Z0X01W3000Y0X10WY4W0YZX4100000Y200001001WW2X00000000001444414444ZWWWWZWWWW03XW31X34X024YYZYW2213Y2XZX2Y402100133Y10W00001X0014414144440030000300 --- EOF 'test-1.txs' ---


[ Start Setun-1958 ]

reg C = 00001

TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1
TECT1

```

```shell
./emu --help 
```

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

## Notes

* `lpt0`, `ptp0` ... `ur0`, `ur1` folders - virtual device files like tty and others
* `Documentation` folder contains collection of documentation and program (`Programming` folder) examples

# Links

*Materials on ternary computer science* - <http://ternarycomp.cs.msu.su/>
*Emulator of the first ternary Soviet "Setun"* - <http://trinary.su/projects/>
*Unique Setun based on the ternary code* - <https://habr.com/ru/company/ua-hosting/blog/273929/>

# Contributing

Everybody is invited and welcome to contribute to Setun VM.

# History

- Current version: 1.52
- Create date: 01.11.2018
