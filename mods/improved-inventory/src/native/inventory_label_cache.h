/* Content only: panel motion and hover feedback still update every frame. */
typedef struct {
    int valid,equipment,items[2],set_count,set_offset,popup,focus,cursor,select_all;
    uint64_t view_revision,metadata_revision;
    IQFilter filter;
    IQSearch search;
    wchar_t set_query[IQ_QUERY_LIMIT];
} IQLabelCache;
static IQLabelCache iq_label_cache;
static int iq_labels_match(void) {
    const IQLabelCache* c=&iq_label_cache;
    return c->valid && c->view_revision==iq_view_revision &&
        c->metadata_revision==iq_metadata_revision && c->equipment==iq.equipment &&
        c->items[0]==iq.items[0] && c->items[1]==iq.items[1] &&
        c->set_count==iq_set_count && c->set_offset==iq_set_offset &&
        c->popup==filter_popup && c->focus==iq_focus && c->cursor==iq_cursor &&
        c->select_all==iq_select_all && c->filter.type==active_filter.type &&
        c->filter.rarity==active_filter.rarity && c->search.set_mode==iq_search.set_mode &&
        !wcscmp(c->search.query,iq_search.query) && !wcscmp(c->set_query,iq_set_query) &&
        !memcmp(c->search.sets,iq_search.sets,sizeof(c->search.sets));
}
static void iq_labels_store(void) {
    IQLabelCache* c=&iq_label_cache;
    c->view_revision=iq_view_revision;c->metadata_revision=iq_metadata_revision;
    c->equipment=iq.equipment;memcpy(c->items,iq.items,sizeof(c->items));
    c->set_count=iq_set_count;c->set_offset=iq_set_offset;c->popup=filter_popup;
    c->focus=iq_focus;c->cursor=iq_cursor;c->select_all=iq_select_all;
    c->filter=active_filter;c->search=iq_search;
    memcpy(c->set_query,iq_set_query,sizeof(c->set_query));c->valid=1;
}
