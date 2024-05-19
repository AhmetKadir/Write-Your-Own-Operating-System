
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;

void printf(char *str)
{
    static uint16_t *VideoMemory = (uint16_t *)0xb8000;

    static uint8_t x = 0, y = 0;

    for (int i = 0; str[i] != '\0'; ++i)
    {
        switch (str[i])
        {
        case '\n':
            x = 0;
            y++;
            break;
        default:
            VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | str[i];
            x++;
            break;
        }

        if (x >= 80)
        {
            x = 0;
            y++;
        }

        if (y >= 25)
        {
            for (y = 0; y < 25; y++)
                for (x = 0; x < 80; x++)
                    VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void printInt(int n)
{
    if (n == 0)
    {
        printf("0");
        return;
    }

    char buffer[32];
    char *str = buffer + 31;
    *str = '\0';

    bool isNegative = false;

    if (n < 0)
    {
        isNegative = true;
        n = -n;
    }

    while (n != 0)
    {
        str--;
        *str = (n % 10) + '0';
        n /= 10;
    }

    if (isNegative)
    {
        str--;
        *str = '-';
    }

    printf(str);
}

void pause()
{
    for (int i = 0; i < 100000000; i++)
        ;
}

void printfHex(uint8_t key)
{
    char *foo = "00";
    char *hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex(key & 0xFF);
}
class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c)
    {
        char *foo = " ";
        foo[0] = c;
        printf(foo);
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;

public:
    MouseToConsole()
    {
        uint16_t *VideoMemory = (uint16_t *)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }

    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t *VideoMemory = (uint16_t *)0xb8000;
        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);

        x += xoffset;
        if (x >= 80)
            x = 79;
        if (x < 0)
            x = 0;
        y += yoffset;
        if (y >= 25)
            y = 24;
        if (y < 0)
            y = 0;

        VideoMemory[80 * y + x] = (VideoMemory[80 * y + x] & 0x0F00) << 4 | (VideoMemory[80 * y + x] & 0xF000) >> 4 | (VideoMemory[80 * y + x] & 0x00FF);
    }
};

void sysprintf(char *str)
{
    asm("int $0x80" : : "a"(4), "b"(str));
}

void sysExit()
{
    asm("int $0x80" : : "a"(1));
}

void schedule()
{
    asm("int $0x80" : : "a"(21));
}

// int sysFork(int *pid)
// {
//     asm("int $0x80" :"=c" (*pid): "a" (2));
//     return *pid;
// }

int sysGetPid()
{
    int pid;
    asm("int $0x80" : "=c"(pid) : "a"(20));
    return pid;
}

int sysFork()
{
    // printf("sysFork is called\n");
    int ret;
    asm("int $0x80" : "=c"(ret) : "a"(2));
    // printf("sysFork returned: "); printInt(ret); printf("\n");
    return ret;
}

int sysWaitPid(int pid)
{
    int ret;
    asm("int $0x80" : "=c"(ret) : "a"(3), "b"(pid));
    return ret;
}

void taskA()
{
    sysprintf("Task A is running\n");
    pause();
    // int i = 0;
    // while (true){
    //     if (i % 100000000 == 0) {
    //         printf("Task A is running\n");
    //         printInt(i);
    //         printf("\n");
    //         i = 0;
    //     }
    //     i++;
    //     // if (i++ > 1000000000)
    //     //     sysExit();
    // }
}

void taskB()
{
    sysprintf("Task B is running\n");
    int i = 0;
    while (true)
    {
        if (i % 100000000 == 0)
        {
            printf("Task B is running\n");
            printInt(i);
            printf("\n");
        }
        if (i++ > 1000000000)
            sysExit();
    }

    while (1)
        ;
}

void taskC()
{
    sysprintf("Task C is running\n");
    int i = 0;
    while (true)
    {
        if (i % 100000000 == 0)
        {
            printf("Task C is running\n");
            printInt(i);
            printf("\n");
        }

        if (i++ > 1000000000)
            sysExit();
    }
    while (1)
        ;
}

void long_running_program(int n)
{
    int result;
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            result = i * j;
        }
    }
    printf("Result: ");
    printInt(result);
    printf("\n");
    pause();
}

void collatz(int n)
{
    printInt(n);
    while (n != 1)
    {
        if (n % 2 == 0)
        {
            n = n / 2;
        }
        else
        {
            n = 3 * n + 1;
        }
        printf(", ");
        printInt(n);
    }
    printf("\n");
    pause();
}

void runCollatz(int n)
{
    printf("Child is going to run collatz: ");
    collatz(n);
    sysExit();
    // pause();
    // while(1){
    //     printf("Child is running\n");
    // }
}

void runLongRunningProgram(int n)
{
    printf("Child is going to run long running program\n");
    long_running_program(n);
    sysExit();
    // while(1){
    //     printf("Child2 is running\n");
    // }
}

void initTask2()
{
    int pid = sysFork();

    // printf("Fork returned: ");
    // printInt(pid);
    // printf("\n");
    if (pid == 0)
    {
        for (int i = 0; i < 3; i++)
        {
            printf("Child is going to run collatz\n");
        }
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    pid = sysFork();
    if (pid == 0)
    {
        printf("Child is going to run collatz\n");
        collatz(105);
        sysExit();
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    pid = sysFork();
    if (pid == 0)
    {
        printf("Child is going to run collatz\n");
        pause();
        collatz(51);
        sysExit();
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    while (1)
    {
        // printf(" pid = ");
        // printInt(sysGetPid());
        // printf(" ... ");
        // printf("Special Task is running\n");
        // pause();
    }
}

void initTask1()
{
    int pid = sysFork();

    // printf("Fork returned: ");
    // printInt(pid);
    // printf("\n");

    if (pid == 0)
    {
        runCollatz(23);
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    pid = sysFork();

    if (pid == 0)
    {
        runCollatz(15);
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    pid = sysFork();

    if (pid == 0)
    {
        runCollatz(10);
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    pid = sysFork();
    if (pid == 0)
    {
        runLongRunningProgram(10);
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    pid = sysFork();
    if (pid == 0)
    {
        runLongRunningProgram(1000);
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    pid = sysFork();
    if (pid == 0)
    {
        runLongRunningProgram(1000);
    }
    else if (pid < 0)
    {
        printf("Fork failed\n");
        sysExit();
    }

    while (1)
    {
        // printf(" pid = ");
        // printInt(sysGetPid());
        // printf(" ... ");
        // printf("Special Task is running\n");
        // pause();
    }
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for (constructor *i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}

extern "C" void kernelMain(const void *multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("Hello This is KadirOS!\n");

    GlobalDescriptorTable gdt;

    Task initTask(&gdt, initTask1);
    // Task task2(&gdt, initTask2);
    // Task task2(&gdt, taskA);
    // Task task3(&gdt, taskB);
    // Task task4(&gdt, taskC);

    TaskManager taskManager(&gdt);
    taskManager.AddTask(&initTask);
    // taskManager.AddTask(&task2);
    // taskManager.AddTask(&task3);
    // taskManager.AddTask(&task4);

    taskManager.printProcessTable();

    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);

    interrupts.Activate();

    while (1)
    {
#ifdef GRAPHICSMODE
        desktop.Draw(&vga);
#endif
    }
}
