#ifndef __FANFAN_URL_OUTPUT_H__
#define __FANFAN_URL_OUTPUT_H__

#include <string>
#include <pthread.h>

using namespace std;

class CUrlOutput
{
public:
    CUrlOutput(void){}
	~CUrlOutput(void){}
	int init(string dir);
	int destroy(void);
	int append(string& url);


private:
	string m_base_dir;
	pthread_mutex_t m_mutex;
	char m_buf[5120];


	CUrlOutput(const CUrlOutput&);
	CUrlOutput& operator=(const CUrlOutput&);
};

#endif
