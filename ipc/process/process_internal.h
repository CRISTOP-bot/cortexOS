#ifndef PROCESS_INTERNAL_H
#define PROCESS_INTERNAL_H
#include "process.h"
extern struct process processes[MAX_PROCESSES];
struct process *find_process(int pid);
#endif
