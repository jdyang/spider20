#ifndef _URLPOOL_H_
#define _URLPOOL_H_

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

using namespace std;


class UrlInfo {
public:
    string url;
    unsigned long last_crawl_time;
	string domain;
	string site;
};

class UrlPool {
public:
    CSpiderConf& get_conf();
	FuncRet load_urls(const char *path);

	void set_conf(CSpiderConf *conf);
    deque<UrlInfo>& get_url_queue();
	set<string>& get_url_set();
	ncMutex get_url_mutex(){return m_url_mutex;}

	bool url_empty();
	void push_url(UrlInfo ui);
	UrlInfo pop_url();
	
	
private:
    deque<UrlInfo> m_url_queue;
    ncMutex m_url_mutex;

	set<string> m_url_set;
    ncMutex m_url_set_mutex;

	CSpiderConf *m_conf;
};

#endif
