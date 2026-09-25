#include <assert.h>
#include "../src/native/inventory_filter.h"
int main(void) {
    /* Game rarity pairs must normalize together without equating quest to rare. */
    for(int i=1;i<=8;i++)assert(iq_rarity_group(i)==(i+1)/2);
    for(int rank=9;rank<=15;rank++) {
        assert(iq_rarity_group(rank)==0);
        for(int filter=1;filter<=4;filter++)assert(!iq_filter_match((IQFilter){IQ_ALL,filter},(IQItemTraits){0,0,0,0},rank));
    }
    assert(iq_rarity_choice(10,61)==0 && iq_rarity_choice(319,112)==0);
    assert(iq_rarity_choice(85,195)==1 && iq_rarity_choice(245,195)==2);
    assert(iq_rarity_choice(85,355)==3 && iq_rarity_choice(245,355)==4);
    assert(iq_rarity_choice(165,195)==-1 && iq_rarity_choice(85,280)==-1);
    assert(iq_rarity_choice(320,195)==-1 && iq_rarity_choice(85,436)==-1);
    assert(iq_rarity_choice(85,113)==-1);
    assert(iq_rarity_choice(85,50)==-1 && iq_rarity_choice(-1,20)==-1);
    IQItemTraits unknown={-1,-1,-1,-1},limited={0,1,1,3},reusable={0,1,0,4},consumable={1,0,0,5};
    assert(iq_filter_match((IQFilter){IQ_ALL,0},unknown,-1));
    for(int type=1;type<IQ_TYPE_COUNT;type++)assert(!iq_filter_match((IQFilter){type,0},unknown,5));
    assert(iq_filter_match((IQFilter){IQ_WEAPON_CONSUMABLES,3},limited,5));
    assert(!iq_filter_match((IQFilter){IQ_WEAPON_CONSUMABLES,0},reusable,5));
    assert(!iq_filter_match((IQFilter){IQ_WEAPON_CONSUMABLES,0},consumable,5));
    assert(iq_filter_match((IQFilter){IQ_REUSABLE_WEAPONS,0},reusable,5));
    assert(!iq_filter_match((IQFilter){IQ_REUSABLE_WEAPONS,0},limited,5));
    assert(iq_filter_match((IQFilter){IQ_ITEM_CONSUMABLES,3},consumable,5));
    assert(!iq_filter_match((IQFilter){IQ_ITEM_CONSUMABLES,0},limited,5));
    assert(!iq_filter_match((IQFilter){IQ_ITEM_CONSUMABLES,1},consumable,5));
    assert(iq_filter_match((IQFilter){IQ_WORN,0},limited,5));
    assert(iq_filter_match((IQFilter){IQ_WORN,0},reusable,5));
    assert(!iq_filter_match((IQFilter){IQ_WORN,0},consumable,5));
    assert(iq_filter_match((IQFilter){IQ_BROKEN,0},consumable,5));
    assert(!iq_filter_match((IQFilter){IQ_BROKEN,0},limited,5));
    assert(!iq_filter_match((IQFilter){IQ_ALL,3},consumable,9));
    assert(!iq_filter_match((IQFilter){IQ_ALL,1},consumable,-1));
    assert(iq_max_row(0,0)==0 && iq_max_row(36,6)==0);
    assert(iq_max_row(37,6)==1 && iq_max_row(42,6)==1);
    assert(iq_max_row(43,6)==2 && iq_max_row(94,6)==10);
    return 0;
}
