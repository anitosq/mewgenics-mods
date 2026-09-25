/* Experimental presentation only. Item containers and item IDs are never written. */
#include <math.h>
#include "inventory_filter.h"
#include "inventory_search.h"
#define IQ_MAX_DRAWERS 1024
#define IQ_COLUMNS 6
typedef void (__cdecl *DrawerUpdate)(void*);
typedef unsigned char (__cdecl *MouseEvent)(void*, void*);
typedef void (__cdecl *ItemClick)(void*);
static DrawerUpdate original_drawer_update;
static MouseEvent original_mouse_event;
static ItemClick original_item_click;
static int layout_test;
static unsigned char* game_base;
static IQFilter active_filter;
static int filter_popup;
static IQSearch iq_search;
static int iq_extended_match(void* drawer,uint64_t id,int include_sets);
static void iq_ui_update(void);
static int iq_ui_event(void* event);
static int iq_wheel_side(void);
static void iq_ui_capture(void);
static IQItemTraits iq_traits(void* drawer, uint64_t id);
static int iq_read(const void* p, void* out, size_t n);
#include "inventory_lifetime.h"
#include "inventory_index.h"

typedef struct {
    void* renderer;
    void* transform;
    uint64_t renderer_generation, transform_generation;
    int side, ordinal;
    int tracking_index;
    double offset_x, offset_y;
    double x,y,sx,sy;
    unsigned char visible;
} IQBackground;
typedef struct {
    void* drawer;
    void* button;
    IQReference reference;
    uint64_t item_id;
    int side, ordinal;
    IQItemTraits traits;
    int rarity;
} IQDrawer;
typedef struct {
    void* owner;
    int equipment;
    void* panels[2];
    IQReference owner_ref, panel_refs[2], scene_ref, manager_ref, entity_ref;
    IQDrawer drawers[IQ_MAX_DRAWERS];
    int count, items[2], row[2], columns[2];
    IQBackground backgrounds[800];
    int background_count;
    double left[2], bottom[2], span[2], scale[2];
    ULONGLONG last_update;
} IQView;
static IQView iq;
static IQView pending_view;
static IQIndex iq_drawer_index,iq_button_index;
static uint64_t iq_view_revision,iq_metadata_revision;
static int iq_view_live(void) {
    return iq_reference_valid(iq.owner_ref) && iq_reference_valid(iq.scene_ref) && iq_reference_valid(iq.entity_ref) &&
        iq_reference_valid(iq.panel_refs[0]) && (iq.equipment ||
        (iq_reference_valid(iq.panel_refs[1]) && iq_reference_valid(iq.manager_ref)));
}

static int iq_read(const void* p, void* out, size_t n) {
    SIZE_T received=0;
    return p && ReadProcessMemory(GetCurrentProcess(),p,out,n,&received) && received==n;
}
static void* iq_ptr(const void* p, size_t off) {
    void* value=NULL;
    if (p) iq_read((const unsigned char*)p+off,&value,sizeof(value));
    return value;
}
static int iq_int(const void* p,size_t off) {
    int value=0;
    if (p) iq_read((const unsigned char*)p+off,&value,sizeof(value));
    return value;
}
static double iq_double(const void* p,size_t off) {
    double value=0;
    if (p) iq_read((const unsigned char*)p+off,&value,sizeof(value));
    return value;
}
static uint64_t iq_generation(const void* p) {
    uint64_t value=0;
    if(p) iq_read((const unsigned char*)p-8,&value,sizeof(value));
    return value;
}
static void iq_write_double(void* p,size_t off,double value) {
    memcpy((unsigned char*)p+off,&value,sizeof(value));
}
static void* iq_panel_transform(int side) {
    return iq.equipment?NULL:iq_ptr(iq.panels[side],0x60);
}
static void* iq_drawer_button(void* drawer) {
    return iq_ptr(drawer,iq.equipment?0x48:0x58);
}
static void iq_index_view(void) {
    iq_index_clear(&iq_drawer_index);iq_index_clear(&iq_button_index);
    for(int i=0;i<iq.count;i++) {
        IQDrawer* d=&iq.drawers[i];d->button=iq_drawer_button(d->drawer);
        iq_index_put(&iq_drawer_index,d->drawer,i);
        iq_index_put(&iq_button_index,d->button,i);
    }
}
static void* iq_tracking(void* panel, void* transform, int* index) {
    uintptr_t first=(uintptr_t)iq_ptr(panel,0x70),end=(uintptr_t)iq_ptr(panel,0x78);
    if(!first || end<first || end-first>2000*0x28 || (end-first)%0x28) return NULL;
    for(uintptr_t p=first;p<end;p+=0x28) {
        if(iq_ptr((void*)p,0)==transform && iq_generation(transform)==*(uint64_t*)(p+8)) {
            if(index) *index=(int)((p-first)/0x28);
            return (void*)p;
        }
    }
    return NULL;
}

