SETUN-1958 VM
=============

A virtual machine of ternary computer Setun, also known as "Small Automatic Digital Machine" (in Russian: МЦВМ "
Сетунь") written on C

# Using

Build and run

```shell
$ make
gcc -o setun1958emu -g emusetun.c
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
- Edit date:   11.03.2022

- Current version: 1.82
