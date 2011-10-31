#include "../../url_output.h"
#include "../../spider.h"
#include <string>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

using namespace std;

void* write_thread(void* arg);

void end_handler(int sig)
{
	printf("%ld\n", time(NULL));
	exit(0);
}

int main(int argc, char** argv)
{
	pthread_t ths[5];
	int i;

    CSpiderConf conf;
	conf.spider_name = "spider02";
	conf.item_output_path = "item.list";

    CUrlOutput url_output;
	if (-1 == url_output.init(&conf))
	{
		printf("page output init error.\n");
		return 0;
	}
	signal(SIGINT, end_handler);
    printf("%ld\n", time(NULL));
	for (i=0; i<5; i++)
	{
		if (pthread_create(&ths[i], NULL, write_thread, &url_output) != 0)
		{
			printf("pthread create error\n");
			return 0;
		}
	}
	pthread_join(ths[0], NULL);
	if (-1 == url_output.destroy())
	{
		printf("page output destroy error\n");
	}

	return 0;
}

void* write_thread(void* arg)
{
	CUrlOutput* puo = (CUrlOutput*)arg;
	string url = "http:/product.dangdang.com/product.aspx?id=2434";

	while (true)
	{
        puo->append(url);
		usleep(300000);
	}

	return NULL;
}
