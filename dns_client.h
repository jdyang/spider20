#ifndef __FANFAN_DNS_CLIENT_H__
#define __FANFAN_DNS_CLIENT_H__

#include "udpdns.h"
#include "map"
#include "vector"
#include <set>
#include <string>
#include <iostream>

#include "ncarch.h"
#include "ncenums.h"
#include "ncconsts.h"
#include "ncmutex.h"
#include "ncsocket.h"
#include "ncpack.h"
#include "ncthread.h"
#include "spider_common.h"

using namespace std;

class CDnsClient {
public:
	int init(CSpiderConf *conf); // 0 for ok ; -1 for error
	int query_site_ip(set<string> *sites);
	
    CSpiderConf* get_conf();
	void set_conf(CSpiderConf *conf);
    
 	map<string, string>& get_ip_map();
	
	int clear_map();
	void put_ip(string site, string ip);
	string get_ip(const char* site); //if the site haven't been queried yet, it will update the ip map(adding a new record)

private:
	string query_real_dns(const char* site);
	
    map<string, string> m_ip_list;
    ncMutex m_ip_mutex;

	ncMutex m_singleton_mutex;

	CUdpDns udp_dns;

	CSpiderConf *m_conf;
};

#endif

