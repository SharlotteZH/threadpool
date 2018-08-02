#include "threadpool.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

static void
print_dispatch(void* args)
{
	int* j = (int *)args;
	printf("%d\n", *j);
	delete j;

   struct timespec tim, tim2;
   tim.tv_sec = 0;
   tim.tv_nsec = rand()%100000000;

   if(nanosleep(&tim , &tim2) < 0 ) { //randomly sleep, should see print out of order
      printf("Nano sleep system call failed \n");
   }
}

int main(int argc, char** argv)
{
	threadpool pool = create_threadpool(10);
	if (!pool) return -1;
	for (int i=0;i<100;i++) {
		int* j = new int;
		*j = i;
		if (dispatch(pool, print_dispatch, j) < 0)
			break;
	}

	destroy_threadpool(pool);
	return 0;
}