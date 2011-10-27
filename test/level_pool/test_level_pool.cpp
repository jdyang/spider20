#include "../../level_pool.h"
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

using namespace std;

void* crawl_thread(void* arg);

void end_handler(int sig)
{
	printf("%lld\n", time(NULL));
	exit(0);
}


int main(int argc, char** argv)
{
	pthread_t work[5];
	int i;

    CLevelPool level_pool;
	SLevelInfo li;

	if (-1 == level_pool.init())
	{
		printf("level pool init error.\n");
		return 0;
	}
	signal(SIGINT, end_handler);
    printf("%lld\n", time(NULL));
	for (i=0; i<5; i++)
	{
		if (pthread_create(&work[i], NULL, crawl_thread, &level_pool) != 0)
		{
			printf("pthread create error\n");
			return 0;
		}
	}

    string site = "product.dangdang.com";
	li.max_crawl_thread_count = 3;
	li.crawl_thread_count = 0;
	li.last_crawl_time = time(NULL);
	li.crawl_interval = 300;
	level_pool.insert_level_info(site, li);

    site = "www.360buy.com";
	li.max_crawl_thread_count = 3;
	li.crawl_thread_count = 0;
	li.last_crawl_time = time(NULL);
	li.crawl_interval = 300;
	level_pool.insert_level_info(site, li);

	pthread_join(work[0], NULL);
	if (-1 == level_pool.destroy())
	{
		printf("level destroy error\n");
	}

	return 0;
}

void* crawl_thread(void* arg)
{
	CLevelPool* pool = (CLevelPool*)arg;
	string site;

	while (true)
	{
		site = "product.dangdang.com";
		if (pool->is_crawl_enabled(site))
		{
			printf("crawl enabled\n");
			pool->finish_crawl(site);
		}
		else
		{
			printf("crawl forbiden\n");
		}

		sleep(2);
	}

	return NULL;
}
