#include "../../url_recognizer.h"
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	CUrlRecognizer url_recog;

	if (url_recog.load_conf("../../conf/strong_rule.conf") != FR_OK) {
		cout << "load_conf error" << endl;
	}

    //cout << "load success" << endl;
	//url_recog.print_conf();

	FILE *fp = fopen(argv[1], "r");
	if(fp == NULL)
	{
		cout << "open file error : " << argv[1] << endl;
		return 0;
	}

	char line[1024];
	while(!feof(fp))
	{
		memset(line, 0, 1024);
		fgets(line, 1024, fp);
		char *ptr = strchr(line, '\n');
		if(ptr != NULL)*ptr=0;
		ptr = strchr(line, '\r');
		if(ptr != NULL)*ptr=0;
		int type = url_recog.get_type(line);
		cout << "url:" << line << " " << "type:" << type << endl;
		cout << "normalize:" << url_recog.normalize(line) << endl;
		cout << endl;
	}

	if(fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}

	return 0;
}
