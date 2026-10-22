/* Stat and effect traits from the effective loaded item definition (GON).
 * Read-only walk of the game's GonObject tree; ability effects come from the
 * generated build-time table. Unknown or unreadable data stays unmatched. */
/* GonObject layout in the supported build: children vector (objects and
   arrays), number, string value, key name, field type; 0xb0 bytes each. */
#define IQ_GON_CHILDREN 0x38
#define IQ_GON_NUMBER 0x58
#define IQ_GON_STRING 0x68
#define IQ_GON_NAME 0x88
#define IQ_GON_TYPE 0xa8
#define IQ_GON_SIZE 0xb0
enum { IQ_GON_NULL, IQ_GON_TEXT, IQ_GON_VALUE, IQ_GON_OBJECT, IQ_GON_ARRAY };
#define IQ_TRAIT_DEPTH 8
#define IQ_TRAIT_NODES 512
static void iq_trait_walk(const unsigned char* node,int depth,int ability,IQItemFx* fx,int* budget) {
    if(!node || depth>IQ_TRAIT_DEPTH || --*budget<0)return;
    char text[64];int type=iq_int(node,IQ_GON_TYPE);
    if(depth && iq_string_read(node+IQ_GON_NAME,text,sizeof(text))) {
        /* Display/set fields hold localization keys and set names, not effects. */
        if(depth==1 && (!strcmp(text,"set") || !strcmp(text,"name") || !strcmp(text,"desc")))return;
        fx->effects|=iq_effect_bits(text);
        if(depth==1 && !strcmp(text,"ability"))ability=1;
        if(depth==1 && type==IQ_GON_VALUE && iq_double(node,IQ_GON_NUMBER)>0) {
            if(!strcmp(text,"shield"))fx->effects|=iq_effect_bits("Shield");
            if(!strcmp(text,"divine_shield"))fx->effects|=iq_effect_bits("DivineShield");
        }
    }
    if(type==IQ_GON_TEXT && iq_string_read(node+IQ_GON_STRING,text,sizeof(text)))
        fx->effects|=iq_effect_bits(text)|(ability?iq_ability_bits(text):0);
    if(type!=IQ_GON_OBJECT && type!=IQ_GON_ARRAY)return;
    uintptr_t first=(uintptr_t)iq_ptr(node,IQ_GON_CHILDREN),end=(uintptr_t)iq_ptr(node,IQ_GON_CHILDREN+8);
    if(!first || end<first || end-first>(uintptr_t)IQ_TRAIT_NODES*IQ_GON_SIZE || (end-first)%IQ_GON_SIZE)return;
    for(uintptr_t child=first;child<end && *budget>0;child+=IQ_GON_SIZE)
        iq_trait_walk((const unsigned char*)child,depth+1,ability,fx,budget);
}
static IQItemFx iq_item_traits(void* definition) {
    IQItemFx fx={0,0,0};
    for(int i=0;i<IQ_STAT_COUNT;i++) {
        void* stat=iq_property(definition,iq_stat_fields[i]);
        if(iq_int(stat,IQ_GON_TYPE)==IQ_GON_VALUE) {
            double value=iq_double(stat,IQ_GON_NUMBER);
            if(value>0)fx.up|=(uint8_t)(1u<<i);else if(value<0)fx.down|=(uint8_t)(1u<<i);
        }
    }
    /* Direct "Up" passives (passives { SpeedUp 2 }) also raise the wearer's stat. */
    void* passives=iq_property(definition,"passives");
    if(iq_int(passives,IQ_GON_TYPE)==IQ_GON_OBJECT)
        for(int i=0;i<IQ_STAT_COUNT;i++)
            if(iq_int(iq_property(passives,iq_stat_up_passives[i]),IQ_GON_TYPE)!=IQ_GON_NULL)fx.up|=(uint8_t)(1u<<i);
    int budget=IQ_TRAIT_NODES;
    iq_trait_walk(definition,0,0,&fx,&budget);
    return fx;
}
/* Item definitions are shared and stay loaded; walk each one once. */
#define IQ_TRAIT_CACHE 1536
static IQIndex iq_trait_index;
static IQItemFx iq_trait_cache[IQ_TRAIT_CACHE];
static int iq_trait_count;
static IQItemFx iq_item_traits_cached(void* definition) {
    int at=iq_index_get(&iq_trait_index,definition);
    if(at>=0 && at<iq_trait_count)return iq_trait_cache[at];
    IQItemFx fx=iq_item_traits(definition);
    if(iq_trait_count<IQ_TRAIT_CACHE && iq_index_put(&iq_trait_index,definition,iq_trait_count))iq_trait_cache[iq_trait_count++]=fx;
    return fx;
}
