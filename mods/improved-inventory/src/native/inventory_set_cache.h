/* Cache alphabetical menu rows separately from full owned counts. */
typedef struct {
    uint64_t view_revision,metadata_revision;
    int valid,multiple_pieces;
    wchar_t set_query[IQ_QUERY_LIMIT];
} IQSetCache;
static int iq_set_cache_matches(const IQSetCache* c) {
    return c->valid && c->view_revision==iq_view_revision &&
        c->metadata_revision==iq_metadata_revision &&
        c->multiple_pieces==iq_search.multiple_pieces && !wcscmp(c->set_query,iq_set_query);
}
static void iq_set_cache_store(IQSetCache* c) {
    c->view_revision=iq_view_revision;c->metadata_revision=iq_metadata_revision;
    c->multiple_pieces=iq_search.multiple_pieces;
    wcsncpy(c->set_query,iq_set_query,IQ_QUERY_LIMIT-1);c->set_query[IQ_QUERY_LIMIT-1]=0;
    c->valid=1;
}
