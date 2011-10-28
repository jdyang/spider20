#include "../../dns_client.h"
#include "../../spider.h"
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

    DnsClient dns_client;
	CSpiderConf conf;
	conf.urlpool_empty_sleep_time = 500;
	conf.max_url_len = 4096;
	conf.dns_host = "61.130.254.34";
	conf.dns_port = 53;
	conf.dns_sleep_interval = 300;
	
	if (-1 == dns_client.init(&conf))
	{
		printf("dns init error.\n");
		return 0;
	}
	signal(SIGINT, end_handler);
    printf("start in %lld\n", time(NULL));
	for (i=0; i<5; i++)
	{
		if (pthread_create(&ths[i], NULL, write_thread, &dns_client) != 0)
		{
			printf("pthread create error\n");
			return 0;
		}
	}
	pthread_join(ths[0], NULL);

	return 0;
}

void* write_thread(void* arg)
{

	while (true)
	{
		usleep(300000);
	}

	return NULL;
}
