
#include <syscalls.h>
 
using namespace myos;
using namespace myos::common;
using namespace myos::hardwarecommunication;
 
SyscallHandler::SyscallHandler(InterruptManager* interruptManager, uint8_t InterruptNumber)
:    InterruptHandler(interruptManager, InterruptNumber  + interruptManager->HardwareInterruptOffset())
{
}

SyscallHandler::~SyscallHandler()
{
}

void printf(char*);
void printInt(int);

uint32_t SyscallHandler::HandleInterrupt(uint32_t esp)
{
    CPUState* cpu = (CPUState*)esp;
    
    switch(cpu->eax)
    {
        // exit syscall
        case 1:
            InterruptHandler::exitCurrentTask();
            break;
        // fork syscall
        case 2:
            // printf("CPU State eip: ");
            // printInt(cpu->eip);
            // printf(", ");
            cpu->ecx = InterruptHandler::fork(cpu);
            // InterruptHandler::HandleInterrupt(esp);
            break;
        case 4:
            printf((char*)cpu->ebx);
            break;
        // waitPid syscall
        case 7:
            cpu->ecx = InterruptHandler::waitPid(cpu->ebx);
            break;
        // getpid syscall
        case 20:
            cpu->ecx = InterruptHandler::getPid();
            break;
        // call scheduler
        case 21:
            return InterruptHandler::Schedule(cpu);
            break;
        default:
            printf("Unknown syscall\n");
            return esp;
            break;
    }
    
    return esp;
}

