void _start() {
  char *msg = "hello world";
  asm("mov $1, %%rax\n"
      "mov $1, %%rdi\n"
      "mov $11,%%rdx\n"
      "mov %0, %%rsi\n"
      "syscall;"
      :
      : "r"(msg)
      : "rax", "rdi", "rsi", "rdx");

  asm("mov $60,   %%rax\n"
      "xor %%rdi, %%rdi\n"
      "syscall"
      :
      :
      : "rax", "rdi");
}
