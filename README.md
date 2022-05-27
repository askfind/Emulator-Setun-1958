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

  K  : [000000000], 00000, (0)
  F  : [00000], 000, (0)
  C  : [00000], 000, (0)
  W  : [0], 0, (0)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [0000], 00, (0)

 --- Load 'ur0/03-input-checksum.txs' ---
00002 -> [00002] addr: [0---0], 0WX, (-39)
ZY24X -> [ZY24X] addr: [0---+], 0WY, (-38)

...

[ Start Setun-1958 ]

 С: [0000+], 001, (1)           -> fram[0] : 02WX3
A*: [0+---], 02W, (14)          -> fram[0] : 3020123X4
 k6..8[-0+]: (Фа*)=>(Мд*)

  K  : [0+----0+0], 02WX3, (1110)
  F  : [00000], 000, (0)
  C  : [000+0], 003, (3)
  W  : [0], 0, (0)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [000000000000000000], 000000000, (0)
  R  : [000000000000000000], 000000000, (0)
  MB : [+---], 2W, (14)

```

## Print  list commands 
```shell
./emu --help 
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

 С: [0-0+0], 0X3, (-24)         -> zone[0] : 01233
A*: [00++-], 012, (11)          -> zone[0] : 431302000
 k6..8[+0+]: (S)+(A*)=>(S)

  K  : [00++-+0+0], 01233, (921)
  F  : [00-0+], 0Z1, (-8)
  C  : [0-0++], 0X4, (-23)
  W  : [+], 1, (1)
  ph1: [0], 0, (0)
  ph2: [0], 0, (0)
  S  : [+++00++00+-++00000], 43131Y300, (187249725)
  R  : [+--+00000000000000], 2Y0000000, (76527504)
  MB : [0+--], 1W, (5)


--- END emulator Setun-1958 --- 
```

## Execute STEP=10  program 'Setun-1958' with debugging information
```shell

./emu --step 10
```

## Run Test #5
```shell

./emu --test 5

 --- TEST #5  Load program FT1,FT2 for VM SETUN-1958 ---

fopen: ur1/00_ip5.lst

...

 --- Load ur1/07_ip5_drum_11_setun.txs ---

 i=0

KC:
: [000000000], 00000, (0)
: [000000000], 00000, (0)

-KC = 0-KC:
: [000000000], 00000, (0)
: [000000000], 00000, (0)


Чтение файла закончено
fclose: ur1/00_ip5.lst

 --- END TEST #5 ---
```

## Run Test #6
```shell

./emu --test 6

--- TEST #6  Load program FT1,FT2 for VM SETUN-1958 ---

fopen: software/ip5_in_out_10_3/00_ip5_in_out_10_3.lst

 --- Load software/ip5_in_out_10_3/01_ip5_in_out_10_3_fram_0_setun.txs
 ---
00010
ZW3W1
00001

...

KC:
: [0000000+0], 00003, (3)
: [+++++0+0+], 14411, (9811)

-KC = 0-KC:
: [0000000-0], 0000X, (-3)
: [-----0-0-], ZWWZZ, (-9811)


Чтение файла закончено
fclose: software/ip5_in_out_10_3/00_ip5_in_out.lst

 --- END TEST #6 ---
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
- Edit date:   27.05.2022

- Current version: 1.62
