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

FuncRet CSpiderStatis::init()
{
	m_CPQ_url_num = 0;
	m_IPQ_url_num = 0;
	m_COQ_url_num = 0;
	m_IOQ_url_num = 0;
	m_SQ_url_num = 0;
	pthread_mutex_init(&m_cate_mutex, NULL);
	pthread_mutex_init(&m_item_mutex, NULL);
}


uint32_t CSpiderStatis::get_domain_cate_done_num(string domain)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_cate_done.find(domain)) == m_domain_cate_done.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

uint32_t CSpiderStatis::get_domain_item_done_num(string domain)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_item_done.find(domain)) == m_domain_item_done.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

uint32_t CSpiderStatis::get_domain_cate_select_num(string domain)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_cate_select.find(domain)) == m_domain_cate_select.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

uint32_t CSpiderStatis::get_domain_item_select_num(string domain)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_item_select.find(domain)) == m_domain_item_select.end())
	{
		return 0;
	}
	else
	{
		return (*map_iter).second;
	}
}

void CSpiderStatis::set_domain_cate_done_num(string domain, uint32_t cate_num)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_cate_done.find(domain)) == m_domain_cate_done.end())
	{
		m_domain_cate_done.insert(make_pair(domain, cate_num));
	}
	else
	{
		(*map_iter).second = cate_num;
	}
}

void CSpiderStatis::set_domain_item_done_num(string domain, uint32_t item_num)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_item_done.find(domain)) == m_domain_item_done.end())
	{
		m_domain_item_done.insert(make_pair(domain, item_num));
	}
	else
	{
		(*map_iter).second = item_num;
	}
}

void CSpiderStatis::set_domain_cate_select_num(string domain, uint32_t cate_num)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_cate_select.find(domain)) == m_domain_cate_select.end())
	{
		m_domain_cate_select.insert(make_pair(domain, cate_num));
	}
	else
	{
		(*map_iter).second = cate_num;
	}
}

void CSpiderStatis::set_domain_item_select_num(string domain, uint32_t item_num)
{
	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_item_select.find(domain)) == m_domain_item_select.end())
	{
		m_domain_item_select.insert(make_pair(domain, item_num));
	}
	else
	{
		(*map_iter).second = item_num;
	}
}

FuncRet CSpiderStatis::update_domain_cate_done(string domain)
{
	if(pthread_mutex_lock(&m_cate_mutex) != 0)
	{
		printf("update_domain_cate_done error->pthread_mutex_lock error: lock fail.\n");
		return FR_FALSE;
	}

	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_cate_done.find(domain)) != m_domain_cate_done.end())
	{
		(*map_iter).second++;
	}
	else
	{
		m_domain_cate_done.insert(make_pair(domain, 1));
	}

	pthread_mutex_unlock(&m_cate_mutex);
	
	return FR_OK;
}


FuncRet CSpiderStatis::update_domain_item_done(string domain)
{
	if(pthread_mutex_lock(&m_item_mutex) != 0)
	{
		printf("update_domain_item_done error->pthread_mutex_lock error: lock fail.\n");
		return FR_FALSE;
	}

	map<string, uint32_t>::iterator map_iter;
	if((map_iter = m_domain_item_done.find(domain)) != m_domain_item_done.end())
	{
		(*map_iter).second++;
	}
	else
	{
		m_domain_item_done.insert(make_pair(domain, 1));
	}

	pthread_mutex_unlock(&m_item_mutex);
	
	return FR_OK;
}






