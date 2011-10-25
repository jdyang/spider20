#include "../../url_output.h"

int main(int argc, char** argv)
{
	CUrlOutput uo;
    string url;

	if (uo.init("item.list") != 0)
	{
		printf("init error\n");
		return 0;
	}

    url = "http://item.taobao.com/id=234";
	printf("%s\n", url.c_str());
	uo.append(url);

	url = "http://product.taobao.com/product.aspx?product_id=23434";
	uo.append(url);

	url = "http://www.360buy.com/product/2243DSF234";
	uo.append(url);

	if (uo.destroy() != 0)
	{
		printf("destroy error\n");
		return 0;
	}

	return 0;
}
