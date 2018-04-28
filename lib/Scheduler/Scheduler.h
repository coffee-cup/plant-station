#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

#include <inttypes.h>

/// Up to this many tasks can be run, in addition to the idle task
#define MAXTASKS 8

/// A task callback function
typedef void (*task_cb)();

/**
 * Initialise the scheduler.  This should be called once in the setup routine.
 */
void SchedulerInit();

/**
 * Start a task.
 * The function "task" will be called roughly every "period" milliseconds
 * starting after "delay" milliseconds. The scheduler does not guarantee that
 * the task will run as soon as it can.  Tasks are executed until completion. If
 * a task misses its scheduled execution time then it simply executes as soon as
 * possible.  Don't pass stupid values (e.g. negatives) to the parameters.
 *
 * \param id The tasks ID number.  This must be between 0 and MAXTASKS (it is
 * used as an array index). \param delay The task will start after this many
 * milliseconds. \param period The task will repeat every "period" milliseconds.
 * \param task The callback function that the scheduler is to call.
 */
void SchedulerStartTask(int16_t delay, int16_t period, task_cb task);

/**
 * Go through the task list and run any tasks that need to be run.  The main
 * function should simply be this function called as often as possible, plus any
 * low-priority code that you want to run sporadically.
 */
uint32_t SchedulerDispatch();

#endif /* SCHEDULER_H_ */
