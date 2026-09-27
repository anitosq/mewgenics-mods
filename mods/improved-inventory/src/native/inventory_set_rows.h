/* Derived menu state, separated from rendering for regression/performance checks. */
#include "inventory_set_cache.h"
#define IQ_SET_PAGE 7
static IQSetCache iq_set_cache;
static int iq_set_rows[IQ_SET_LIMIT],iq_set_rows_count;
static void iq_build_set_rows(void) {
    iq_build_ownership();
    if(!iq_set_cache_matches(&iq_set_cache)) {
        iq_set_rows_count=0;
        for(int j=0;j<iq_set_count;j++) {
            if(!iq_query_match(iq_sets[j].name,iq_set_query))continue;
            int at=iq_set_rows_count++;
            while(at>0 && _wcsicmp(iq_sets[iq_set_rows[at-1]].name,iq_sets[j].name)>0){iq_set_rows[at]=iq_set_rows[at-1];at--;}
            iq_set_rows[at]=j;
        }
        iq_set_cache_store(&iq_set_cache);
    }
    int max=iq_set_rows_count>IQ_SET_PAGE?iq_set_rows_count-IQ_SET_PAGE:0;
    if(iq_set_offset>max)iq_set_offset=max;if(iq_set_offset<0)iq_set_offset=0;
}
