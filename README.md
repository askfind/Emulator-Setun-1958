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
c=a-b: [00000000-1-1-1-1-1-1-1-1-1-1], (-29524), 0000ZWWWW0

t18: test Oper=k6..8[+00] : (A*)=>(S)
addr=: [00000], (0), X00
ram[...] (  0: 0) =  hex = 0x021210 [10-1010-100], (5904), 1Z1Z0
ram[...] (  0: 1) =  hex = 0x000080 [000001000], (27), 00030

reg C = 00001
K=: [000001000], (27), 00030
A*=[00000], (   0),    k6..8[+00] : (A*)=>(S)
ret_exec = 0

[ Registers Setun-1958: ]
 K: [000001000], (27), 00030
 F: [00000], (0), 000
 C: [00010], (3), 003
 W: [1], (1), Y
 S: [10-1010-100000000000], (116208432), 1Z1Z000000
 R: [000000000000000000], (0), 0000000000
 MB: [0000], (0), 000
...
```

## Notes

* `lpt0`, `ptp0` ... `ur0`, `ur1` folders - virtual device files like tty and others
* `Documentation` folder contains collection of documentation and program (`Programming` folder) examples

# Links

- <https://en.wikipedia.org/wiki/Ternary_computer>
- <https://en.wikipedia.org/wiki/Setun>
- <https://habr.com/en/post/46688/>

# Contributing

Everybody is invited and welcome to contribute to Setun VM.

# History

- Current version: 1.24
- Create date: 01.11.2018
