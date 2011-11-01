#ifndef __FANFAN_INCLUDE_URL_RECOGNIZER_H__
#define __FANFAN_INCLUDE_URL_RECOGNIZER_H__

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/regex.hpp>
#include <cstdlib>
#include "sse_common.h"
#include "uc_url.h"

using namespace std;

struct RuleVector 
{
	vector<pair<boost::regex, string> > rules;
};

struct CDomainRules 
{
	map<string, RuleVector> rules_of_domain;
};

class CUrlRecognizer 
{
public:
    FuncRet load_conf(const char* conf);
    void print_conf(void);
    int get_type(string url);	
	string normalize(string url);
private:
    void get_one_line(FILE* fp, char* line, int len, int& lineno);
    char* filter_headtail_blank(char* buf, int len=1024);

    map<string, CDomainRules> m_rules;
};

#endif
