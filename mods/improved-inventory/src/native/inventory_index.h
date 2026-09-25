/* Presentation-only pointer lookup. Callers must still validate native lifetime
 * and identity before using a match. Rebuild when the native grid changes. */
#define IQ_INDEX_CAPACITY 2048
typedef struct { const void* key; int value; } IQIndexEntry;
typedef struct { IQIndexEntry entries[IQ_INDEX_CAPACITY]; } IQIndex;
static size_t iq_index_slot(const void* key) {
    uint64_t bits=(uint64_t)(uintptr_t)key;
    bits^=bits>>33;bits*=UINT64_C(0xff51afd7ed558ccd);bits^=bits>>33;
    return (size_t)bits&(IQ_INDEX_CAPACITY-1);
}
static void iq_index_clear(IQIndex* index) {memset(index,0,sizeof(*index));}
static int iq_index_put(IQIndex* index,const void* key,int value) {
    if(!key)return 0;
    size_t at=iq_index_slot(key);
    for(size_t n=0;n<IQ_INDEX_CAPACITY;n++,at=(at+1)&(IQ_INDEX_CAPACITY-1)) {
        IQIndexEntry* entry=&index->entries[at];
        if(!entry->key || entry->key==key){entry->key=key;entry->value=value;return 1;}
    }
    return 0;
}
static int iq_index_get(const IQIndex* index,const void* key) {
    if(!key)return -1;
    size_t at=iq_index_slot(key);
    for(size_t n=0;n<IQ_INDEX_CAPACITY;n++,at=(at+1)&(IQ_INDEX_CAPACITY-1)) {
        const IQIndexEntry* entry=&index->entries[at];
        if(!entry->key)return -1;
        if(entry->key==key)return entry->value;
    }
    return -1;
}
