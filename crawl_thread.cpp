void* crawl_thread(void* arg)
{
	CSpider* psp = (CSpider*)arg;

	CSelectedQueue& sq = &(psp->m_sq);
	CPageOutput& po = &(psp->m_page_output);

	SSQItem qi;

	while (!round_end)
	{
        if (!sq.pop(qi))
		{
            
		}
	}
}
