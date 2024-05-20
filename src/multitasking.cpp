
#include <multitasking.h>

using namespace myos;
using namespace myos::common;

void printf(char* str);
void printInt(int n);
void printfHex(uint8_t n);

int counter = 0;


Task::Task(GlobalDescriptorTable *gdt, void entrypoint())
{
    // stack grows downwards
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
       
    // eip is the instruction pointer, the address of the next instruction to be executed
    cpustate -> eip = (uint32_t)entrypoint;
    cpustate -> cs = gdt->CodeSegmentSelector();
    cpustate -> eflags = 0x202;

    ppid = -1;
    state = task::ProcessState::READY;
    
    // printf("Task created\n");
}

Task::Task()
{
    state=task::ProcessState::READY;
    ppid=0;
    cpustate = (CPUState*)(stack + 4096 - sizeof(CPUState));
    
    cpustate -> eax = 0;
    cpustate -> ebx = 0;
    cpustate -> ecx = 0;
    cpustate -> edx = 0;

    cpustate -> esi = 0;
    cpustate -> edi = 0;
    cpustate -> ebp = 0;
    
    cpustate -> eip = 0;
    cpustate -> cs = 0;

    cpustate -> eflags = 0x202;

    pid = -1;
}

Task::~Task()
{
}

TaskManager::TaskManager(GlobalDescriptorTable *_gdt)
{
    numTasks = 0;
    currentTask = -1;
    gdt = _gdt;
    for(int i = 0; i < 256; i++)
    {
        tasks[i].cpustate -> cs = gdt->CodeSegmentSelector();
    }
}

TaskManager::~TaskManager()
{
}

bool TaskManager::AddTask(Task* task){
    if (numTasks >= 256) {
        return false; // Maximum number of tasks reached
    }

    tasks[numTasks].pid = numTasks;
    tasks[numTasks].ppid = -1;
    tasks[numTasks].state = task::ProcessState::READY;
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    
    tasks[numTasks].cpustate -> eax = task->cpustate->eax;
    tasks[numTasks].cpustate -> ebx = task->cpustate->ebx;
    tasks[numTasks].cpustate -> ecx = task->cpustate->ecx;
    tasks[numTasks].cpustate -> edx = task->cpustate->edx;

    tasks[numTasks].cpustate -> esi = task->cpustate->esi;
    tasks[numTasks].cpustate -> edi = task->cpustate->edi;
    tasks[numTasks].cpustate -> ebp = task->cpustate->ebp;
    
    tasks[numTasks].cpustate -> eip = task->cpustate->eip;
    tasks[numTasks].cpustate -> cs = task->cpustate->cs;
    
    tasks[numTasks].cpustate -> eflags = task->cpustate->eflags;

    numTasks++;
    return true;
}

CPUState* TaskManager::Schedule(CPUState* cpustate)
{
    // round robin scheduling
    if(numTasks <= 0)
        return cpustate;
    
    if(currentTask >= 0) {
        tasks[currentTask].cpustate = cpustate;
        if (tasks[currentTask].state == task::ProcessState::RUNNING)
            tasks[currentTask].state = task::ProcessState::READY;
    }

    // traverse until we find a task that is ready
    bool foundReadyTask = false;
    for (int i = 0; i < numTasks; i++) {
        currentTask++;
        currentTask %= numTasks;
        if (tasks[currentTask].state == task::ProcessState::READY && currentTask != 0) {
            foundReadyTask = true;
            break;
        }
    }

    // no task is ready, run special task
    if (!foundReadyTask) {
        currentTask = 0; 
    }
    
    tasks[currentTask].state = task::ProcessState::RUNNING;

    if (counter > 1){
        printProcessTable();
        for (int i = 0; i < 100000000; i++);
        counter = 0;
    }
    else {
        counter++;
    }


    printf("Scheduling pid: "); printInt(tasks[currentTask].pid); 
    printf(", esp: "); printInt((uint32_t)tasks[currentTask].cpustate); 
    printf(", eip: "); printInt(tasks[currentTask].cpustate->eip); printf("\n");
    
    return tasks[currentTask].cpustate;
}

void TaskManager::printProcessTable(){
    printf("\n--------Process table--------\n");
    for (int i = 0; i < numTasks; i++) {
        printf(" pid: ");
        printInt(tasks[i].getPid());
        printf(", ppid: ");
        printInt(tasks[i].ppid);
        printf(", state: ");
        switch (tasks[i].state) {
            case task::ProcessState::READY:
                printf("READY");
                break;
            case task::ProcessState::RUNNING:
                printf("RUNNING");
                break;
            case task::ProcessState::BLOCKED:
                printf("BLOCKED");
                break;
            case task::ProcessState::FINISHED:
                printf("FINISHED");
                break;
            case task::ProcessState::ZOMBIE:
                printf("ZOMBIE");
                break;
            default:
                printf("UNKNOWN");
                break;
        }
        printf(" esp: ");
        printInt((uint32_t)tasks[i].cpustate);
        printf(" eip: ");
        printInt(tasks[i].cpustate->eip);
        printf(" Cpu state: ");
        printInt((uint32_t)tasks[i].cpustate);
        printf("\n");

    }
    printf("\n");
}

