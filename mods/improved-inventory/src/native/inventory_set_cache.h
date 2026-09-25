/* Cache only derived set-menu data. Selection and paging do not affect counts. */
typedef struct {
    uint64_t view_revision, metadata_revision;
    int valid, type, rarity;
    wchar_t query[IQ_QUERY_LIMIT], set_query[IQ_QUERY_LIMIT];
} IQSetCache;

static int iq_set_cache_matches(const IQSetCache* cache, uint64_t view_revision,
    uint64_t metadata_revision, IQFilter filter, const wchar_t* query,
    const wchar_t* set_query) {
    return cache->valid && cache->view_revision==view_revision &&
        cache->metadata_revision==metadata_revision && cache->type==filter.type &&
        cache->rarity==filter.rarity && !wcscmp(cache->query,query) &&
        !wcscmp(cache->set_query,set_query);
}

static void iq_set_cache_store(IQSetCache* cache, uint64_t view_revision,
    uint64_t metadata_revision, IQFilter filter, const wchar_t* query,
    const wchar_t* set_query) {
    cache->view_revision=view_revision;cache->metadata_revision=metadata_revision;
    cache->type=filter.type;cache->rarity=filter.rarity;
    wcsncpy(cache->query,query,IQ_QUERY_LIMIT-1);cache->query[IQ_QUERY_LIMIT-1]=0;
    wcsncpy(cache->set_query,set_query,IQ_QUERY_LIMIT-1);cache->set_query[IQ_QUERY_LIMIT-1]=0;
    cache->valid=1;
}
