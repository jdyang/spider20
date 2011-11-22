#ifndef _FANFAN_URLPOOL_H_
#define _FANFAN_URLPOOL_H_

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <iostream>

#include "sse_common.h"
#include "ncarch.h"
#include "ncenums.h"
#include "ncconsts.h"
#include "ncmutex.h"
#include "ncsocket.h"
#include "ncpack.h"
#include "util.h"
#include "uc_url.h"

#include "spider_common.h"

using namespace std;


class UrlInfo {
public:
    string url;
    unsigned long last_crawl_time;
	string domain;
	string site;
	int type; // 0 for oitem, 1for ocate, 2for pitem, 3for pcate
};

class CUrlPool {
public:
    CSpiderConf& get_conf();
	FuncRet load_urls(const char *path);

	void set_conf(CSpiderConf *conf);
    deque<UrlInfo>& get_url_queue();
	set<string>& get_url_set();
	ncMutex get_url_mutex(){return m_url_mutex;}
	ncMutex get_url_set_mutex(){return m_url_set_mutex;}

	bool url_empty();
	void clear_urls();
	void push_url(UrlInfo ui);
	UrlInfo pop_url();
	int print_pool();
	
	void add_to_que(UrlInfo ui);
	
	
private:
    deque<UrlInfo> m_url_queue;
    ncMutex m_url_mutex;

	set<string> m_url_set;
    ncMutex m_url_set_mutex;

	CSpiderConf *m_conf;
};

#endif
