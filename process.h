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
  uint64_t *sp;        
  uint64_t pid;        
  task_state_t state;  
  char name[32];       
  struct task *next;   
} task_t;

 
extern task_t *current_task;

 
void process_init(void);
task_t *process_create(void (*entry)(void), const char *name);
void schedule(void);
void yield(void);

#endif  