static int iq_rows(int side) {return iq.equipment && iq.columns[side]>5?5:iq.columns[side];}
static int iq_scroll_limit(int side) {return iq_last_row(iq.items[side],iq.columns[side],iq_rows(side));}
static int iq_visible(const IQDrawer* d) {
    if(d->ordinal<0) return 0;
    int cols=iq.columns[d->side];
    int row=d->ordinal/cols-iq.row[d->side];
    return row>=0 && row<iq_rows(d->side);
}

static void iq_present(IQDrawer* d) {
    if(!iq_reference_valid(d->reference))return;
    unsigned char* drawer=d->drawer;
    void* renderer=iq_ptr(drawer,iq.equipment?0x40:0x50);
    void* button=iq_drawer_button(drawer);
    void* transform=iq_ptr(drawer,iq.equipment?0x50:0x60);
    if (!renderer || !button || !transform) return;
    const int side=d->side;
    int cols=iq.columns[side];
    double pitch=iq.span[side]/cols*(side==0?0.90:1.0);
    int row=d->ordinal/cols-iq.row[side];
    int visible=iq_visible(d);
    double x=visible ? pitch*(d->ordinal%cols)+pitch/2 : -100000.0;
    double y=visible ? pitch*(cols-1-row)+pitch/2-(side==0?iq.span[side]*0.072:0) : -100000.0;
    if(iq.equipment && visible)y=iq.span[0]-y;
    iq_write_double(drawer,iq.equipment?0xb8:0x78,x+(iq.equipment?iq.left[side]:0));
    iq_write_double(drawer,iq.equipment?0xc0:0x80,y+(iq.equipment?iq.bottom[side]:0));
    iq_write_double(drawer,iq.equipment?0xc8:0x88,iq.scale[side]*(side==0?0.90:1.0));
    /* The game's instant layout method updates the transform used by both its
       renderer and button. This avoids animating hidden items across hit areas. */
    ((DrawerUpdate)(void*)(game_base+(iq.equipment?0x34e230:0x213a20)))(drawer);
    *((unsigned char*)renderer+0x51)=(unsigned char)visible;
    /* Keep the native button update enabled: it owns mouse-leave/tooltip
       cleanup. Hidden buttons move offscreen and the click hook rejects them. */
}

static void iq_before_grid(void* owner) {
    if(iq.owner==owner){iq_index_clear(&iq_drawer_index);iq_index_clear(&iq_button_index);}
    if(!layout_test || iq.owner!=owner || !iq_view_live()) return;
    for(int i=0;i<iq.background_count;i++) {
        IQBackground* b=&iq.backgrounds[i];
        if(iq_ptr(b->renderer,0x40)!=b->transform ||
           iq_generation(b->renderer)!=b->renderer_generation ||
           iq_generation(b->transform)!=b->transform_generation) continue;
        void* parent_transform=iq_panel_transform(b->side);
        if(!iq.equipment && !parent_transform) continue;
        iq_write_double(b->transform,0x80,iq_double(parent_transform,0x80)+b->offset_x);
        iq_write_double(b->transform,0x88,iq_double(parent_transform,0x88)+b->offset_y);
        iq_write_double(b->transform,0x98,b->sx);
        iq_write_double(b->transform,0xa0,b->sy);
        *((unsigned char*)b->renderer+0x51)=b->visible;
        void* track=iq.equipment?NULL:iq_tracking(iq.panels[b->side],b->transform,NULL);
        if(track) {
            iq_write_double(track,0x10,b->offset_x);
            iq_write_double(track,0x18,b->offset_y);
        }
    }
    iq.background_count=0;
}

