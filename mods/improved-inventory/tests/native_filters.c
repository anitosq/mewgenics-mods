#include <assert.h>
#include "../src/native/inventory_filter.h"
int main(void) {
    /* Game rarity pairs must normalize together without equating quest to rare. */
    for(int i=1;i<=8;i++)assert(iq_rarity_group(i)==(i+1)/2);
    for(int rank=9;rank<=15;rank++) {
        assert(iq_rarity_group(rank)==0);
        for(int filter=1;filter<=4;filter++)assert(!iq_filter_match((IQFilter){IQ_ALL,filter},(IQItemTraits){0,0,0,0,0},rank));
    }
    assert(iq_rarity_choice(10,61)==0 && iq_rarity_choice(319,112)==0);
    assert(iq_rarity_choice(85,195)==1 && iq_rarity_choice(245,195)==2);
    assert(iq_rarity_choice(85,355)==3 && iq_rarity_choice(245,355)==4);
    assert(iq_rarity_choice(165,195)==-1 && iq_rarity_choice(85,280)==-1);
    assert(iq_rarity_choice(320,195)==-1 && iq_rarity_choice(85,436)==-1);
    assert(iq_rarity_choice(85,113)==-1);
    assert(iq_rarity_choice(85,50)==-1 && iq_rarity_choice(-1,20)==-1);
    IQItemTraits unknown={-1,-1,-1,-1,0},limited={0,1,1,3,1},reusable={0,1,0,4,1},consumable={1,0,0,5,5};
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
    /* Equipment slots from the item kind; unknown kinds match none. */
    assert(iq_slot_index("weapon")==1 && iq_slot_index("head")==2 && iq_slot_index("face")==4 && iq_slot_index("trinket")==5 && !iq_slot_index("modifier") && !iq_slot_index(NULL));
    IQItemTraits face={0,0,0,0,4},sword={0,1,0,0,1};
    assert(iq_filter_match((IQFilter){IQ_FACE,0},face,5) && !iq_filter_match((IQFilter){IQ_HEAD,0},face,5));
    assert(iq_filter_match((IQFilter){IQ_WEAPON,0},sword,5) && iq_filter_match((IQFilter){IQ_REUSABLE_WEAPONS,0},sword,5));
    for(int type=IQ_FIRST_SLOT_TYPE;type<IQ_TYPE_COUNT;type++)assert(!iq_filter_match((IQFilter){type,0},unknown,5));
    /* Type popup geometry agrees with its feedback rectangles. */
    for(int i=0;i<IQ_TYPE_COUNT;i++){double r[4];iq_type_rect(i,r);assert(iq_type_choice(r[0]+1,r[1]+1)==i && iq_type_choice(r[0]+r[2]-1,r[1]+r[3]-1)==i);}
    assert(iq_type_choice(275,50)==-1 && iq_type_choice(300,43+5*43+1)==-1 && iq_type_choice(20,43+6*43+1)==-1 && iq_type_choice(20,30)==-1);
    assert(iq_max_row(0,0)==0 && iq_max_row(36,6)==0);
    assert(iq_max_row(37,6)==1 && iq_max_row(42,6)==1);
    assert(iq_max_row(43,6)==2 && iq_max_row(94,6)==10);
    /* Equipment has six columns but five visible rows. Every item remains
       reachable, including the first overflow and a partially filled last row. */
    assert(iq_last_row(30,6,5)==0 && iq_last_row(31,6,5)==1);
    for(int count=0;count<=1024;count++) {
        int seen[1024]={0},last=iq_last_row(count,6,5);
        for(int row=0;row<=last;row++)
            for(int item=row*6;item<count && item<(row+5)*6;item++)seen[item]=1;
        for(int item=0;item<count;item++)assert(seen[item]);
        assert(iq_last_row(count>7?7:count,6,5)==0);
    }
    return 0;
}
