Grant Borthwick
CIS 520
Project 0

To implement alarm-nega, I had to do three things.

1. add alarm-mega.ck which is an exact copy of alarm-multiple with check_alarm (70); instead of check_alarm (7);

2. Add {"alarm-mega", test_alarm_mega}, into tests.c

3. Add extern test_func test_alarm_mega; into tests.h

4. Add alarm-mega to the list of test names in Make.tests

5. Add 
void
test_alarm_mega (void)
{
  test_sleep (5, 70);
} 

into alarm-wait.c
