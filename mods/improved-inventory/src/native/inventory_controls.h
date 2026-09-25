/* Original native controls; all owned renderers are allocated by the game scene. */
typedef void (__cdecl *BindItem)(void*,void*);
typedef double* (__cdecl *MousePosition)(void*,double*);
static BindItem original_bind_item;
static MousePosition original_mouse_position;
typedef unsigned char (__cdecl *ButtonHit)(void*);
static ButtonHit original_button_hit;
static void* iq_camera;
static uint64_t iq_camera_generation;
typedef struct {void* drawer;uint64_t id,generation;IQItemTraits traits; wchar_t text[4096]; uint64_t sets[4]; int sets_known;} IQMetadata;
static struct {char id[128]; wchar_t name[160];} iq_sets[IQ_SET_LIMIT];
static int iq_set_count;
static wchar_t iq_set_query[IQ_QUERY_LIMIT];
static int iq_set_offset,iq_focus;
static IQMetadata iq_metadata[IQ_MAX_DRAWERS];
static int iq_metadata_next;
#include "inventory_metadata.h"

static IQItemTraits iq_traits(void* drawer,uint64_t id) {
    IQMetadata* m=iq_find_metadata(drawer,id);
    return m?m->traits:(IQItemTraits){-1,-1,-1,-1};
}
static void* iq_property(void* definition,const char* name) {
    struct {char data[16];uint64_t size,capacity;} key={{0},0,15};
    key.size=strlen(name);if(key.size>15)return NULL;
    memcpy(key.data,name,key.size+1);
    return ((void*(__cdecl*)(void*,void*))(void*)(game_base+0x942da0))(definition,&key);
}
static int iq_weapon(void* value) {
    if(iq_int(value,0xa8)!=1)return iq_int(value,0xa8)==0?0:-1;
    uint64_t size=0;iq_read((unsigned char*)value+0x78,&size,8);
    if(size!=6)return 0;
    char kind[6];return iq_read((unsigned char*)value+0x68,kind,6)?!memcmp(kind,"weapon",6):-1;
}
static int iq_limited(void* value) {
    int type=iq_int(value,0xa8);
    if(type==0)return 0;
    if(type==2) {double n=iq_double(value,0x58);return isfinite(n)?n>0:-1;}
    if(type==4) {
        unsigned char* first=iq_ptr(value,0x38);
        uintptr_t end=(uintptr_t)iq_ptr(value,0x40);
        if(!first || end!=(uintptr_t)first+2*0xb0)return -1;
        if(iq_int(first,0xa8)!=2 || iq_int(first+0xb0,0xa8)!=2)return -1;
        double lo=iq_double(first,0x58),hi=iq_double(first+0xb0,0x58);
        return isfinite(lo)&&isfinite(hi)&&lo>0&&hi>=lo?1:-1;
    }
    return -1;
}
static void iq_cache_item(void* drawer,void* item) {
    if(!layout_test) return;
    void* manager=iq_ptr(game_base,0x13c79d0);
    if(!manager || !item) return;
    typedef void*(__cdecl *Lookup)(void*,void*);
    Lookup lookup=(Lookup)(void*)(game_base+0x942da0);
    void* definition=lookup((unsigned char*)manager+0x5b8,(unsigned char*)item+8);
    if(!definition) return;
    void* value=iq_property(definition,"consumable");
    unsigned char flag=0;
    int known=iq_int(value,0xa8)==5 && iq_read((unsigned char*)value+0x60,&flag,1);
    /* Missing property is the game's default false; non-boolean is unknown. */
    int type=iq_int(value,0xa8);
    int consumable=known?(flag!=0):(type==0?0:-1);
    uint64_t id=0;iq_read(item,&id,8);
    int index=-1;
    for(int i=0;i<IQ_MAX_DRAWERS;i++) if(iq_metadata[i].drawer==drawer){index=i;break;}
    if(index<0) index=iq_metadata_next++%IQ_MAX_DRAWERS;
    IQItemTraits traits={consumable,iq_weapon(iq_property(definition,"kind")),
        iq_limited(iq_property(definition,"durability")),iq_int(item,0x5c)};
    iq_metadata[index]=(IQMetadata){.drawer=drawer,.id=id,.generation=iq_generation(drawer),.traits=traits};
    iq_search_metadata(&iq_metadata[index],manager,item,definition);
    iq_metadata_revision++;
}
static void __cdecl iq_bind(void* drawer,void* item) {
    original_bind_item(drawer,item);
    iq_cache_item(drawer,item);
}
static unsigned char __cdecl iq_button_hit(void* button) {
    if(layout_test && iq_is_open())
        for(int i=0;i<iq.count;i++)
            if((filter_popup || !iq_visible(&iq.drawers[i])) &&
               iq_reference_valid(iq.drawers[i].reference) && iq_drawer_button(iq.drawers[i].drawer)==button)return 0;
    return original_button_hit(button);
}
static double* __cdecl iq_mouse_position(void* camera,double* point) {
    double* result=original_mouse_position(camera,point);
    if(layout_test) {iq_camera=camera;iq_camera_generation=iq_generation(camera);}
    return result;
}
#include "inventory_widgets.h"
