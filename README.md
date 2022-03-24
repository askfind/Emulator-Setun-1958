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

reg C = 00001
A*=[-+++-], ( -43),    k6..8[-+-] : Норм.(S)=>(A*); (N)=>(S)
A*=[-0+-+], ( -74),    k6..8[+0-] : (S)-(A*)=>(S)
A*=[+0+-+], (  88),    k6..8[0+-] : A*=>(C) при w=0
A*=[0-++-], ( -16),    k6..8[000] : A*=>(C)
A*=[-+0--], ( -58),    k6..8[+0-] : (S)-(A*)=>(S)
A*=[-+0+-], ( -52),    k6..8[+0-] : (S)-(A*)=>(S)
A*=[-+0--], ( -58),    k6..8[+-+] : (A*)=>(R)
A*=[-+-+-], ( -61),    k6..8[+++] : (S)+(A*)(R)=>(S)
A*=[-+-+-], ( -61),    k6..8[++-] : (A*)+(S)(R)=>(S)
A*=[-+-+-], ( -61),    k6..8[+0-] : (S)-(A*)=>(S)
A*=[+0-++], (  76),    k6..8[0+-] : A*=>(C) при w=0
A*=[+-+-0], (  60),    k6..8[0--] : (F)+(A*)=>(F)
A*=[++0-+], ( 106),    k6..8[0+-] : A*=>(C) при w=+1
A*=[+0-00], (  72),    k6..8[-00] : STOP

[ Registers Setun-1958: ]
 K: [10-100-1000], (5805), 1Z0X0
 F: [-1110-11000], (-3699), Z4Z30
 C: [100-11], (79), 10Y
 W: [-1], (-1), Z
 ph1: [0], (0), 0
 ph2: [0], (0), 0
 S: [000000000000000000], (0), 0000000000
 R: [000000000000000000], (0), 0000000000
 MB: [0000], (0), 000

[ Stop Setun-1958 ]
...
```

```shell
./emu --help 
```

```shell
./emu --test 1
 
 1.4. Логическая операция SUM FULL
 C, P1 = A + B + P0 
 a,b,p0,c,p1 {-1,-1,-1}-> 0,-1
 ...
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

- Current version: 1.33
- Create date: 01.11.2018
