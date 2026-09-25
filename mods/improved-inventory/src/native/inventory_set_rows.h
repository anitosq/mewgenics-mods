/* Derived menu state, separated from rendering for regression/performance checks. */
#include "inventory_set_cache.h"
static IQSetCache iq_set_cache;
static int iq_set_rows[IQ_SET_LIMIT+3],iq_set_rows_count,iq_set_counts[IQ_SET_LIMIT+3];
static void iq_build_set_rows(void) {
    if(!iq_set_cache_matches(&iq_set_cache,iq_view_revision,iq_metadata_revision,
        active_filter,iq_search.query,iq_set_query)) {
        memset(iq_set_counts,0,sizeof(iq_set_counts));
        for(int i=0;i<iq.count;i++) {
            IQDrawer* d=&iq.drawers[i];IQMetadata* m=iq_find_metadata(d->drawer,d->item_id);
            if(!m || !iq_filter_match(active_filter,d->traits,d->rarity) || !iq_query_match(m->text,iq_search.query))continue;
            iq_set_counts[0]++;
            if(!m->sets_known)continue;
            int has=0;
            for(int j=0;j<iq_set_count;j++)if(m->sets[j/64]&(UINT64_C(1)<<(j%64))) {has=1;iq_set_counts[j+3]++;}
            iq_set_counts[has?1:2]++;
        }
        iq_set_rows_count=0;
        for(int j=0;j<iq_set_count;j++)if(iq_query_match(iq_sets[j].name,iq_set_query)) {
            int at=iq_set_rows_count++;
            while(at>0 && _wcsicmp(iq_sets[iq_set_rows[at-1]].name,iq_sets[j].name)>0){iq_set_rows[at]=iq_set_rows[at-1];at--;}
            iq_set_rows[at]=j;
        }
        iq_set_cache_store(&iq_set_cache,iq_view_revision,iq_metadata_revision,
            active_filter,iq_search.query,iq_set_query);
    }
    /* Paging still clamps on every update, even when contents are cached. */
    int max=iq_set_rows_count>9?iq_set_rows_count-9:0;
    if(iq_set_offset>max)iq_set_offset=max;if(iq_set_offset<0)iq_set_offset=0;
}
