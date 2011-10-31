#ifndef _URLPOOL_H_
#define _URLPOOL_H_

#include "udpdns.h"
#include "map"
#include "vector"
#include <set>
#include <string>
#include "spider.h"
#include <iostream>

#include "ncarch.h"
#include "ncenums.h"
#include "ncconsts.h"
#include "ncmutex.h"
#include "ncsocket.h"
#include "ncpack.h"
#include "ncthread.h"

class DnsClient {
public:
	int init(CSpiderConf *conf); // 0 for ok ; -1 for error
	int query_site_ip(set<string> *sites);
	
    CSpiderConf* get_conf();
	void set_conf(CSpiderConf *conf);
    
 	map<string, string>& get_ip_map();

	bool url_empty();
	void put_ip(string site, string ip);
	string get_ip(string site);

private:
	string query_real_dns(string site);
	
    map<string, string> m_ip_list;
    ncMutex m_ip_mutex;

	CUdpDns udp_dns;

	CSpiderConf *m_conf;
};

#endif

