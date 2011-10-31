/*
 @file: spider_statis.cpp
 @brief: spider2.0 统计接口实现
 @author: stanshen
 @create date: 2011.10.25
*/

#include "spider_statis.h"

using namespace std;

CSpiderStatis::CSpiderStatis()
{

}

CSpiderStatis::~CSpiderStatis()
{

}

int CSpiderStatis::init()
{
	m_CPQ_url_num = 0;
	m_IPQ_url_num = 0;
	m_COQ_url_num = 0;
	m_IOQ_url_num = 0;
	m_SQ_url_num = 0;
	
	pthread_mutex_init(&m_cate_mutex, NULL);
	pthread_mutex_init(&m_item_mutex, NULL);
	
	return 0;
}

int CSpiderStatis::set_statis_to_file(char* file_path)
{
	if(file_path == NULL)
	{
		printf("set_statis_to_file error: file_path may be illegal");
		return -1;
	}

	FILE* fp = fopen(file_path, "a");
	if(fp == NULL)
	{
		printf("set_statis_to_file error: can not open/create file: %s", file_path);
		return -1;
	}
	
	

	fprintf(fp, "%lld\t", time(NULL));
	fprintf(fp, "CPQ=%d\tCOQ=%d\tIPQ=%d\tIOQ=%d\tSQ=%d\n", get_cpq_url_num(), get_coq_url_num(),
	             get_ipq_url_num(), get_ioq_url_num(), get_sq_url_num());
	
	set<string>::iterator set_iter;
	for(set_iter = m_domain.begin(); set_iter != m_domain.end(); set_iter++)
	{
		fprintf(fp, "domain=%s\tcate_done=%d\tcate_sel=%d\titem_done=%d\titem_sel=%d\n",(*set_iter).c_str(),
		            get_domain_cate_done_num(*set_iter), get_domain_cate_select_num(*set_iter),
					get_domain_item_done_num(*set_iter), get_domain_item_select_num(*set_iter));
	}
	
	return 0;
}


int CSpiderStatis::get_domain_cate_done_num(string domain)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return 0;
	if((map_iter = m_domain_cate_done.find(domain)) == m_domain_cate_done.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

int CSpiderStatis::get_domain_item_done_num(string domain)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return 0;
	if((map_iter = m_domain_item_done.find(domain)) == m_domain_item_done.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

int CSpiderStatis::get_domain_cate_select_num(string domain)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return 0;
	if((map_iter = m_domain_cate_select.find(domain)) == m_domain_cate_select.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

int CSpiderStatis::get_domain_item_select_num(string domain)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return 0;
	if((map_iter = m_domain_item_select.find(domain)) == m_domain_item_select.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

void CSpiderStatis::set_domain_cate_done_num(string domain, int cate_num)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return;
	if((map_iter = m_domain_cate_done.find(domain)) == m_domain_cate_done.end())
	{
		m_domain_cate_done.insert(make_pair(domain, cate_num));
	}
	else
	{
		(*map_iter).second = cate_num;
	}
}

void CSpiderStatis::set_domain_item_done_num(string domain, int item_num)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return;
	if((map_iter = m_domain_item_done.find(domain)) == m_domain_item_done.end())
	{
		m_domain_item_done.insert(make_pair(domain, item_num));
	}
	else
	{
		(*map_iter).second = item_num;
	}
}

void CSpiderStatis::set_domain_cate_select_num(string domain, int cate_num)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return;
	if((map_iter = m_domain_cate_select.find(domain)) == m_domain_cate_select.end())
	{
		m_domain_cate_select.insert(make_pair(domain, cate_num));
	}
	else
	{
		(*map_iter).second = cate_num;
	}
}

void CSpiderStatis::set_domain_item_select_num(string domain, int item_num)
{
	map<string, int>::iterator map_iter;
	if(domain.c_str() == NULL)
		return;
	if((map_iter = m_domain_item_select.find(domain)) == m_domain_item_select.end())
	{
		m_domain_item_select.insert(make_pair(domain, item_num));
	}
	else
	{
		(*map_iter).second = item_num;
	}
}

int CSpiderStatis::update_domain_cate_done(string domain)
{
	if(domain.c_str() == NULL)
		return 0;

	if(pthread_mutex_lock(&m_cate_mutex) != 0)
	{
        // TODO change printf to log
		printf("update_domain_cate_done error->pthread_mutex_lock error: lock fail.\n");
		//SDLOG_WARNING(SP_IFNAME, "update_domain_cate_done error->pthread_mutex_lock error: lock fail.");
		return -1;
	}

	map<string, int>::iterator map_iter;
	if((map_iter = m_domain_cate_done.find(domain)) != m_domain_cate_done.end())
	{
		(*map_iter).second++;
	}
	else
	{
		m_domain_cate_done.insert(make_pair(domain, 1));
	}

	pthread_mutex_unlock(&m_cate_mutex);
	
	return 0;
}


int CSpiderStatis::update_domain_item_done(string domain)
{
	if(domain.c_str() == NULL)
		return 0;

	if(pthread_mutex_lock(&m_item_mutex) != 0)
	{
		printf("update_domain_item_done error->pthread_mutex_lock error: lock fail.\n");
		return -1;
	}

	map<string, int>::iterator map_iter;
	if((map_iter = m_domain_item_done.find(domain)) != m_domain_item_done.end())
	{
		(*map_iter).second++;
	}
	else
	{
		m_domain_item_done.insert(make_pair(domain, 1));
	}

	pthread_mutex_unlock(&m_item_mutex);
	
	return 0;
}