static void iq_capture(void* owner) {
    if (!layout_test) return;
    /* Include failed/empty captures: they can also invalidate menu counts. */
    iq_view_revision++;
    memset(&pending_view,0,sizeof(pending_view));
    if(iq.owner==owner && iq_view_live()) memcpy(pending_view.row,iq.row,sizeof(iq.row));
    memset(&iq,0,sizeof(iq));
    /* Collect and validate the entire view before performing layout writes. */
#define iq pending_view
    iq.owner=owner;
    iq.panels[0]=iq_ptr(owner,0x38);
    iq.panels[1]=iq_ptr(owner,0x40);
    iq.owner_ref=iq_reference(owner);
    for(int side=0;side<2;side++)iq.panel_refs[side]=iq_reference(iq.panels[side]);
    iq.scene_ref=iq_reference(iq_ptr(iq.panels[0],0x20));
    void* entity=iq_ptr(iq.panels[0],0x18);
    if(!entity || !iq_ptr(entity,8))return;
    iq.entity_ref=iq_reference(entity);
    /* Resolve while the native layout callback owns a live house panel.
       Never call this house-only lookup later from global input callbacks. */
    iq.manager_ref=iq_reference(((void*(__cdecl*)(void*))(void*)(game_base+0xed390))(entity));
    if(!iq_reference_valid(iq.manager_ref))return;
    int n=iq_int(owner,0x64);
    void** array=iq_ptr(owner,0x68);
    if (n<0 || n>IQ_MAX_DRAWERS || !array) return;
    /* Read ref_grid directly. Trash background arrays run bottom-up while
       Storage runs top-down, so the first background is not a shared origin. */
    for (int side=0;side<2;side++) {
        int columns=iq_int(owner,0x50+side*4);
        int backgrounds=iq_int(owner,side?0x84:0x74);
        void** cells=iq_ptr(owner,side?0x88:0x78);
        if (columns<2 || columns>20 || backgrounds!=columns*columns || !cells) return;
        int cols=columns<IQ_COLUMNS?columns:IQ_COLUMNS;
        iq.columns[side]=cols;
        void *first=NULL;
        if (!iq_read(cells,&first,8)) return;
        void* t0=iq_ptr(first,0x40);
        struct {char data[16];uint64_t size,capacity;} key={{0},8,15};
        memcpy(key.data,"ref_grid",9);
        double bounds[4]={0};
        ((void*(__cdecl*)(void*,double*,void*))(void*)(game_base+0x97cb30))
            (iq_ptr(iq.panels[side],0x50),bounds,&key);
        double width=bounds[1]-bounds[0];
        if (!isfinite(width) || width<=0 || width>10000) return;
        iq.left[side]=bounds[0];
        iq.bottom[side]=bounds[2];
        iq.span[side]=width;
        iq.scale[side]=iq_double(t0,0x98)*columns/cols;
        /* Reuse existing backgrounds; never allocate or release game objects. */
        for(int j=0;j<backgrounds;j++) {
            void* renderer=NULL;
            if(!iq_read(cells+j,&renderer,8)) return;
            void* transform=iq_ptr(renderer,0x40);
            if(!transform) return;
            IQBackground* b=&iq.backgrounds[iq.background_count++];
            b->renderer=renderer;b->transform=transform;
            b->renderer_generation=iq_generation(renderer);
            b->transform_generation=iq_generation(transform);
            b->side=side;b->ordinal=j;
            void* track=iq_tracking(iq.panels[side],transform,&b->tracking_index);
            if(!track) return;
            b->offset_x=iq_double(track,0x10);b->offset_y=iq_double(track,0x18);
            b->x=iq_double(transform,0x80);b->y=iq_double(transform,0x88);
            b->sx=iq_double(transform,0x98);b->sy=iq_double(transform,0xa0);
            iq_read((unsigned char*)renderer+0x51,&b->visible,1);
        }
    }
    for (int i=0;i<n;i++) {
        void* drawer=NULL;
        if (!iq_read(array+i,&drawer,8)) return;
        void* panel=iq_ptr(drawer,0x48);
        int side=panel==iq.panels[0]?0:panel==iq.panels[1]?1:-1;
        if (side<0) continue;
        IQDrawer* d=&iq.drawers[iq.count++];
        d->drawer=drawer;
        d->reference=iq_reference(drawer);
        if(!iq_read((unsigned char*)drawer+0x68,&d->item_id,8)) return;
        d->side=side;
        d->traits=iq_traits(drawer,d->item_id);
        d->rarity=iq_int(drawer,0xb4);
        d->ordinal=(iq_filter_match(active_filter,d->traits,d->rarity) && iq_extended_match(drawer,d->item_id,1))?iq.items[side]++:-1;
    }
    for(int side=0;side<2;side++) {
        int cols=iq.columns[side];
        int maxrow=iq_max_row(iq.items[side],cols);
        if(iq.row[side]>maxrow) iq.row[side]=maxrow;
    }
#undef iq
    iq=pending_view;
    iq_index_view();
    for(int i=0;i<iq.background_count;i++) {
        IQBackground* b=&iq.backgrounds[i];
        int side=b->side,j=b->ordinal,cols=iq.columns[side];
        int visible=j<cols*cols;
        *((unsigned char*)b->renderer+0x51)=(unsigned char)visible;
        if(visible) {
            double pitch=iq.span[side]/cols*(side==0?0.90:1.0);
            iq_write_double(b->transform,0x80,iq.left[side]+pitch*(j%cols+0.5));
            iq_write_double(b->transform,0x88,iq.bottom[side]+pitch*(cols-j/cols-0.5)-(side==0?iq.span[side]*0.072:0));
            double factor=(double)iq_int(owner,0x50+side*4)/cols*(side==0?0.90:1.0);
            iq_write_double(b->transform,0x98,b->sx*factor);
            iq_write_double(b->transform,0xa0,b->sy*factor);
            void* track=iq_tracking(iq.panels[side],b->transform,NULL);
            if(track) {
                iq_write_double(track,0x10,b->offset_x+iq_double(b->transform,0x80)-b->x);
                iq_write_double(track,0x18,b->offset_y+iq_double(b->transform,0x88)-b->y);
            }
        }
    }
    for(int i=0;i<iq.count;i++) iq_present(&iq.drawers[i]);
    iq_ui_capture();
    char message[180];
    snprintf(message,sizeof(message),"Layout test: %d/%d native drawers; six columns; wheel scroll enabled",iq.items[0],iq.items[1]);
    report(message);
}

