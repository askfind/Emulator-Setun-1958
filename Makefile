.PHONY : run
emu : emusetun.c
#	gcc -Wall -Wextra -Wshadow -Wlogical-op  -Wshift-overflow=2 -std=c++11 -o emu -g emusetun.c
	gcc -std=c++11 -o emu -g emusetun.c
clean :
	rm -f emu
	rm -f output.vcd

run : emu
	./emusetun
