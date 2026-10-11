/* Index follows ring-slot replacement; native identity is checked at lookup. */
typedef struct {void* drawer;uint64_t id,generation;IQItemTraits traits; wchar_t text[4096]; uint64_t sets[4]; int sets_known;} IQMetadata;
static IQMetadata iq_metadata[IQ_MAX_DRAWERS];
static unsigned iq_metadata_next;
static IQIndex iq_metadata_index;
static IQMetadata* iq_metadata_slot(void* drawer) {
    int at=iq_index_get(&iq_metadata_index,drawer);
    if(at<0) {
        at=(int)(iq_metadata_next++%IQ_MAX_DRAWERS);
        iq_index_remove(&iq_metadata_index,iq_metadata[at].drawer);
        iq_index_put(&iq_metadata_index,drawer,at);
    }
    return &iq_metadata[at];
}
static IQMetadata* iq_find_metadata(void* drawer,uint64_t id) {
    int at=iq_index_get(&iq_metadata_index,drawer);
    if(at<0 || at>=IQ_MAX_DRAWERS)return NULL;
    IQMetadata* m=&iq_metadata[at];
    return m->drawer==drawer && m->id==id && m->generation==iq_generation(drawer)?m:NULL;
}
