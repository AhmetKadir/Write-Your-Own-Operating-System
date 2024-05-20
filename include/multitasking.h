 
#ifndef __MYOS__MULTITASKING_H
#define __MYOS__MULTITASKING_H

#include <common/types.h>
#include <gdt.h>

namespace myos
{
    struct CPUState
    {
        common::uint32_t eax;
        common::uint32_t ebx;
        common::uint32_t ecx;
        common::uint32_t edx;

        common::uint32_t esi;
        common::uint32_t edi;
        common::uint32_t ebp;

        /*
        common::uint32_t gs;
        common::uint32_t fs;
        common::uint32_t es;
        common::uint32_t ds;
        */
        common::uint32_t error;

        // eip : instruction pointer
        common::uint32_t eip;
        common::uint32_t cs;
        common::uint32_t eflags;
        // esp : stack pointer
        common::uint32_t esp;
        common::uint32_t ss;        
    } __attribute__((packed));
    
    
    class Task
    {
    friend class TaskManager;
    private:
        common::uint8_t stack[4096]; // 4 KiB
        CPUState* cpustate;
        // process id
        int pid;
        // parent process id
        int ppid;
        task::ProcessState state;


    public:
        Task(GlobalDescriptorTable *gdt, void entrypoint());
        Task();
        ~Task();
        common::uint32_t getPid() { return pid; }
    };
    
    
    class TaskManager
    {
    private:
        Task tasks[256];
        int numTasks;
        int currentTask;
        GlobalDescriptorTable *gdt;
    public:
        // TaskManager();
        TaskManager(GlobalDescriptorTable *_gdt);
        ~TaskManager();

        bool AddTask(Task* task);
        CPUState* Schedule(CPUState* cpustate);
        void printProcessTable();
        int exitCurrentTask();
        int fork(CPUState* cpustate);
        int waitPid(int pid);
        void sleep();
        void runChilds();
        int getCurrentTaskPid();
    };
}


#endif