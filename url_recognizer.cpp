#include "url_recognizer.h"

void CUrlRecognizer::get_one_line(FILE* fp, char* line, int len, int& lineno) 
{
	lineno++;
	memset(line,0,len);
	if(feof(fp)) return;

	fgets(line,len,fp);
	char* pch = strchr(line,'\r');
	if(pch != NULL) *pch = 0;
	pch = strchr(line,'\n');
	if(pch != NULL) *pch = 0;
}

char* CUrlRecognizer::filter_headtail_blank(char* buf, int len) 
{
	char *p = buf;
	char *q = buf + len -1;
	while (true) 
	{
		if (*p != ' ' && *p != '\t' && *p != '\n') break;
		*p++ = '\0';
	}

	while (true) 
	{
		if (*q != ' ' && *q != '\t' && *q != '\n' && *q != '\0') break;
		*q-- = '\0';
	}
	return p;
}

FuncRet CUrlRecognizer::load_conf(const char* conf_path) 
{
    FILE* fp = fopen(conf_path, "r");
	if (!fp) 
	{
		cout << "open file err	:	" << conf_path << endl;
		return FR_FALSE;
	}

	char line[1024];
	int lineno = 0;
	char* p = NULL;
	char* ptr = NULL;
	int len = 0;
	string domain;
	string name;
	boost::regex url_regex;
	string repstr;

	while (!feof(fp)) 
	{
        get_one_line(fp, line, 1024, lineno);
	    p = filter_headtail_blank(line);
	    len = strlen(p);
    	if (p[0] == '\0' || p[0] == '#') 
		{
	        continue;
	    } 
		else if (p[0] == '[') 
		{
	        if (p[len-1] != ']') 
			{
				cout << "no ]	:	" << lineno  << endl;
			    return FR_FALSE;
		    }
		    p[len-1] = '\0';
		    domain = p+1;
			map<string, CDomainRules>::iterator it = m_rules.find(domain);
			if (m_rules.end() == it) 
			{
				m_rules.insert(make_pair(domain, CDomainRules()));
			}
		} 

		//cout << "domain:" << domain << endl;
		//cout << "lineno:" << lineno << endl;
        get_one_line(fp, line, 1024, lineno);
	    p = filter_headtail_blank(line);
	    len = strlen(p);
		if (strncmp(p, "<item>", 6) != 0) 
		{
			cout << "<item> error" << endl;
			return FR_FALSE;
		}
		else
		{
			get_one_line(fp, line, 1024, lineno);
			p = filter_headtail_blank(line);
			len = strlen(p);
		}
		while(strncmp(p, "<normalize>", 11) != 0) 
		{
			ptr = p;
			for (; ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != 0; ptr++) ;
			*ptr = '\0';
			url_regex.assign(p);
			repstr = "";
			name = "item";
			//cout << "item:" << p << endl;
			(((m_rules[domain]).rules_of_domain)[name]).rules.push_back(make_pair(url_regex, repstr));

			get_one_line(fp, line, 1024, lineno);
			p = filter_headtail_blank(line);
			len = strlen(p);
		}

		if(strncmp(p, "<normalize>", 11) != 0)
		{
			cout << "<normalize> error" << endl;
			return FR_FALSE;
		}
		else
		{
			get_one_line(fp, line, 1024, lineno);
			p = filter_headtail_blank(line);
			len = strlen(p);
		}
		while(strncmp(p, "<category>", 10) != 0)
		{
			ptr = p;
			for (; ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != 0; ptr++) ;
			*ptr = '\0';
			url_regex.assign(p);
			ptr++;
			if((ptr-p) < (len-1))
			{
				for (; ptr && (*ptr == ' ' || *ptr == '\t'); ptr++) ;
				repstr = ptr;
			}
			else
				repstr = "";
			name = "normalize";
			//cout << "normalize:" << p << ":" << repstr << endl;
			(((m_rules[domain]).rules_of_domain)[name]).rules.push_back(make_pair(url_regex, repstr));

			get_one_line(fp, line, 1024, lineno);
			p = filter_headtail_blank(line);
			len = strlen(p);
		}

		if(strncmp(p, "<category>", 11) != 0)
		{
			cout << "<category> error" << endl;
			return FR_FALSE;
		}
		else
		{
			get_one_line(fp, line, 1024, lineno);
			p = filter_headtail_blank(line);
			len = strlen(p);
		}
		while(p[0] !=  0)
		{
			ptr = p;
			for (; ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' && *ptr != 0; ptr++) ;
			*ptr = '\0';
			url_regex.assign(p);
			repstr = "";
			name = "category";
			//cout << "category:" << p << endl;
			(((m_rules[domain]).rules_of_domain)[name]).rules.push_back(make_pair(url_regex, repstr));

			get_one_line(fp, line, 1024, lineno);
			p = filter_headtail_blank(line);
			len = strlen(p);
		}
	}
	if (fp) 
	{
	    fclose(fp);
		fp = NULL;
	}
	return FR_OK;
}

void CUrlRecognizer::print_conf(void) 
{
    map<string, CDomainRules>::iterator it =  m_rules.begin();
	for (; it != m_rules.end(); ++it) 
	{
		cout << "[" << it->first << "]" << endl;
		CDomainRules& rv = it->second;
		map<string, RuleVector>::iterator vit = rv.rules_of_domain.begin();
		for (; vit != rv.rules_of_domain.end(); vit++) 
		{
			cout << "<" << vit->first << ">" << endl;
			vector< pair<boost::regex, string> >::iterator rit = vit->second.rules.begin();
			for (; rit != vit->second.rules.end(); rit++) 
			{
                cout << (rit->first).str() << "\t\t" << rit->second << endl;
			}
		}
	}
}

int CUrlRecognizer::get_type(string url) 
{
	int is_item = 0;
	ucUrl url_(url);
	url_.build();
	string domain = url_.get_domain();
	map<string, CDomainRules>::iterator iter_domain = m_rules.find(domain);
	if(iter_domain == m_rules.end())
	{
		return -1;
	}
	else
	{
		map<string, RuleVector>::iterator iter_rule = iter_domain->second.rules_of_domain.find("item");
		if(iter_rule == iter_domain->second.rules_of_domain.end())
		{
		}
		else
		{
			vector<pair<boost::regex, string> >::iterator iter_regex = iter_rule->second.rules.begin();
			for(; iter_regex != iter_rule->second.rules.end(); ++iter_regex)
			{
				if(boost::regex_search(url, iter_regex->first))
				{
					is_item = 1;
					return is_item;
				}
			}
		}

		iter_rule = iter_domain->second.rules_of_domain.find("category");
		if(iter_rule == iter_domain->second.rules_of_domain.end())
		{
		}
		else
		{
			vector<pair<boost::regex, string> >::iterator iter_regex = iter_rule->second.rules.begin();
			for(; iter_regex != iter_rule->second.rules.end(); ++iter_regex)
			{
				if(boost::regex_search(url, iter_regex->first))
				{
					is_item = 2;
					return is_item;
				}
			}
		}
	}
	return is_item;
}

string CUrlRecognizer::normalize(string url) 
{
	string url_do(url);
	ucUrl url_(url);
	url_.build();
	string domain = url_.get_domain();
	map<string, CDomainRules>::iterator iter_domain = m_rules.find(domain);
	if(iter_domain == m_rules.end())
	{
		return "";
	}
	else
	{
		map<string, RuleVector>::iterator iter_rule = iter_domain->second.rules_of_domain.find("normalize");
		if(iter_rule == iter_domain->second.rules_of_domain.end())
		{
			return "";
		}
		else
		{
			vector<pair<boost::regex, string> >::iterator iter_regex = iter_rule->second.rules.begin();
			for(; iter_regex != iter_rule->second.rules.end(); ++iter_regex)
			{
				url_do = boost::regex_replace(url_do, iter_regex->first, iter_regex->second);
			}
		}
	}
	return url_do;
}
