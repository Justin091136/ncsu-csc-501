#ifndef H_LAB1_H_
#define H_LAB1_H_

#define DEFAULTSCHED 0
#define AGESCHED 1 
#define LINUXSCHED 2

extern int scheduler_class;

void setschedclass(int sched_class);
int getschedclass();

#endif 