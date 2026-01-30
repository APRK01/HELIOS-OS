#include "process.h"
#include "console.h"
#include "heap.h"
#include "string.h"

task_t *current_task = NULL;
static task_t *task_list = NULL;
static uint64_t next_pid = 1;

// Defined in entry.S
extern void switch_to(task_t *prev, task_t *next);

// Initialize the first kernel task (idle/shell)
void process_init(void) {
  task_t *kernel_task = (task_t *)malloc(sizeof(task_t));
  if (!kernel_task) {
    console_print("PANIC: Failed to allocate kernel task\n");
    return;
  }

  // The current flow is the kernel task
  kernel_task->pid = 0;
  kernel_task->state = TASK_RUNNING;
  k_strcpy(kernel_task->name, "Kernel");
  kernel_task->sp = NULL;          // Current SP (will be saved on switch)
  kernel_task->next = kernel_task; // Circular list for Round Robin

  current_task = kernel_task;
  task_list = kernel_task;

  console_print("PROCESS: Multitasking Initialized.\n");
}

task_t *process_create(void (*entry)(void), const char *name) {
  task_t *new_task = (task_t *)malloc(sizeof(task_t));
  if (!new_task)
    return NULL;

  // Allocate stack (4KB)
  uint64_t *stack = (uint64_t *)malloc(4096);
  if (!stack) {
    free(new_task);
    return NULL;
  }

  // Calculate top of stack (stacks grow down)
  uint64_t stack_top = (uint64_t)stack + 4096;

  // Prepare initial stack frame mimicking switch_to's expectation
  // We pushed 12 registers (x19-x30). x30 is LR (return address).
  // We want 'ret' in switch_to to jump to 'entry'.
  stack_top -= 96; // Reserve space for context

  // Initialize context
  // x29, x30 are at offset 80
  // We place 'entry' address into x30 (LR) location
  // So when switch_to does 'ldp x29, x30... ret', it returns to 'entry'

  uint64_t *context = (uint64_t *)stack_top;
  // Indices: 0-1 (19,20), ..., 10-11 (29,30)
  context[11] = (uint64_t)entry; // x30 (LR)

  new_task->sp = (uint64_t *)stack_top;
  new_task->pid = next_pid++;
  new_task->state = TASK_READY;
  k_strcpy(new_task->name, name);

  // Add to list (Round Robin insert at head for simplicity)
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
  // Simple Round Robin: find next READY task
  // Since we only have RUNNING or READY in the active list mostly
  while (next != current_task) {
    if (next->state == TASK_READY)
      break;
    next = next->next;
  }

  if (next == current_task)
    return; // No other task needed

  task_t *prev = current_task;
  current_task = next;
  current_task->state = TASK_RUNNING;
  prev->state = TASK_READY;

  switch_to(prev, next);
}

void yield(void) { schedule(); }
