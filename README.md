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
[ Start Setun-1958 ]

A*: [+0-++], 1Z4, (76), {10-111}
           k6..8[0+-] : A*=>(C) при w=0
A*: [+-+-0], 1YX, (60), {1-11-10}
           k6..8[0--] : (F)+(A*)=>(F)
A*: [++0-+], 13Y, (106), {110-11}
           k6..8[0+-] : A*=>(C) при w=+1
A*: [+0-00], 1Z0, (72), {10-100}
           k6..8[-00] : Ввод в Фа* - Вывод из Фа*
   k2..5[0-00] : Пишущая машинка ПМ (Телетайп ТП) печать одним цветом в виде символов
[
TECT1]

[DUMP registers Setun-1958]
  K  : [0+0+-+0-0], 0323X, (2373), {0101-110-10}
  F  : [+0+++], 114, (94), {10111}
  C  : [0-++0], 0Y3, (-15), {0-1110}
  W  : [-], Z, (-1), {-1}
  ph1: [0], 0, (0), {0}
  ph2: [0], 0, (0), {0}
  S  : [00-+00+00000-00000], 0Y0300X00, (-9389034), {00-1100100000-100000}
  R  : [000000000000000000], 000000000, (0), {000000000000000000}
  MB : [0000], 00, (0), {0000}

--- END emulator Setun-1958 ---

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

- Current version: 1.48
- Create date: 01.11.2018
