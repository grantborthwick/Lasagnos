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
  int j = 0;
  
  /* This test does not work with the MLFQS. */
  ASSERT (!thread_mlfqs);

  wake_time = timer_ticks () + 5 * TIMER_FREQ;
  sema_init (&wait_sema, 0);
  for (i = 27; i<37; ++i){
    ++j;
    char name[16];
    snprintf (name, sizeof name, "priority %d", i-20);
	thread_create(name,i-20, alarm_priority_thread, NULL);
	snprintf (name, sizeof name, "priority %d", i);
    thread_create(name,i, alarm_priority_thread, NULL);
  }
  
  
  /*for (i = 0; i < 10; i++) 
    {
      int priority = PRI_DEFAULT - (i + 5) % 10 - 1;
      char name[16];
      snprintf (name, sizeof name, "priority %d", priority);
      thread_create (name, priority, alarm_priority_thread, NULL);
    }
  printf("starter! (%d)%s\n",(thread_current ()->priority),(thread_current ()->name)); 
  thread_set_priority (0);
  printf("I am the lowest thread. Now I start some more!(%d)%s\n",(thread_current ()->priority),(thread_current ()->name)); 
  for (i = 27; i < 38; i++) 
    {
      int priority = i; //definitely weird order and sema down before others.
      char name[16];
      snprintf (name, sizeof name, "priority %d", priority);
      thread_create (name, priority, alarm_priority_thread, NULL);
    }
  timer_sleep(1000);
  for (i = 0; i < 20; i++)
    sema_up (&wait_sema);*/
  printf("(%d) all accounted for.\n", (thread_current ()->priority));
  for (i = 0; i<j; ++i){
    sema_down(&wait_sema);
  }
}

static void
alarm_priority_thread (void *aux UNUSED) 
{
  
  //sema_down (&wait_sema);
  printf ("(%d)Thread %s here!.\n", (thread_current ()->priority) , thread_name ());
  sema_up(&wait_sema);

  //sema_up (&wait_sema);
}
