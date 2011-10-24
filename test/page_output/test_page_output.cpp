#include "page_output.h"
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

using namespace std;

void* write_thread(void* arg);

void end_handler(int sig)
{
	printf("%lld\n", time(NULL));
	exit(0);
}


int main(int argc, char** argv)
{
	pthread_t ths[5];
	int i;

    printf("%d\n", sizeof(long));
	exit(0);
    CPageOutput page_output;
	if (-1 == page_output.init(string("./")))
	{
		printf("page output init error.\n");
		return 0;
	}
	signal(SIGINT, end_handler);
    printf("%lld\n", time(NULL));
	for (i=0; i<5; i++)
	{
		if (pthread_create(&ths[i], NULL, write_thread, &page_output) != 0)
		{
			printf("pthread create error\n");
			return 0;
		}
	}
	pthread_join(ths[0], NULL);
	if (-1 == page_output.destroy())
	{
		printf("page output destroy error\n");
	}

	return 0;
}

void* write_thread(void* arg)
{
	CPageOutput* ppo = (CPageOutput*)arg;

	char* p = (char*)malloc(100 * 1024);
	if (!p) 
	{
		printf("malloc error\n");
		exit(0);
	}
    memset(p, 1, 100*1024);


	while (true)
	{
        ppo->append(p, 102400);
		//usleep(300000);
	}

	return NULL;
}
