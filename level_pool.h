#ifndef _FANFAN_LEVEL_POOL_H__
#define _FANFAN_LEVEL_POOL_H__

#include <pthread.h>
#include <string>
#include <map>

using namespace std;

class CSpiderConf;

struct SLevelInfo
{
	int max_crawl_thread_count;
	int crawl_thread_count;

	time_t last_crawl_time;
	time_t crawl_interval;
};

class CLevelPool
{
public:
    CLevelPool(void) {}
	~CLevelPool(void) {}
    int init(CSpiderConf* conf);
	int destroy(void);
    void insert_level_info(string& site, SLevelInfo li);
	bool is_crawl_enabled(string& site);
	void finish_crawl(string& site, bool change_time=true);

	//for debug
	void print(void);

private:
    time_t get_time_now(void);

    CSpiderConf* mp_conf;
    pthread_mutex_t m_mutex;
	map<string, SLevelInfo> m_pool;

	CLevelPool(const CLevelPool& lp);
	CLevelPool& operator=(const CLevelPool& lp);
};

#endif

