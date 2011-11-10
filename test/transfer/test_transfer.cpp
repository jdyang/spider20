#include "spider_common.h"
#include "spider.h"

int main(void)
{
	CSpider sp;

	if (sp.load_conf("../../conf/spider.conf") == -1)
	{
		cout << "load conf error\n";
		return -1;
	}

	sp.transfer();

	return 0;
}
