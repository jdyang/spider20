#ifndef __FANFAN_UTF8_CONVERTER_H__
#define __FANFAN_UTF8_CONVERTER_H__

#include <string>
#include "code_recognizer.h"
#include "code_converter.h"

#define CONVERTER_MAX_CONTENT_LEN (1024*1024*2)
using namespace std;

class UTF8Converter
{
public:
    UTF8Converter(void);
	~UTF8Converter(void);
	// 载入识别编码的字典
    int init(const char* code_path);
	void set_input(const char* content, int len);
	CodeType detect_code_type(void);
	CodeType get_code_type(void);
	const char* get_code_type_str(void);
	int to_utf8(void);
	string& get_converted_content(void);

private:
    CodeRecognizer* mp_code_recognizer;
	CodeConverter* mp_gb2utf8_converter;
	CodeConverter* mp_big2utf8_converter;

    CodeType m_code_type;
    string* mp_html_content;
	string m_converted_content;

	char m_output[CONVERTER_MAX_CONTENT_LEN];
	const char* m_input;
	int m_input_len;

};

#endif
