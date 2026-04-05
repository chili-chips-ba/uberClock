#include <stdio.h>

extern "C" void hello_cpp_run(void);
void hello_cpp_run(void)
{
    printf("C++: Hello, world!\n");
}
