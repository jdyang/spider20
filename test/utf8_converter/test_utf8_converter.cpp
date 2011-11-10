#include "../../utf8_converter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("program file-name\n");
		return -1;
	}

    FILE* fp = NULL;

	if (!(fp=fopen(argv[1], "r")))
	{
		printf("open file error\n");
		return 0;
	}

    fseek(fp, 0, SEEK_END);
	int file_len = ftell(fp);
	cout << "file_len: " << file_len << endl;
	char* p_file = NULL;
    p_file = (char*)malloc(file_len+1);
	if (!p_file)
	{
		printf("file malloc error\n");
		return -1;
	}

    memset(p_file, 0, file_len+1);
	fseek(fp, 0, SEEK_SET);
	if (file_len == fread(p_file, file_len, 1, fp))
	{
		printf("file read error\n");
		return -1;
	}

	UTF8Converter uc;

	if (-1 == uc.init("../../conf/ec/code/"))
	{
		printf("utf8_converter");
		return -1;
	}

    uc.set_input(p_file, strlen(p_file));
	uc.detect_code_type();
	printf("original content\n");
	printf("code type: %s\n", uc.get_code_type_str());

	uc.to_utf8();
    string& ct = uc.get_converted_content();
	uc.set_input(ct.c_str(), ct.length());
	uc.detect_code_type();
	printf("converted content\n");
	printf("code type: %s\n", uc.get_code_type_str());

    return 0;
}
