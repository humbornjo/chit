chw:
	gcc -nostdlib -no-pie -c test.c -o test.o
	ld -N -m elf_x86_64 test.o -o chw
	sstrip chw

ahw:
	fasm test.asm ahw

.PHONY: clean
clean: 
	rm *.o *hw
