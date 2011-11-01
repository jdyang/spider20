#include "../../selected_queue.h"
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>

using namespace std;

void* select_thread(void* arg);

void* crawl_thread(void* arg);

long get_time_now(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000000 + tv.tv_usec;
}
int main(int argc, char** argv)
{
	CSelectedQueue sq;
	pthread_t works[5];
	pthread_t select;

	if (-1 == sq.init(100000))
	{
		printf("selected queue init error\n");
		return 0;
	}

	for (int i=0; i<5; i++)
	{
		if (pthread_create(&works[i], NULL, crawl_thread, &sq) != 0)
		{
			printf("crawl pthread create error\n");
			return 0;
		}
	}

    if (pthread_create(&select, NULL, select_thread, &sq) != 0)
	{
		printf("selected pthread create error\n");
		return 0;
	}

	pthread_join(select, NULL);

	return 0;
}

void* select_thread(void* arg)
{
	CSelectedQueue* psq = (CSelectedQueue*)arg;

    SSQItem qi;


    int count = 0;
	printf("%ld\n", get_time_now());
	while (true)
	{
	    qi.url = "http://item.taobao.com/id=234";
	    qi.fail_count = 0;
	    qi.dns_count = 0;
	    qi.last_crawl_time = 23123123123;

		psq->push(qi);
		count++;
		if (count == 100000){
			break;
		}
		//sleep(2);
	}
	printf("%ld\n", get_time_now());
	return NULL;
}

void* crawl_thread(void* arg)
{
	CSelectedQueue* psq = (CSelectedQueue*)arg;

	SSQItem qi;

	while (true)
	{
		if (!psq->pop(qi))
		{
			printf("selected queue is empty\n");
            usleep(200000);
			continue;
		}
		psq->push(qi);
		printf("%s\n", qi.url.c_str());
	}
	return NULL;
}

