format ELF64 executable 3

main:
	mov eax, 1
	mov edi, eax
	lea rsi, [msg]
	mov edx, 13
	syscall

	mov eax, 60
	syscall

msg db 'hello world!', 10
