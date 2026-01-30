#ifndef PROCESS_H
#define PROCESS_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
  TASK_READY,
  TASK_RUNNING,
  TASK_BLOCKED,
  TASK_TERMINATED
} task_state_t;

typedef struct task {
  uint64_t *sp;       // Saved Stack Pointer (Context)
  uint64_t pid;       // Process ID
  task_state_t state; // Current State
  char name[32];      // Task Name
  struct task *next;  // Next task in the list (Round Robin)
} task_t;

// Global pointer to the currently running task
extern task_t *current_task;

// Function prototypes
void process_init(void);
task_t *process_create(void (*entry)(void), const char *name);
void schedule(void);
void yield(void);

#endif // PROCESS_H
