#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#ifdef NDEBUG
#error Compile this regression test with assertions enabled (use -UNDEBUG).
#endif
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "../src/native/inventory_index.h"
#include "../src/native/inventory_trait_ui.h"

/* Synthetic GonObject trees in this process stand in for loaded item data. */
static int iq_read(const void* p,void* out,size_t n) {
    SIZE_T got=0;return p && ReadProcessMemory(GetCurrentProcess(),p,out,n,&got) && got==n;
}
static void* iq_ptr(const void* p,size_t off) {void* v=NULL;if(p)iq_read((const unsigned char*)p+off,&v,8);return v;}
static int iq_int(const void* p,size_t off) {int v=0;if(p)iq_read((const unsigned char*)p+off,&v,4);return v;}
static double iq_double(const void* p,size_t off) {double v=0;if(p)iq_read((const unsigned char*)p+off,&v,8);return v;}
typedef struct { unsigned char data[16];uint64_t length,capacity; } IQGameString;
static int iq_string_read(const void* p,char* out,size_t capacity) {
    IQGameString s;
    if(!iq_read(p,&s,sizeof(s)) || s.length>=capacity || s.length>s.capacity || s.capacity>15)return 0;
    memcpy(out,s.data,(size_t)s.length);out[s.length]=0;return 1;
}
typedef struct { _Alignas(8) unsigned char bytes[0xb0]; } Node;
static Node nodes[64],null_node;
static int used;
static Node* node(const char* name,int type) {
    Node* n=&nodes[used++];memset(n,0,sizeof(*n));
    IQGameString s={{0},strlen(name),15};memcpy(s.data,name,s.length);
    memcpy(n->bytes+0x88,&s,sizeof(s));memcpy(n->bytes+0xa8,&type,4);
    return n;
}
static Node* number(const char* name,double value) {Node* n=node(name,2);memcpy(n->bytes+0x58,&value,8);return n;}
static Node* text(const char* name,const char* value) {
    Node* n=node(name,1);IQGameString s={{0},strlen(value),15};memcpy(s.data,value,s.length);
    memcpy(n->bytes+0x68,&s,sizeof(s));return n;
}
/* Children must be contiguous, as in the native vector. */
static Node* parent(const char* name,int type,Node* first,int count) {
    Node* n=node(name,type);void* begin=first;void* end=first+count;
    memcpy(n->bytes+0x38,&begin,8);memcpy(n->bytes+0x40,&end,8);return n;
}
static void* iq_property(void* definition,const char* name) {
    Node* first=iq_ptr(definition,0x38);Node* end=iq_ptr(definition,0x40);
    for(Node* n=first;n && n<end;n++){char key[64];if(iq_string_read(n->bytes+0x88,key,64) && !strcmp(key,name))return n;}
    return &null_node;
}
#include "../src/native/inventory_traits.h"

static int sorted(const IQTraitName* table,size_t count) {
    for(size_t i=1;i<count;i++)if(strcmp(table[i-1].name,table[i].name)>=0)return 0;
    return 1;
}
int main(void) {
    /* Generated tables: binary search needs strictly sorted names. */
    assert(sorted(iq_effect_identifiers,sizeof(iq_effect_identifiers)/sizeof(*iq_effect_identifiers)));
    assert(sorted(iq_ability_effects,sizeof(iq_ability_effects)/sizeof(*iq_ability_effects)));
    uint32_t bleed=1u<<0,poison=1u<<1,thorns=1u<<10,shield=1u<<12,holy=1u<<13,regen=1u<<14;
    assert(iq_effect_bits("Bleed")==bleed && iq_effect_bits("BleedThorns")==(bleed|thorns));
    assert(iq_effect_bits("PoisonThorns")==(poison|thorns) && !iq_effect_bits("bleed") && !iq_effect_bits(""));
    assert(!iq_ability_bits("NotAnAbility"));
    /* Predicate: stat 0 any, 1..7 bonus, 8..14 malus; effects match any. */
    IQItemFx fx={1u<<6,1u<<5,bleed|regen};
    assert(iq_trait_match(0,0,&fx) && iq_trait_match(7,0,&fx) && !iq_trait_match(1,0,&fx));
    assert(iq_trait_match(1+IQ_STAT_COUNT+5,0,&fx) && !iq_trait_match(1+IQ_STAT_COUNT+6,0,&fx));
    assert(iq_trait_match(0,poison|bleed,&fx) && !iq_trait_match(0,poison,&fx) && !iq_trait_match(7,poison,&fx));
    /* Popup geometry agrees with its feedback rectangles. */
    double r[4];
    for(int i=0;i<IQ_STAT_CHOICES;i++){iq_stat_rect(i,r);int c=iq_stat_choice(r[0]+1,r[1]+1);assert(c==(i?(((i-1)%2)?1+IQ_STAT_COUNT+(i-1)/2:1+(i-1)/2):0));}
    assert(iq_stat_choice(165,90)==-1 && iq_stat_choice(20,82)==-1 && iq_stat_choice(20,86+7*43)==-1);
    for(int i=0;i<IQ_EFFECT_COUNT;i++){iq_effect_rect(i,r);assert(iq_effect_choice(r[0]+1,r[1]+1)==i && iq_effect_choice(r[0]+r[2]-1,r[1]+r[3]-1)==i);}
    iq_effect_rect(IQ_EFFECT_COUNT,r);assert(iq_effect_choice(r[0]+1,r[1]+1)==IQ_EFFECT_CLEAR);
    iq_effect_rect(IQ_EFFECT_COUNT+1,r);assert(iq_effect_choice(r[0]+1,r[1]+1)==IQ_EFFECT_DONE);
    assert(iq_effect_choice(215,50)==IQ_EFFECT_GAP && iq_effect_choice(220,43+8*40+1)==IQ_EFFECT_GAP);
    puts("Trait tables, matching and popup geometry passed.");

    /* An armor piece: +lck, -spd, armor shield, a bleed-on-hit passive, a set
       named like an effect (ignored) and a SpeedUp passive (stat bonus). */
    Node* inner=number("Bleed",1);
    Node* passive_kids=&nodes[used];
    parent("AddStatusToBasicAttack",3,inner,1);number("SpeedUp",2);
    Node* kids=&nodes[used];
    number("lck",2);number("spd",-1);number("shield",3);text("set","Shield");
    parent("passives",3,passive_kids,2);text("name","ITEM_X_NAME");
    Node* item=parent("Item",3,kids,6);
    IQItemFx got=iq_item_traits(item);
    assert(got.up==((1u<<6)|(1u<<5)) && got.down==(1u<<5));
    assert(got.effects==(bleed|shield));
    /* A divine_shield field and an unknown ability stay readable. */
    used=0;Node* k2=&nodes[used];number("divine_shield",1);text("ability","NoSuchAbility");number("con",0);
    Node* item2=parent("Item2",3,k2,3);
    got=iq_item_traits(item2);
    assert(got.effects==holy && !got.up && !got.down);
    /* Cache: one walk per definition pointer. */
    IQItemFx a=iq_item_traits_cached(item2),b=iq_item_traits_cached(item2);
    assert(a.effects==b.effects && iq_trait_count==1);
    /* Malformed vectors stop the walk without reading past them. */
    used=0;Node* broken=node("Broken",3);void* bad=(void*)0x10;memcpy(broken->bytes+0x38,&bad,8);
    got=iq_item_traits(broken);assert(!got.effects && !got.up);
    puts("Native GON trait walk: stats, passives, shields, ignored fields, cache and bad vectors passed.");
    return 0;
}
