#include "../../urlpool.h"
#include "../../spider_common.h"

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "uc_url.h"

using namespace std;

void end_handler(int sig)
{
	printf("%ld\n", time(NULL));
	exit(0);
}

void* select_thread(void* arg)
{
	UrlPool* pool = (UrlPool*)arg;
	
	while(true)
	{
		pool->pop_url();
		usleep(100000);

	}
}

void* count_thread(void* arg)
{
    UrlPool* pool = (UrlPool*)arg;

    while(true)
    {
        usleep(1000000);
		pool->print_pool();
    }
}


void* work_thread(void* arg)
{
	UrlPool* pool = (UrlPool*)arg;

	while(true)
	{
		string url = "http://list.wooha.com/da8/da845446c2964bab.html";
			ucUrl ucurl(url);
            ucurl.build();
			UrlInfo ui;
            ui.domain = ucurl.get_domain();
            ui.site = ucurl.get_site();
            ui.last_crawl_time = 0;
		pool->push_url(ui);
		usleep(30000);
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

	CSpiderConf conf;
	conf.urlpool_empty_sleep_time = 500;
	conf.max_url_len = 4096;
	UrlPool pool;
	pool.set_conf(&conf);
	pool.load_urls(argv[1]);
	
	signal(SIGINT, end_handler);
	printf("%ld\n", time(NULL));
	if(pthread_create(&ths[0], NULL, select_thread, &pool) != 0)
	{
		printf("pthread_create error: select_thread error.\n");
		return 0;
	}

	if(pthread_create(&ths[0], NULL, count_thread, &pool) != 0)
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



