#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../src/native/inventory_filter.h"
#include "../src/native/inventory_index.h"
#define IQ_MAX_DRAWERS 1024
static unsigned reads;
static const void* unreadable;
static uint64_t iq_generation(const void* drawer) {
    reads++;
    return drawer==unreadable?0:*(const uint64_t*)drawer;
}
#include "../src/native/inventory_metadata_cache.h"
static uint64_t objects[4096];
int main(void) {
    /* Deletion repairs a collision chain, including wraparound. */
    IQIndex index={0};const void* keys[12];unsigned count=0;
    for(uintptr_t p=16;count<12;p+=16)if(iq_index_slot((void*)p)==IQ_INDEX_CAPACITY-1) {
        keys[count]=(void*)p;assert(iq_index_put(&index,keys[count],(int)count));count++;
    }
    iq_index_remove(&index,keys[0]);iq_index_remove(&index,keys[6]);
    for(unsigned i=0;i<count;i++)assert(iq_index_get(&index,keys[i])==(i==0 || i==6?-1:(int)i));
    iq_index_remove(&index,(void*)1);
    /* Multiple full ring turnovers must neither fill the index nor leak stale keys. */
    for(int i=0;i<4096;i++) {
        objects[i]=(uint64_t)i+1;
        IQMetadata* m=iq_metadata_slot(&objects[i]);
        *m=(IQMetadata){.drawer=&objects[i],.id=(uint64_t)i+10,.generation=objects[i]};
        if(i>=1024)assert(!iq_find_metadata(&objects[i-1024],(uint64_t)i-1024+10));
        assert(iq_find_metadata(&objects[i],(uint64_t)i+10)==m);
    }
    reads=0;
    for(int i=3072;i<4096;i++)assert(iq_find_metadata(&objects[i],(uint64_t)i+10));
    assert(reads==1024);
    IQMetadata* m=iq_metadata_slot(&objects[4000]);unsigned next=iq_metadata_next;
    m->id=42;assert(iq_metadata_slot(&objects[4000])==m && iq_metadata_next==next);
    assert(!iq_find_metadata(&objects[4000],4010));assert(iq_find_metadata(&objects[4000],42)==m);
    objects[4000]++;assert(!iq_find_metadata(&objects[4000],42));
    m->generation=objects[4000];unreadable=m->drawer;assert(!iq_find_metadata(m->drawer,42));
    unreadable=NULL;assert(iq_find_metadata(m->drawer,42)==m);
    reads=0;assert(!iq_find_metadata(NULL,0));assert(!iq_find_metadata(&objects[0],10));assert(!reads);
    puts("Metadata: indexed lookup, collisions, eviction, rebinding and pooled identities passed.");
}
