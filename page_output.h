#ifndef __FANFAN_PAGE_OUTPUT_H__
#define __FANFAN_PAGE_OUTPUT_H__

#include "spider.h"
#include <string>
#include <pthread.h>

using namespace std;

class CSpider;

class CPageOutput
{
public:
    CPageOutput(void){}
	~CPageOutput(void){}

	int init(string dir);
	int destroy(void);

	void set_spider(CSpider* sp);

	int append(const char* buf, int len, bool need_write=true);


private:
    CSpider* mp_spider;

    int m_fd;
	int m_cur_day;
	int m_cur_min; 

	string m_base_dir;

	pthread_mutex_t m_mutex;

	CPageOutput(const CPageOutput&);
	CPageOutput& operator=(const CPageOutput&);
};

#endif
