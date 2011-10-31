#ifndef __FANFAN_PAGE_OUTPUT_H__
#define __FANFAN_PAGE_OUTPUT_H__

#include "spider.h"
#include <string>
#include <pthread.h>

using namespace std;

class CSpiderConf;

class CPageOutput
{
public:
    CPageOutput(void){}
	~CPageOutput(void){}

	int init(CSpiderConf* p_conf);
	int destroy(void);

	int append(const char* buf, int len, bool need_write=true);


private:
    CSpiderConf* mp_conf;

    int m_fd;
	int m_cur_day;
	int m_cur_min; 

	pthread_mutex_t m_mutex;

	CPageOutput(const CPageOutput&);
	CPageOutput& operator=(const CPageOutput&);
};

#endif
