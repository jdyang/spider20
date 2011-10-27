#include "../../selected_queue.h"
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

using namespace std;

void* select_thread(void* arg);

void* crawl_thread(void* arg);

int main(int argc, char** argv)
{
	CSelectedQueue sq;
	pthread_t works[5];
	pthread_t select;

	if (-1 == sq.init())
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


	while (true)
	{
	    qi.url = "http://item.taobao.com/id=234";
	    qi.fail_count = 0;
	    qi.dns_count = 0;
	    qi.last_crawl_time = 23123123123;

		psq->push(qi);
		sleep(2);
	}
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
            sleep(1);
			continue;
		}
		printf("%s\n", qi.url.c_str());
	}
	return NULL;
}