int TaskManager::exitCurrentTask(){
    // printf("Task finished: ");
    // printInt(tasks[currentTask].getPid());
    // printf("\n\n");
    tasks[currentTask].state = task::ProcessState::FINISHED;
    int parentPid = tasks[currentTask].ppid;
    // if parent process is blocked, unblock it
    if (parentPid != -1) {
        if (tasks[parentPid].state == task::ProcessState::BLOCKED) {
            tasks[parentPid].state = task::ProcessState::READY;
        }
    }
    return (uint32_t)Schedule(tasks[currentTask].cpustate);
}

// fork syscall
int TaskManager::fork(CPUState* cpu) {
    // printf("Fork called by pid: "); printInt(tasks[currentTask].pid); printf(", ");

    if (tasks[currentTask].pid != 0) {
        return 0;
    }

    if (numTasks >= 256) {
        return -1; // Maximum number of tasks reached
    }

    // printf("Inside fork: ");

    tasks[numTasks].pid = numTasks;
    tasks[numTasks].ppid = tasks[currentTask].pid;
    tasks[numTasks].state = task::ProcessState::READY;
    
    tasks[currentTask].cpustate->ecx = tasks[numTasks].pid;

    // // Print base address of stacks
    // printf("Parent stack: "); printInt((uint32_t)tasks[currentTask].stack); printf(", ");
    // printf("Child stack: "); printInt((uint32_t)tasks[numTasks].stack); printf("\n");

    // // Print initial CPU states
    // printf("Parent CPUState: "); printInt((int32_t)tasks[currentTask].cpustate); printf(", ");
    // printf("Initial Child CPUState: "); printInt((int32_t)tasks[numTasks].cpustate); printf(", ");
    // printf("CPU (cpu): "); printInt((uint32_t)cpu); printf(", ");
    // printf("Parent eip: "); printInt(tasks[currentTask].cpustate->eip); printf(", ");
    // printf("Initial child eip: "); printInt(tasks[numTasks].cpustate->eip); printf(", ");
    // printf("Cpu eip: "); printInt(cpu->eip); printf("\n");

        // Copy stack from parent to child
    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++) {
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];
    }

    //Calculate the offset for the CPU state
    uint32_t stackOffset = (uint32_t)cpu - (uint32_t)tasks[currentTask].stack;
    tasks[numTasks].cpustate = (CPUState*)((uint32_t)tasks[numTasks].stack + stackOffset);
    tasks[numTasks].cpustate->esp = (uint32_t)tasks[numTasks].stack + cpu->esp - (uint32_t)tasks[currentTask].stack;
    // tasks[currentTask].cpustate = (CPUState*)((uint32_t)tasks[currentTask].stack + stackOffset);

    // printf("Calculated stack offset: "); printInt(stackOffset); printf(", ");
    // printf("Updated Child CPUState: "); printInt((int32_t)tasks[numTasks].cpustate); printf(", ");
    // printf("Updated Child eip: "); printInt(tasks[numTasks].cpustate->eip); printf("\n");
    // tasks[numTasks].cpustate->esp = (common::uint32_t)tasks[numTasks].stack + (cpu->esp - (common::uint32_t)tasks[currentTask].stack);

    // Adjust the child's ESP to point to the correct location in the child's stack
    // tasks[numTasks].cpustate->esp = tasks[numTasks].stack + cpu->esp - tasks[currentTask].stack;

    // Print debug information
    // printf("cpu: "); printInt((uint32_t)cpu); printf(", ");
    // printf("Parent CPUState at: ");
    // printInt((common::uint32_t)tasks[currentTask].cpustate);
    // printf("\nChild CPUState at: ");
    // printInt((common::uint32_t)tasks[numTasks].cpustate);
    // printf("\n");

    tasks[numTasks].cpustate->ecx = 0;
    tasks[numTasks].state = task::ProcessState::BLOCKED;

    // printf("Parent CPUState:\n");
    // printf("EAX: "); printInt(cpustate->eax); printf(", ");
    // printf("ECX: "); printInt(cpustate->ecx); printf(", ");
    // // printf("cpustate: "); printInt((int32_t)tasks[currentTask].cpustate); printf(", ");
    // printf("EIP: "); printInt(tasks[currentTask].cpustate->eip); printf(", ");
    // printf("CS: "); printInt(cpustate->cs); printf("\n");
    // printf("Parent CPUState:\n");
    // printf("EAX: "); printInt(tasks[currentTask].cpustate->eax); printf(", ");
    // printf("ECX: "); printInt(tasks[currentTask].cpustate->ecx); printf(", ");
    // // printf("ESP: "); printInt(tasks[currentTask].cpustate->esp); printf(", ");
    // printf("EIP: "); printInt(tasks[currentTask].cpustate->eip); printf(", ");
    // printf("CS: "); printInt(tasks[currentTask].cpustate->cs); printf("\n");
    // printf("Child CPUState:\n");
    // printf("EAX: "); printInt(tasks[numTasks].cpustate->eax); printf(", ");
    // printf("ECX: "); printInt(tasks[numTasks].cpustate->ecx); printf(", ");
    // // printf("ESP: "); printInt(tasks[numTasks].cpustate->esp); printf(", ");
    // printf("EIP: "); printInt(tasks[numTasks].cpustate->eip); printf(", ");
    // printf("CS: "); printInt(tasks[numTasks].cpustate->cs); printf("\n");

    numTasks++;

    printProcessTable();
    for(int i = 0; i < 1000000000; i++);

    // printf("Forked successfully. ");
    return tasks[numTasks - 1].pid; // Return the child's PID
}


