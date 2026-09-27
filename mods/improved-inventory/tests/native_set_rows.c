#include <assert.h>
#ifdef NDEBUG
#error Compile this regression test with assertions enabled (use -UNDEBUG).
#endif
#include <stdio.h>
#include <string.h>
#include "../src/native/inventory_filter.h"
#include "../src/native/inventory_search.h"

/* Only game memory is faked. Ownership, menu and matching use production code. */
typedef struct {void* drawer;uint64_t item_id;IQItemTraits traits;int rarity,side;} IQDrawer;
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
#include "../src/native/inventory_ownership.h"
#include "../src/native/inventory_set_rows.h"

int main(void) {
    iq.count=1024;iq_set_count=12;
    for(int s=0;s<12;s++)swprintf(iq_sets[s].name,160,L"Set %02d",11-s);
    for(int i=0;i<iq.count;i++) {
        iq.drawers[i]=(IQDrawer){.item_id=(uint64_t)i,.traits={i%2,0,0,0},.rarity=i%2?5:1,.side=i%2};
        wcscpy(metadata[i].text,i%2?L"Rare potion":L"Common hat");
        metadata[i].sets_known=1;metadata[i].sets[0]=UINT64_C(1)<<(i%12);
    }
    for(int frame=0;frame<600;frame++)iq_build_set_rows();
    assert(lookups==1024 && iq_owned.counts[0][0]==86 && iq_owned.counts[0][1]==0);
    assert(iq_owned.counts[1][1]==86 && iq_owned.counts[4][0]==85);
    assert(iq_set_rows_count==12 && iq_set_rows[0]==11 && iq_set_rows[11]==0);
    printf("600 unchanged menu updates / 1024 items: %d metadata lookups.\n",lookups);
    iq_set_offset=999;iq_search.set_mode=3;iq_search.sets[0]=1;iq_build_set_rows();
    assert(iq_set_offset==5 && lookups==1024);
    iq_set_offset=-4;iq_build_set_rows();assert(iq_set_offset==0 && lookups==1024);
    /* All filters/selection/query/paging reuse full owned counts. */
    wcscpy(iq_search.query,L"potion");active_filter.rarity=1;active_filter.type=IQ_ITEM_CONSUMABLES;
    iq_build_set_rows();
    assert(iq_owned.counts[0][0]==86 && lookups==1024 && iq_set_rows_count==12);
    wcscpy(iq_set_query,L"Set 03");iq_set_offset=3;iq_build_set_rows();
    assert(iq_set_rows_count==1 && iq_set_rows[0]==8 && iq_set_offset==0 && lookups==1024);
    iq_set_query[0]=0;

    /* Count every copy, including worn items, in its current container. */
    iq.count=3;iq_view_revision++;
    for(int i=0;i<3;i++)metadata[i].sets[0]=1;
    iq.drawers[0].side=0;iq.drawers[1].side=1;iq.drawers[2].side=1;
    iq.drawers[2].traits.condition=1;
    iq_build_set_rows();
    assert(iq_owned.counts[0][0]==1 && iq_owned.counts[0][1]==2);
    assert(iq_set_rows_count==12);
    /* Moving an item changes columns, preserving totals. */
    iq.drawers[1].side=0;iq_view_revision++;iq_build_set_rows();
    assert(iq_owned.counts[0][0]==2 && iq_owned.counts[0][1]==1);
    /* Membership rebinds refresh counts even when the grid hasn't changed. */
    metadata[1].sets[0]=3;iq_metadata_revision++;iq_build_set_rows();
    assert(iq_owned.counts[0][0]==2 && iq_owned.counts[1][0]==1);
    /* Partial and missing metadata cannot contribute guessed membership. */
    metadata[2].sets_known=0;iq_metadata_revision++;iq_build_set_rows();
    assert(iq_owned.counts[0][0]==2 && iq_owned.counts[0][1]==0);
    iq.drawers[0].item_id=9999;iq_metadata_revision++;iq_build_set_rows();assert(iq_owned.counts[0][0]==1);
    /* Empty/repopulated views and highest supported set index. */
    iq.count=0;iq_view_revision++;iq_build_set_rows();assert(iq_owned.counts[0][0]==0 && iq_set_rows_count==12);
    iq.count=2;iq_view_revision++;iq_set_count=256;iq_search.set_mode=0;
    for(int i=0;i<2;i++) {
        iq.drawers[i].item_id=(uint64_t)i;memset(metadata[i].sets,0,sizeof(metadata[i].sets));
        metadata[i].sets[3]=UINT64_C(1)<<63;
    }
    wcscpy(iq_sets[255].name,L"Last set");iq_build_set_rows();
    assert(iq_owned.counts[255][0]==2 && iq_set_rows_count==256);
    /* Assigning equipment reduces Available without hiding any set rows. */
    iq.count=1;iq_view_revision++;iq_build_set_rows();
    assert(iq_owned.counts[255][0]==1 && iq_set_rows_count==256);
    puts("Container counts, transfers, metadata rebinds, cache reuse and empty views passed.");
    return 0;
}
