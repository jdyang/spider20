#include "../../dns_client.h"
#include "../../spider.h"
#include <string>
#include <set>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

void* write_thread(void* arg);

void end_handler(int sig)
{
	printf("%ld\n", time(NULL));
	exit(0);
}

set<string> g_site_set;
vector<string> g_site_vector;

int main(int argc, char** argv)
{
	pthread_t ths[5];
	int i;
	srand(time(0));

    CDnsClient dns_client;
	CSpiderConf conf;
	conf.urlpool_empty_sleep_time = 500;
	conf.max_url_len = 4096;
	conf.dns_host = "61.130.254.34";
	conf.dns_port = 53;
	conf.dns_sleep_interval = 300;
	
	g_site_set.insert("0591.doido.com");
	g_site_set.insert("search.paipai.com");
	g_site_set.insert("http://www.baidu.com/");
	g_site_set.insert("categoryb.dangdang.com");
	
	set<string>::iterator it;
	it = g_site_set.begin();
	
	g_site_vector.assign(it, g_site_set.end());
	
	if (-1 == dns_client.init(&conf))
	{
		printf("dns init error.\n");
		return 0;
	}
	signal(SIGINT, end_handler);
    printf("start in %ld\n", time(NULL));
	write_thread(&dns_client);

	for (i=0; i<5; i++)
	{
		if (pthread_create(&ths[i], NULL, write_thread, &dns_client) != 0)
		{
			printf("pthread create error\n");
			return 0;
		}
	}
	dns_client.query_site_ip(&g_site_set);
	
	pthread_join(ths[0], NULL);

	return 0;
}

void* write_thread(void* arg)
{
	CDnsClient* dns = (CDnsClient*)arg;
	while (true)
	{
		usleep(300000);
		cout << dns->get_ip(g_site_vector[rand()%g_site_vector.size()]) << endl;
		
	}

	return NULL;
}
