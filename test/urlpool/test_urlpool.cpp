/*
 @file: test_spider_statis.cpp
 @brief: 测试程序，测试spider统计部分功能
 @author: stanshen
 @create date: 2011.10.25
*/

#include "../../spider_statis.h"

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
	CSpiderStatis* spider_statis = (CSpiderStatis*)arg;
	int cpq_url_num, coq_url_num, ipq_url_num, ioq_url_num, sq_url_num, cate_select_num, item_select_num;

	while(true)
	{
		cpq_url_num = rand()%100;
		coq_url_num = rand()%100;
		ipq_url_num = rand()%100;
		ioq_url_num = rand()%100;
		sq_url_num = rand()%100;
		cate_select_num = rand()%100;
		item_select_num = rand()%100;
		
		spider_statis->set_domain_cate_select_num("xxx.com", cate_select_num);
		spider_statis->set_domain_item_select_num("xxx.com", item_select_num);
		spider_statis->set_cpq_url_num(cpq_url_num);
		spider_statis->set_ipq_url_num(ipq_url_num);
		spider_statis->set_coq_url_num(coq_url_num);
		spider_statis->set_ioq_url_num(ioq_url_num);
		spider_statis->set_sq_url_num(sq_url_num);

		cout<<time(NULL)<<" select_thread [domain=xxx.com] ";
		cout<<"cpq="<<spider_statis->get_cpq_url_num()<<" ";
		cout<<"ipq="<<spider_statis->get_ipq_url_num()<<" ";
		cout<<"coq="<<spider_statis->get_coq_url_num()<<" ";
		cout<<"ioq="<<spider_statis->get_ioq_url_num()<<" ";
		cout<<"sq="<<spider_statis->get_sq_url_num()<<" ";
		cout<<"cate_sel="<<spider_statis->get_domain_cate_select_num("xxx.com")<<" ";
		cout<<"item_sel="<<spider_statis->get_domain_item_select_num("xxx.com")<<" ";
		cout<<"cate_done="<<spider_statis->get_domain_cate_done_num("xxx.com")<<" ";
		cout<<"item_done="<<spider_statis->get_domain_item_done_num("xxx.com")<<endl;
		
		usleep(100000);

	}
}
void* work_thread(void* arg)
{
	CSpiderStatis* spider_statis = (CSpiderStatis*)arg;

	while(true)
	{
		if(rand()%2 == 0)
			spider_statis->update_domain_cate_done("xxx.com");
		else
		{
			spider_statis->update_domain_item_done("xxx.com");
		}

		cout<<time(NULL)<<" work_thread: [domain=xxx.com] ";
		cout<<"cate_done="<<spider_statis->get_domain_cate_done_num("xxx.com")<<" ";
		cout<<"item_done="<<spider_statis->get_domain_item_done_num("xxx.com")<<endl;
		usleep(300000);
	}
	return NULL;
}


int main(int argc, char* argv[])
{
	pthread_t ths[6];
	int i;

	CSpiderStatis spider_statis;
	spider_statis.init();
	
	signal(SIGINT, end_handler);
	printf("%lld\n", time(NULL));
	if(pthread_create(&ths[0], NULL, select_thread, &spider_statis) != 0)
	{
		printf("pthread_create error: select_thread error.\n");
		return 0;
	}
	for(i = 1; i < 5; i++)
	{
		if(pthread_create(&ths[i], NULL, work_thread, &spider_statis) != 0)
		{
			printf("pthread_create error: work_thread error.\n");
			return 0;
		}
	}

	pthread_join(ths[0], NULL);
	
	return 0;

}



