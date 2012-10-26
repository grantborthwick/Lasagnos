/* Open a file. */

#include <syscall.h>
#include <stdio.h>
#include "tests/lib.h"
#include "tests/main.h"

void
test_main (void) 
{
  printf("Opening Sample.txt");
  int handle = open ("sample.txt");
  if (handle < 2)
    fail ("open() returned %d", handle);
}
