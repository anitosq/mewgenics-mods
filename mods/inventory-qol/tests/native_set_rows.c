#include <assert.h>
#ifdef NDEBUG
#error Compile this regression test with assertions enabled (use -UNDEBUG).
#endif
#include <stdio.h>
#include <string.h>
#include "../src/native/inventory_filter.h"
#include "../src/native/inventory_search.h"

/* Fixture adapters replace game memory; the menu builder itself is production code. */
typedef struct {void* drawer;uint64_t item_id;IQItemTraits traits;int rarity;} IQDrawer;
typedef struct {wchar_t text[128];uint64_t sets[4];int sets_known;} IQMetadata;
static struct {int count;IQDrawer drawers[1024];} iq;
static IQMetadata metadata[1024];
static struct {wchar_t name[160];} iq_sets[IQ_SET_LIMIT];
static int iq_set_count,iq_set_offset,lookups;
static uint64_t iq_view_revision,iq_metadata_revision;
static IQFilter active_filter;
static IQSearch iq_search;
static wchar_t iq_set_query[IQ_QUERY_LIMIT];
static IQMetadata* iq_find_metadata(void* drawer,uint64_t id) {
    (void)drawer;lookups++;return id<1024?&metadata[id]:NULL;
}
#include "../src/native/inventory_set_rows.h"

int main(void) {
    iq.count=1024;iq_set_count=12;
    for(int s=0;s<12;s++)swprintf(iq_sets[s].name,160,L"Set %02d",11-s);
    for(int i=0;i<iq.count;i++) {
        iq.drawers[i]=(IQDrawer){.item_id=(uint64_t)i,.traits={i%2,0,0,0},.rarity=i%2?5:1};
        wcscpy(metadata[i].text,i%2?L"Rare potion":L"Common hat");
        metadata[i].sets_known=1;metadata[i].sets[0]=UINT64_C(1)<<(i%12);
    }
    for(int frame=0;frame<600;frame++)iq_build_set_rows();
    assert(lookups==1024 && iq_set_counts[0]==1024 && iq_set_counts[1]==1024);
    assert(iq_set_counts[3]==86 && iq_set_counts[7]==85);
    assert(iq_set_rows_count==12 && iq_set_rows[0]==11 && iq_set_rows[11]==0);
    printf("600 unchanged menu updates / 1024 items: %d metadata lookups (previous path: 1228800).\n",lookups);

    iq_set_offset=999;iq_search.set_mode=3;iq_search.sets[0]=1;iq_build_set_rows();
    assert(iq_set_offset==3 && lookups==1024); /* Paging/selection reuse counts. */
    iq_set_offset=-4;iq_build_set_rows();assert(iq_set_offset==0 && lookups==1024);
    wcscpy(iq_search.query,L"potion");iq_build_set_rows();assert(iq_set_counts[0]==512 && lookups==2048);
    active_filter.rarity=1;iq_build_set_rows();assert(iq_set_counts[0]==0 && lookups==3072);
    active_filter.rarity=0;active_filter.type=IQ_ITEM_CONSUMABLES;
    iq_build_set_rows();assert(iq_set_counts[0]==512 && lookups==4096);
    wcscpy(iq_set_query,L"Set 03");iq_set_offset=3;iq_build_set_rows();
    assert(iq_set_rows_count==1 && iq_set_rows[0]==8 && iq_set_offset==0 && lookups==5120);
    iq_set_query[0]=0;active_filter=(IQFilter){0,0};iq_search.query[0]=0;
    iq_build_set_rows();assert(iq_set_rows_count==12 && iq_set_counts[0]==1024);

    /* Same count, changed item membership: metadata rebind must invalidate. */
    metadata[0].sets[0]=0;iq_metadata_revision++;iq_build_set_rows();
    assert(iq_set_counts[3]==85 && iq_set_counts[2]==1);
    /* Empty/failed capture and then a repopulated view. */
    iq.count=0;iq_view_revision++;iq_build_set_rows();assert(iq_set_counts[0]==0);
    iq.count=2;iq_view_revision++;iq_build_set_rows();assert(iq_set_counts[0]==2);
    /* Missing and unknown metadata preserve existing behavior. */
    iq.drawers[0].item_id=9999;metadata[1].sets_known=0;iq_metadata_revision++;
    iq_build_set_rows();assert(iq_set_counts[0]==1 && iq_set_counts[1]==0 && iq_set_counts[2]==0);
    puts("Set counts, alphabetical rows, search/filter invalidation, paging, selection, rebinds and empty views passed.");
    return 0;
}
