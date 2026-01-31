#include "process.h"
#include "console.h"
#include "heap.h"
#include "string.h"

task_t *current_task = NULL;
static task_t *task_list = NULL;
static uint64_t next_pid = 1;

 
extern void switch_to(task_t *prev, task_t *next);

 
void process_init(void) {
  task_t *kernel_task = (task_t *)malloc(sizeof(task_t));
  if (!kernel_task) {
    console_print("PANIC: Failed to allocate kernel task\n");
    return;
  }

   
  kernel_task->pid = 0;
  kernel_task->state = TASK_RUNNING;
  k_strcpy(kernel_task->name, "Kernel");
  kernel_task->sp = NULL;           
  kernel_task->next = kernel_task;  

  current_task = kernel_task;
  task_list = kernel_task;

  console_print("PROCESS: Multitasking Initialized.\n");
}

task_t *process_create(void (*entry)(void), const char *name) {
  task_t *new_task = (task_t *)malloc(sizeof(task_t));
  if (!new_task)
    return NULL;

   
  uint64_t *stack = (uint64_t *)malloc(4096);
  if (!stack) {
    free(new_task);
    return NULL;
  }

   
  uint64_t stack_top = (uint64_t)stack + 4096;

   
   
   
  stack_top -= 96;  

   
   
   
   

  uint64_t *context = (uint64_t *)stack_top;
   
  context[11] = (uint64_t)entry;  

  new_task->sp = (uint64_t *)stack_top;
  new_task->pid = next_pid++;
  new_task->state = TASK_READY;
  k_strcpy(new_task->name, name);

   
  struct task *tail = task_list;
  while (tail->next != task_list) {
    tail = tail->next;
  }
  new_task->next = task_list;
  tail->next = new_task;

  return new_task;
}

void schedule(void) {
  if (!current_task)
    return;

  task_t *next = current_task->next;
   
   
  while (next != current_task) {
    if (next->state == TASK_READY)
      break;
    next = next->next;
  }

  if (next == current_task)
    return;  

  task_t *prev = current_task;
  current_task = next;
  current_task->state = TASK_RUNNING;
  prev->state = TASK_READY;

  switch_to(prev, next);
}

void yield(void) { schedule(); }
