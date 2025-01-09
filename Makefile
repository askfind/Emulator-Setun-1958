.PHONY : run
emu : emusetun.c
#	gcc -Wall -Wextra -Wshadow -Wlogical-op  -Wshift-overflow=2 -std=c++11 -o emu -g emusetun.c
	gcc -o setun1958emu -g emusetun.c term.c
clean :
	rm -f setun1958emu
	rm -f emusetun
	rm -f output.vcd

run : emu
	./setun1958emu