/*
int TaskManager::fork(CPUState* cpustate) {
    if (numTasks >= 256) {
        return -1; // Maximum number of tasks reached
    }

    printf("Inside fork\n");

    // Print where stacks are
    printf("Parent stack: "); printInt((int32_t)tasks[currentTask].stack); printf("\n");
    printf("Child stack: "); printInt((int32_t)tasks[numTasks].stack); printf("\n");

    CPUState* parentCPUState = cpustate;

    // Print parent CPU state for debugging
    printf("Parent CPUState:\n");
    printf("EAX: "); printInt(parentCPUState->eax); printf(", ");
    printf("ECX: "); printInt(parentCPUState->ecx); printf(", ");
    printf("ESP: "); printInt(parentCPUState->esp); printf(", ");
    printf("EIP: "); printInt(parentCPUState->eip); printf(", ");
    printf("CS: "); printInt(parentCPUState->cs); printf("\n");

    // Copy stack from parent to child
    for (int i = 0; i < sizeof(tasks[currentTask].stack); i++) {
        tasks[numTasks].stack[i] = tasks[currentTask].stack[i];
    }

    // Allocate and set up the child's CPU state
    tasks[numTasks].cpustate = (CPUState*)(tasks[numTasks].stack + 4096 - sizeof(CPUState));
    *tasks[numTasks].cpustate = *parentCPUState; // Copy CPU state

    // Calculate stack offset for ESP and adjust child's ESP
    uint32_t parentStackBase = (uint32_t)(tasks[currentTask].stack + 4096);
    uint32_t childStackBase = (uint32_t)(tasks[numTasks].stack + 4096);
    uint32_t stackOffset = parentStackBase - parentCPUState->esp;
    tasks[numTasks].cpustate->esp = childStackBase - stackOffset;

    // Set return value for the child process
    tasks[numTasks].cpustate->eax = 0; // Child process should return 0 from fork

    // Print child CPU state for debugging
    printf("Child CPUState:\n");
    printf("EAX: "); printInt(tasks[numTasks].cpustate->eax); printf(", ");
    printf("ECX: "); printInt(tasks[numTasks].cpustate->ecx); printf(", ");
    printf("ESP: "); printInt(tasks[numTasks].cpustate->esp); printf(", ");
    printf("EIP: "); printInt(tasks[numTasks].cpustate->eip); printf(", ");
    printf("CS: "); printInt(tasks[numTasks].cpustate->cs); printf("\n");

    // Set up the child process
    tasks[numTasks].pid = numTasks;
    tasks[numTasks].ppid = tasks[currentTask].pid;
    tasks[numTasks].state = task::ProcessState::READY;

    printProcessTable();

    // Set return value for the parent process
    parentCPUState->ecx = tasks[numTasks].pid;

    numTasks++;

    printf("Forked successfully\n");
    return tasks[numTasks - 1].pid; // Return the child's PID
}*/


int TaskManager::waitPid(int pid) {
    // Find the child process with the given PID
    int childIndex = -1;
    for (int i = 0; i < numTasks; i++) {
        if (tasks[i].pid == pid) {
            childIndex = i;
            break;
        }
    }

    // If the child process is not found, return -1
    if (childIndex == -1) {
        return -1;
    }

    // Make the process state blocked until it finishes
    tasks[childIndex].state = task::ProcessState::BLOCKED;

    // Return the child process's PID
    return tasks[childIndex].pid;
}   

int TaskManager::getCurrentTaskPid(){
    return tasks[currentTask].getPid();
}

void TaskManager::sleep(){
    for(int i = 0; i < 1000000000; i++);
}

void TaskManager::runChilds(){
    printf("Childs are going to be ready\n");
    for (int i = 0; i < numTasks; i++) {
        if (tasks[i].ppid == tasks[currentTask].pid) {
            tasks[i].state = task::ProcessState::READY;
        }
    }
}