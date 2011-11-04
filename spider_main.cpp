#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <cstdio>
#include <iterator>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>
#include <errno.h>
#include <signal.h>

#include <string>
#include <cstdio>
#include <iterator>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <map>
#include <list>

#include "spider.h"
#include "sdlog.h"

using namespace std;

static void SigTerm(int x)
{
	cerr << "Terminated in main!" << endl;
	exit(0);
}

int main(int argc, char **argv)
{
	CSpider sp;
	
	signal(SIGTERM, SigTerm);
	signal(SIGKILL, SigTerm);
	signal(SIGINT, SigTerm);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD,SIG_IGN);

	if (argc < 2) {
		cout << "Usage: " << argv[0] <<" [config_file]" << endl;
		return 0;
	}
	
	if (-1 == sp.load_conf(argv[1])) {
		cerr << "spider_main load conf error" << endl;
		return -1;
	}
	
	SDLOG_INIT(sp.m_spider_conf.log_conf_path);
	
	struct rlimit rlim;
	getrlimit(RLIMIT_STACK,&rlim);
	rlim.rlim_cur=20*1024*1024;
	if(setrlimit(RLIMIT_STACK,&rlim)<0){
		SDLOG_FATAL(SP_WFNAME,"Set stack size failed.");
		return -1;
	}
	
/*	if (getrlimit(RLIMIT_CORE, &rlim)==0) {
		rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	}
	if (setrlimit(RLIMIT_CORE, &rlim)!=0) {
		SDLOG_FATAL(SP_WFNAME,"Set coredump size failed.");
		return -1;
	}
*/
	
	if (!sp.start()) {
		cerr << "spider start error!" << endl;
		return -1;
	}
	return 0;
}
