#ifndef __FANFAN_PAGE_OUTPUT_H__
#define __FANFAN_PAGE_OUTPUT_H__

#include <string>
#include <pthread.h>

using namespace std;

class CPageOutput
{
public:
    CPageOutput(string dir);
	~CPageOutput(void) { if (m_fd != -1) { close(m_fd); } }
	int append(const char* buf, int len, bool need_write=true);

private:
    int m_fd;
	int m_cur_day;
	int m_cur_min; 
	string m_base_dir;

	pthread_mutex_t m_mutex;
};

#endif
