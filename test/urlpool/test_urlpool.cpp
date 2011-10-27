#include "../../urlpool.h"

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

using namespace std;

void end_handler(int sig)
{
	printf("%lld\n", time(NULL));
	exit(0);
}

void* select_thread(void* arg)
{
	UrlPool* pool = (UrlPool*)arg;
	
	while(true)
	{
		
		usleep(100000);

	}
}
void* work_thread(void* arg)
{
	UrlPool* pool = (UrlPool*)arg;

	while(true)
	{
		usleep(300000);
	}
	return NULL;
}


int main(int argc, char* argv[])
{
	pthread_t ths[6];
	int i;

	if (argc < 2){
		cout << "./test_urlpool ./input.url" << endl;
		exit(-1);
	}

	SpiderConf conf;
	conf.urlpool_empty_sleep_time = 500;
	conf.max_url_len = 4096;
	UrlPool pool;
	pool.set_conf(&conf);
	pool.load_urls(argv[1]);
	
	signal(SIGINT, end_handler);
	printf("%lld\n", time(NULL));
	if(pthread_create(&ths[0], NULL, select_thread, &pool) != 0)
	{
		printf("pthread_create error: select_thread error.\n");
		return 0;
	}
	for(i = 1; i < 5; i++)
	{
		if(pthread_create(&ths[i], NULL, work_thread, &pool) != 0)
		{
			printf("pthread_create error: work_thread error.\n");
			return 0;
		}
	}

	pthread_join(ths[0], NULL);
	
	return 0;

}



