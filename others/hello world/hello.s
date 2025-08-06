.section .data
hello: .ascii "Hello World!\n"

.section .text
.globl _start

_start:
	movq $1, %rax     # call the 'write' system call
	movq $1, %rdi     # write to file 1 (stdout)
	movq $hello, %rsi # write data starting from the address
	                  # that 'hello' points to
	movq $13, %rdx    # write 13 bytes
	syscall           # actually make the call

	movq $60, %rax    # call the 'exit' system call
	movq $0, %rdi     # return 0 as the exit code
	syscall           # actually make the call


# to run 
# as hello.s -o hello.o
# ld hello.o -o hello
# ./hello

