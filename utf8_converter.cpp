#include "utf8_converter.h"

static char* CODE_TYPE_STR[] = {
    "UNKNOWN",
    "GBK",
	"GB",
	"UTF8",
	"BIG5",
	"ENGLISH",
	"DEFAULT"
};

UTF8Converter::UTF8Converter(void)
{
	mp_code_recognizer = NULL;
	mp_gb2utf8_converter = NULL;
	mp_big2utf8_converter = NULL;
	m_output = NULL;
}

UTF8Converter::~UTF8Converter(void)
{
	if (mp_code_recognizer)
	{
		delete mp_code_recognizer;
		mp_code_recognizer = NULL;
	}
	if (mp_gb2utf8_converter)
	{
		delete mp_gb2utf8_converter;
		mp_gb2utf8_converter = NULL;
	}
	if (mp_big2utf8_converter)
	{
		delete mp_big2utf8_converter;
		mp_big2utf8_converter = NULL;
	}

	if (m_output) {
		free(m_output);
		m_output = NULL;
	}
}

int UTF8Converter::init(const char* code_path)
{
    mp_code_recognizer = new CodeRecognizer();
	if (!mp_code_recognizer)
	{
		printf("mp_code_recognizer malloc error.");
		abort();
	}

    mp_gb2utf8_converter = new CodeConverter();
	if (!mp_gb2utf8_converter)
	{
		printf("mp_gb2utf8_converter malloc error.");
		abort();
	}

    mp_big2utf8_converter = new CodeConverter();
	if (!mp_big2utf8_converter)
	{
		printf("mp_big2utf8_converter malloc error.");
		abort();
	}

	if (!mp_code_recognizer->Initialize(const_cast<char*>(code_path)))
	{
		printf("code recognizer initial error");
		return -1;
	}

	if (!mp_gb2utf8_converter->initialize("utf-8", "gbk"))
	{
		printf("gb2uf8 converter initial error");
		return -1;
	}

	if (!mp_big2utf8_converter->initialize("utf-8", "big5"))
	{
		printf("gb2uf8 converter initial error");
		return -1;
	}

	m_output = malloc(CONVERTER_MAX_CONTENT_LEN);
	if (!m_output)
	{
		printf("m_output malloc err");
		return -1;
	}

    return 0;
}

void UTF8Converter::set_input(const char* content, int len)
{
	m_input = content;
	m_input_len = len;

}

CodeType UTF8Converter::detect_code_type(void)
{
	m_code_type = mp_code_recognizer->DetectCode(m_input, m_input_len);
	return m_code_type;
}


CodeType UTF8Converter::get_code_type(void)
{
	return m_code_type;
}

const char* UTF8Converter::get_code_type_str(void)
{
	return CODE_TYPE_STR[m_code_type];
}

int UTF8Converter::to_utf8(void)
{
	m_converted_content = "";
	int real_len = -1;

	memset(m_output, 0, CONVERTER_MAX_CONTENT_LEN);
	CodeType code_type = detect_code_type();
	switch (code_type)
	{
		case EN_GB:
		    real_len = mp_gb2utf8_converter->convert((char*)m_input, m_input_len, m_output, CONVERTER_MAX_CONTENT_LEN-1);
			if (-1 == real_len)
			{
				return -1;
			}
			m_converted_content = m_output;
			return 0;
		case EN_BIG5:
		    real_len = mp_big2utf8_converter->convert((char*)m_input, m_input_len, m_output, CONVERTER_MAX_CONTENT_LEN-1);
			if (-1 == real_len )
			{
				return -1;
			}
			m_converted_content = m_output;
			return 0;
		default:
		    m_converted_content = m_input;
			return 0;
	}
	return 0;
}

string& UTF8Converter::get_converted_content(void)
{
	return m_converted_content;
}
