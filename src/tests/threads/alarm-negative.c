/* Checks that when the alarm clock wakes up threads, the
   higher-priority threads run first. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static thread_func alarm_priority_thread;
static int64_t wake_time;
static struct semaphore wait_sema;

void
test_alarm_negative (void) 
{
  int i;
  
  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  wake_time = timer_ticks () + 5 * TIMER_FREQ;
  sema_init (&wait_sema, 0);
  
  /*for (i = 0; i < 10; i++) 
    {
      int priority = PRI_DEFAULT - (i + 5) % 10 - 1;
      char name[16];
      snprintf (name, sizeof name, "priority %d", priority);
      thread_create (name, priority, alarm_priority_thread, NULL);
    }
  printf("starter! (%d)%s\n",(thread_current ()->priority),(thread_current ()->name)); */
  thread_set_priority (PRI_MIN);
  printf("I am the lowest thread. Now I start some more!(%d)%s\n",(thread_current ()->priority),(thread_current ()->name)); 
  for (i = 1; i < 63; i++) 
    {
      int priority = i; //definitely weird order and sema down before others.
      char name[16];
      snprintf (name, sizeof name, "priority %d", priority);
      thread_create (name, priority, alarm_priority_thread, NULL);
    }
  for (i = 0; i < 10; i++)
    sema_up (&wait_sema);
  printf("all accounted for.\n");
}

static void
alarm_priority_thread (void *aux UNUSED) 
{
  
  sema_down (&wait_sema);
  /* Print a message on wake-up. */
  printf ("Thread %s woke up.\n", thread_name ());

  sema_up (&wait_sema);
}
