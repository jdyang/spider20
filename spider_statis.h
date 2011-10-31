/*
 @file: spider_statis.h
 @brief: spider2.0统计接口
 @author: stanshen
 @version: 1.0
 @date: 2011.10.25
*/

#ifndef __FANFAN_SPIDER_STATIS_H__
#define __FANFAN_SPIDER_STATIS_H__

#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <map>
#include <set>
#include <pthread.h>

//#include "spider.h"

using namespace std;

//#define FuncRet int

//#define FR_OK 0
//#define FR_FALSE -1

class CSpiderStatis
{
public:
	CSpiderStatis();
	~CSpiderStatis();
	
	int init(const char* file_path);

	int set_statis_to_file();
	int set_statis_to_file(const char* file_path);

	int get_domain_cate_done_num(string domain);
	int get_domain_item_done_num(string domain);
	int get_domain_cate_select_num(string domain);
	int get_domain_item_select_num(string domain);

	void set_domain_cate_done_num(string domain, int cate_num);
	void set_domain_item_done_num(string domain, int item_num);
	void set_domain_cate_select_num(string domain, int cate_num);
	void set_domain_item_select_num(string domain, int item_num);

	int update_domain_cate_done(string domain);
	int update_domain_item_done(string domain);
	//FuncRet update_domain_cate_select(string domain);
	//FuncRet update_domain_item_select(string domain);

	int get_cpq_url_num(){return m_CPQ_url_num;}
	int get_ipq_url_num(){return m_IPQ_url_num;}
	int get_coq_url_num(){return m_COQ_url_num;}
	int get_ioq_url_num(){return m_IOQ_url_num;}
	int get_sq_url_num(){return m_SQ_url_num;}

	void set_cpq_url_num(int url_num){m_CPQ_url_num = url_num;}
	void set_ipq_url_num(int url_num){m_IPQ_url_num = url_num;}
	void set_coq_url_num(int url_num){m_COQ_url_num = url_num;}
	void set_ioq_url_num(int url_num){m_IOQ_url_num = url_num;}
	void set_sq_url_num(int url_num){m_SQ_url_num = url_num;}

	set<string> m_domain;
	set<string> m_site;

private:

	pthread_mutex_t m_cate_mutex;
	map<string, int> m_domain_cate_done;//domain对应已抓取cate数
	
	pthread_mutex_t m_item_mutex;
	map<string, int> m_domain_item_done;//domain对应已抓取item数

	map<string, int> m_domain_cate_select;//domain对应待选取cate数
	
	map<string, int> m_domain_item_select;//domain对应待选取item数

	int m_CPQ_url_num;//CPQ中url数
	int m_IPQ_url_num;//IPQ中url数
	int m_COQ_url_num;//COQ中url数
	int m_IOQ_url_num;//IOQ中url数
	int m_SQ_url_num; //SQ中url数

	FILE* m_fp;
};




#endif
