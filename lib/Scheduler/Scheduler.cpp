#include "Scheduler.h"
#include <Arduino.h>

typedef struct {
  int32_t period;
  int32_t remaining_time;
  uint8_t is_running;
  task_cb callback;
} task_t;

task_t tasks[MAXTASKS];

uint32_t last_runtime;

void SchedulerInit() { last_runtime = millis(); }

void SchedulerStartTask(int16_t delay, int16_t period, task_cb task) {
  static uint8_t id = 0;
  if (id < MAXTASKS) {
    tasks[id].remaining_time = delay;
    tasks[id].period = period;
    tasks[id].is_running = 1;
    tasks[id].callback = task;
    id++;
  }
}

uint32_t SchedulerDispatch() {
  uint8_t i;

  uint32_t now = millis();
  uint32_t elapsed = now - last_runtime;
  last_runtime = now;
  task_cb t = NULL;
  uint32_t idle_time = 0xFFFFFFFF;

  // update each task's remaining time, and identify the first ready task (if
  // there is one).
  for (i = 0; i < MAXTASKS; i++) {
    if (tasks[i].is_running) {
      // update the task's remaining time
      tasks[i].remaining_time -= elapsed;
      if (tasks[i].remaining_time <= 0) {
        if (t == NULL) {
          // if this task is ready to run, and we haven't already selected a
          // task to run, select this one.
          t = tasks[i].callback;
          tasks[i].remaining_time += tasks[i].period;
        }
        idle_time = 0;
      } else {
        idle_time = min((uint32_t)tasks[i].remaining_time, idle_time);
      }
    }
  }
  if (t != NULL) {
    // If a task was selected to run, call its function.
    t();
  }
  return idle_time;
}
