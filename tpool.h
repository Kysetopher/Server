

#ifndef _TPOOL_
#define _TPOOL_


int tpool_init(void (*process_task_Ptr)(int));
int tpool_add_task(int newtask);

#endif
