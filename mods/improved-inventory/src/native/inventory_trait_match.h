/* Pure stat/effect trait matching; tables are generated from game data. */
#pragma once
#include <stdint.h>
#include <string.h>
#include "inventory_trait_tables.h"
typedef struct { uint8_t up,down; uint32_t effects; } IQItemFx;
static uint32_t iq_trait_lookup(const IQTraitName* table,size_t count,const char* name) {
    size_t lo=0,hi=count;
    while(lo<hi) {
        size_t mid=(lo+hi)/2;int order=strcmp(name,table[mid].name);
        if(!order)return table[mid].bits;
        if(order<0)hi=mid;else lo=mid+1;
    }
    return 0;
}
static uint32_t iq_effect_bits(const char* name) {
    return iq_trait_lookup(iq_effect_identifiers,sizeof(iq_effect_identifiers)/sizeof(*iq_effect_identifiers),name);
}
static uint32_t iq_ability_bits(const char* name) {
    return iq_trait_lookup(iq_ability_effects,sizeof(iq_ability_effects)/sizeof(*iq_ability_effects),name);
}
/* Per-choice item counts for the Stats/Effects menus: index 0 counts every
   item with known traits, then bonus, malus and effect choices. */
typedef struct { int stats[1+2*IQ_STAT_COUNT],effects[IQ_EFFECT_COUNT]; } IQTraitCounts;
static void iq_trait_tally(IQTraitCounts* counts,const IQItemFx* fx) {
    counts->stats[0]++;
    for(int i=0;i<IQ_STAT_COUNT;i++) {
        if(fx->up&(1u<<i))counts->stats[1+i]++;
        if(fx->down&(1u<<i))counts->stats[1+IQ_STAT_COUNT+i]++;
    }
    for(int i=0;i<IQ_EFFECT_COUNT;i++)if(fx->effects&(1u<<i))counts->effects[i]++;
}
/* Stat filter: 0 any, 1..7 bonus, 8..14 malus (iq_stat_fields order). */
static int iq_trait_match(int stat,uint32_t effects,const IQItemFx* fx) {
    if(stat>0 && stat<=IQ_STAT_COUNT && !(fx->up&(1u<<(stat-1))))return 0;
    if(stat>IQ_STAT_COUNT && !(fx->down&(1u<<(stat-1-IQ_STAT_COUNT))))return 0;
    return !effects || (fx->effects&effects)!=0;
}