static IQDrawer* iq_find(void* instance) {
    if(!layout_test)return NULL;
    int at=iq_index_get(&iq_drawer_index,instance);
    if(at<0 || at>=iq.count)return NULL;
    IQDrawer* d=&iq.drawers[at];
    if(d->drawer!=instance || !iq_view_live() || !iq_reference_valid(d->reference) ||
       iq_ptr(instance,0x38)!=iq.owner)return NULL;
    uint64_t id=0;
    if(!iq_read((unsigned char*)instance+(iq.equipment?0x58:0x68),&id,8)) return NULL;
    return d->item_id==id?d:NULL;
}

static void __cdecl iq_update(void* instance) {
    original_drawer_update(instance);
    IQDrawer* d=iq_find(instance);
    if(d) {
        if(!iq.last_update) report("Native drawer update callback observed.");
        iq.last_update=GetTickCount64(); iq_present(d);
        if(d==&iq.drawers[0]) iq_ui_update();
    }
}
static void __cdecl iq_click(void* instance) {
    IQDrawer* d=iq_find(instance);
    if(d && (filter_popup || !iq_visible(d))) return;
    original_item_click(instance);
}
static int iq_is_open(void) {
    if(!iq_view_live()) return 0;
    void* scene=iq.scene_ref.pointer;
    unsigned char covered=1;
    if(!scene || !iq_read((unsigned char*)scene+0x4da,&covered,1) || covered) return 0;
    if(!iq_read((unsigned char*)scene+0x4b0,&covered,1) || covered)return 0;
    if(iq.equipment)return iq_ptr(iq_ptr(iq.owner,0x18),8)==scene;
    if(iq_ptr(iq.panels[0],0x20)!=scene || !iq_ptr(iq.panels[0],0x18))return 0;
    void* active=iq_ptr(iq.manager_ref.pointer,0x58);
    return active==iq.panels[0] || active==iq.panels[1];
}
static void iq_scroll(int side,int delta) {
    int maxrow=iq_scroll_limit(side);
    int next=iq.row[side]+delta;
    if(next<0)next=0;
    if(next>maxrow)next=maxrow;
    if(next==iq.row[side])return;
    iq.row[side]=next;
    for(int i=0;i<iq.count;i++)if(iq.drawers[i].side==side)iq_present(&iq.drawers[i]);
    char message[100];snprintf(message,sizeof(message),"Scroll side=%d row=%d/%d",side,next,maxrow);report(message);
}
static unsigned char __cdecl iq_mouse(void* input,void* event) {
    if(layout_test && iq_ui_event(event)) return 0;
    static int wheel_reports;
    if(layout_test && iq_int(event,0)==1027 && wheel_reports++<20) {
        char message[220];
        snprintf(message,sizeof(message),"Wheel diagnostic: count=%d age=%llu fields=%x,%x,%x,%x,%x",iq.count,(unsigned long long)(GetTickCount64()-iq.last_update),iq_int(event,0x10),iq_int(event,0x14),iq_int(event,0x18),iq_int(event,0x1c),iq_int(event,0x20));
        report(message);
    }
    /* SDL_MOUSEWHEEL. Ignore dragging and events outside the focused game. */
    if(layout_test && iq.count &&
       iq_int(event,0)==1027 && !(GetAsyncKeyState(VK_LBUTTON)&0x8000)) {
        if(!iq_is_open()) { report("Wheel ignored: native inventory not active or covered."); return original_mouse_event(input,event); }
        HWND window=GetForegroundWindow(); DWORD pid=0;
        GetWindowThreadProcessId(window,&pid);
        if(pid==GetCurrentProcessId()) {
            int side=iq_wheel_side();
            if(side>=0) {
                /* This build's SDL event uses floating-point Y at 0x1c. */
                float wheel=0;
                iq_read((unsigned char*)event+0x1c,&wheel,4);
                if(!isfinite(wheel) || fabsf(wheel)>1000) return original_mouse_event(input,event);
                int delta=(int)wheel;
                if(!delta && wheel) delta=wheel>0?1:-1;
                int direction=iq_int(event,0x20);
                if(direction==1) delta=-delta;
                if(delta)iq_scroll(side,-delta);
            }
        }
    }
    return original_mouse_event(input,event);
}
