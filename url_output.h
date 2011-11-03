#ifndef __FANFAN_URL_OUTPUT_H__
#define __FANFAN_URL_OUTPUT_H__

#include <string>
#include <pthread.h>

using namespace std;

class CSpiderConf;

class CUrlOutput
{
public:
    CUrlOutput(void){}
	~CUrlOutput(void){}
	int init(const char* file_path);
	int destroy(void);
	int append(string& url);
	int append_error(string& url, const char* err_str);


private:
    int m_fd;
	pthread_mutex_t m_mutex;
	char m_buf[5120];


	CUrlOutput(const CUrlOutput&);
	CUrlOutput& operator=(const CUrlOutput&);
};

#endif
