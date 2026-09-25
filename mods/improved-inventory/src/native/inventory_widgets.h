typedef struct {void* renderer;uint64_t generation;IQReference reference; wchar_t cached[12][200];void* feedback[2][18];int feedback_count;} IQControl;
enum { IQ_RARITY_COUNT=5, IQ_POPUP_COUNT=IQ_RARITY_COUNT+IQ_TYPE_COUNT+4 };
static const double iq_rarity_popup_scale=0.70;
static struct {
    void* owner;uint64_t generation;
    IQControl bars[IQ_TYPE_COUNT*IQ_RARITY_COUNT],popups[IQ_POPUP_COUNT],arrows[2][2],search[2],count,rarity_art[3],rarity_labels;
    double offset_x,offset_y,scale,grid_x[2],grid_y[2];
    int captured_click;
} iq_controls;
#include "inventory_set_rows.h"
static int iq_cursor,iq_select_all;
static unsigned char iq_captured_keys[512];
static void* iq_text_window;
static int iq_started_text;
static void iq_refilter(void);
static void iq_feedback_update(void);
static int iq_control_valid(IQControl* c);
static void iq_overlay_camera(IQControl* c);
static int iq_control_point(IQControl* c,double* output);
static double iq_control_units(void) {return iq.equipment?1.0:32.0;}
static int iq_popup_index(void) {
    return filter_popup==1?active_filter.rarity:filter_popup==2?IQ_RARITY_COUNT+active_filter.type:IQ_RARITY_COUNT+IQ_TYPE_COUNT+iq_search.set_mode;
}
enum { IQ_SOUND_SELECT, IQ_SOUND_RESET, IQ_SOUND_CLOSE, IQ_SOUND_CHECK, IQ_SOUND_UNCHECK };
static void iq_sound(int action) {
    static const char* events[]={"SelectorWidget_Button_Left_Click","Button_Settings_Back_Click",
        "CloseButton_Click","ToggleButton_On_Press","ToggleButton_Off_Press"};
    if(!iq_is_open() || !iq_ptr(iq.panels[0],0x18))return;
    void* audio=((void*(__cdecl*)(void*))(void*)(game_base+0x4a300))(iq.panels[0]);
    if(!audio)return;
    IQGameString event={{0},0,15};
    ((void*(__cdecl*)(void*,const char*,size_t))(void*)(game_base+0x520d0))(&event,events[action],strlen(events[action]));
    /* Native Play consumes its owned std::string. The normal SFX mixer applies. */
    ((void(__cdecl*)(void*,void*,double,double,double,unsigned char))(void*)(game_base+0x95b770))(audio,&event,1.0,1.0,0.0,0);
}
static void iq_control_layer(IQControl* c,int layer) {
    if(!c->renderer || iq_int(c->renderer,0x54)==layer)return;
    memcpy((unsigned char*)c->renderer+0x54,&layer,4);
    /* Match the native setter: changing the number alone leaves the render registry stale. */
    if(*((unsigned char*)c->renderer+0x11)) {
        void* notify=iq_ptr(iq_ptr(c->renderer,0),0x30);
        if(notify)((void(__cdecl*)(void*))notify)(c->renderer);
    }
}
static void iq_set_name(wchar_t* out,const wchar_t* name) {
    wcsncpy(out,name,159);out[159]=0;
    const wchar_t* suffixes[]={L" Set Bonus!",L" Set Bonus",L" set bonus!",L" set bonus"};
    for(int i=0;i<4;i++) {
        size_t n=wcslen(out),s=wcslen(suffixes[i]);
        if(n>=s && !_wcsicmp(out+n-s,suffixes[i])){out[n-s]=0;break;}
    }
}
static void iq_focus_set(int focus) {
    typedef void*(__cdecl *GetFocus)(void);
    typedef unsigned char(__cdecl *TextInput)(void*);
    HMODULE exe=GetModuleHandleW(NULL);
    GetFocus get=(GetFocus)(void*)GetProcAddress(exe,"SDL_GetKeyboardFocus");
    TextInput start=(TextInput)(void*)GetProcAddress(exe,"SDL_StartTextInput");
    TextInput stop=(TextInput)(void*)GetProcAddress(exe,"SDL_StopTextInput");
    TextInput active=(TextInput)(void*)GetProcAddress(exe,"SDL_TextInputActive");
    if(!get || !start || !stop || !active)return;
    if(!focus && iq_started_text && iq_text_window==get())stop(iq_text_window);
    if(focus && !iq_focus) {
        iq_text_window=get();iq_started_text=iq_text_window && !active(iq_text_window);
        if(iq_started_text && !start(iq_text_window)){iq_started_text=0;return;}
    }
    iq_focus=focus;
    if(!focus){iq_text_window=NULL;iq_started_text=0;}
    iq_cursor=(int)wcslen(focus==2?iq_set_query:iq_search.query);iq_select_all=0;
}
static int iq_control_valid(IQControl* c) {return iq_reference_valid(c->reference);}
static void iq_control_hide(IQControl* c) {if(iq_control_valid(c))*((unsigned char*)c->renderer+0x51)=0;}
static void iq_hide_all(void) {
    for(int i=0;i<IQ_TYPE_COUNT*IQ_RARITY_COUNT;i++)iq_control_hide(&iq_controls.bars[i]);
    for(int i=0;i<IQ_POPUP_COUNT;i++)iq_control_hide(&iq_controls.popups[i]);
    for(int i=0;i<2;i++){iq_control_hide(&iq_controls.search[i]);for(int j=0;j<2;j++)iq_control_hide(&iq_controls.arrows[i][j]);}
    iq_control_hide(&iq_controls.count);
    for(int i=0;i<3;i++)iq_control_hide(&iq_controls.rarity_art[i]);
    iq_control_hide(&iq_controls.rarity_labels);
}
static void iq_ui_capture(void) {
    if(iq_controls.owner!=iq.owner || iq_controls.generation!=iq_generation(iq.owner)) {
        iq_hide_all();iq_focus_set(0);memset(&iq_controls,0,sizeof(iq_controls));
        iq_controls.owner=iq.owner;iq_controls.generation=iq_generation(iq.owner);filter_popup=0;
    }
    void* t=iq_panel_transform(0);
    iq_controls.offset_x=iq.left[0]-iq_double(t,0x80);
    iq_controls.offset_y=iq.bottom[0]+iq.span[0]-iq_double(t,0x88);
    iq_controls.scale=iq.span[0]/600.0;
    for(int side=0;side<2;side++) {
        t=iq_panel_transform(side);
        iq_controls.grid_x[side]=iq.left[side]-iq_double(t,0x80);
        iq_controls.grid_y[side]=iq.bottom[side]-iq_double(t,0x88);
    }
}
static void iq_control_show(IQControl* c,const char* name,double x,double y) {
    if(!iq_control_valid(c)) {
        void* scene=iq.scene_ref.pointer;
        void* entity=((void*(__cdecl*)(void*))(void*)(game_base+0x96b3e0))(scene);
        if(!entity)return;
        c->renderer=original_renderer(scene,entity,name);if(!c->renderer)return;
        c->reference=iq_reference(c->renderer);
        c->generation=iq_generation(c->renderer);memset(c->cached,0,sizeof(c->cached));
        c->feedback_count=0;memset(c->feedback,0,sizeof(c->feedback));
        int popup=!strncmp(name,"IQRarity",8) || !strncmp(name,"IQType",6) || !strncmp(name,"IQSets",6);
        iq_control_layer(c,popup?40:39);
        char msg[100];snprintf(msg,sizeof(msg),"Control %s created",name);report(msg);
    }
    /* Native movie children can become available on the following frame.
       Resolve feedback lazily, just like dynamic text fields. */
    if(!c->feedback_count && (!strncmp(name,"IQBar",5) || !strncmp(name,"IQSearch",8) ||
       !strncmp(name,"IQType",6) || !strncmp(name,"IQSets",6) ||
       (!strncmp(name,"IQRarity",8) && strcmp(name,"IQRarityLabels")) ||
       !strcmp(name,"IQUp") || !strcmp(name,"IQDown"))) {
        void* clip=iq_ptr(c->renderer,0x80);
        for(int i=0;clip && i<18;i++) {
            for(int pressed=0;pressed<2;pressed++) {
                char child[24];snprintf(child,sizeof(child),"fx%d_%d",pressed,i);
                IQGameString key=iq_borrow(child);
                c->feedback[pressed][i]=((void*(__cdecl*)(void*,void*))(void*)(game_base+0x99a0e0))(clip,&key);
            }
            if(!c->feedback[0][i] || !c->feedback[1][i])break;
            c->feedback_count++;
        }
    }
    void* t=iq_ptr(c->renderer,0x40);if(!t)return;
    if(iq.equipment)y=2*iq.bottom[0]+iq.span[0]-y;
    iq_write_double(t,0x80,x);iq_write_double(t,0x88,y);
    iq_write_double(t,0x98,iq_controls.scale*iq_control_units());iq_write_double(t,0xa0,iq_controls.scale*iq_control_units());
    for(int i=0;i<c->feedback_count;i++)for(int p=0;p<2;p++)*((unsigned char*)c->feedback[p][i]+8)&=(unsigned char)~0x20;
    *((unsigned char*)c->renderer+0x51)=1;
}
static void iq_rarity_art(double x,double y,double s) {
    static const char* labels[]={"uncommon","rare","very_rare"};
    for(int i=0;i<3;i++) {
        IQControl* art=&iq_controls.rarity_art[i];
        if(filter_popup!=1){iq_control_hide(art);continue;}
        int tile=i+1,col=tile%2,row=tile/2;
        /* Native rarity symbols have different local registration points. */
        double symbol_x=iq.equipment?0:(i==1?4:14);
        double symbol_y=iq.equipment?0:(i==0?9:18);
        double rs=s*iq_rarity_popup_scale;
        iq_control_show(art,"HeadItemIcon",x+238*s+(85+col*160+symbol_x)*rs,y-82*s-(126+row*160+75+symbol_y)*rs);
        if(!iq_control_valid(art))continue;
        iq_control_layer(art,41);
        void* t=iq_ptr(art->renderer,0x40);
        iq_write_double(t,0x98,rs*iq_control_units()*0.85);iq_write_double(t,0xa0,rs*iq_control_units()*0.85);
        iq_overlay_camera(art);
        if(art->cached[0][0])continue;
        void* clip=iq_ptr(art->renderer,0x80);
        if(!clip)continue;
        /* Installed catparts.swf: HeadItemIcon frame 4 has no item drawing.
           Preserve its native rarity child and hide the separate slot badge. */
        ((void(__cdecl*)(void*,int))(void*)(game_base+0x9a88a0))(clip,3);
        *((unsigned char*)clip+9)&=(unsigned char)~2;
        *((unsigned char*)clip+8)|=0x20;
        IQGameString key=iq_borrow("sloticon");
        void* slot=((void*(__cdecl*)(void*,void*))(void*)(game_base+0x99a0e0))(clip,&key);
        if(slot)*((unsigned char*)slot+8)&=(unsigned char)~0x20;
        key=iq_borrow("rarity");
        void* rarity=((void*(__cdecl*)(void*,void*))(void*)(game_base+0x99a0e0))(clip,&key);
        if(!rarity){iq_control_hide(art);continue;}
        /* Native GotoAndStop consumes this short inline string. */
        key=iq_borrow(labels[i]);
        ((void(__cdecl*)(void*,void*))(void*)(game_base+0x9a8c00))(rarity,&key);
        /* Native draw (9B1000) requires both 0x20 and 0x40. The item
           preview setup clears 0x20 to hide rarity; our tiles show it. */
        *((unsigned char*)rarity+8)|=0x20;
        art->cached[0][0]=L'1';
        char msg[100];snprintf(msg,sizeof(msg),"Native rarity artwork ready: %s",labels[i]);report(msg);
    }
    IQControl* labels_control=&iq_controls.rarity_labels;
    if(filter_popup!=1){iq_control_hide(labels_control);return;}
    iq_control_show(labels_control,"IQRarityLabels",x+238*s,y-82*s);
    iq_control_layer(labels_control,42);
    void* t=iq_ptr(labels_control->renderer,0x40);
    iq_write_double(t,0x98,s*iq_control_units()*iq_rarity_popup_scale);
    iq_write_double(t,0xa0,s*iq_control_units()*iq_rarity_popup_scale);
    iq_overlay_camera(labels_control);
}
/* Layers 35..40 and 41..46 use different native cameras. Convert overlay
   placement from the inventory camera, including its zoom and translation. */
static void* iq_layer_camera(void* renderer,int layer) {
    void* scene=iq_ptr(iq_ptr(renderer,0x18),8);
    void* cameras=iq_ptr(iq_ptr(scene,0x20),0x70);
    int count=iq_int(cameras,0xc);void* entries=iq_ptr(cameras,0x10);
    if(count<0 || count>64)return NULL;
    for(int i=0;i<count;i++) {
        void* camera=iq_ptr(entries,(size_t)i*8);
        void* mask=iq_ptr(camera,0xc8);
        if(mask && ((unsigned)iq_int(mask,(size_t)(layer/32)*4)&(1u<<(layer%32))))return camera;
    }
    return NULL;
}
/* Use the same camera and current panel transform as the visible grid. */
static int iq_wheel_side(void) {
    double point[2];
    if(!iq_control_point(&iq_controls.search[iq_focus==1],point))return -1;
    double s=iq_controls.scale;
    if(!isfinite(s) || s<=0)return -1;
    for(int side=0;side<(iq.equipment?1:2);side++) {
        double x=0,top=103.2,width=540,height=width*iq_rows(side)/iq.columns[side];
        if(side) {
            void* first=iq_panel_transform(0);void* other=iq_panel_transform(side);
            x=(iq_double(other,0x80)+iq_controls.grid_x[side]-
               iq_double(first,0x80)-iq_controls.offset_x)/s;
            top=(iq_double(first,0x88)+iq_controls.offset_y-
                 iq_double(other,0x88)-iq_controls.grid_y[side]-iq.span[side])/s;
            width=height=iq.span[side]/s;
        }
        if(point[0]>=x && point[0]<x+width && point[1]>=top && point[1]<top+height)return side;
    }
    return -1;
}
static void iq_overlay_camera(IQControl* c) {
    if(!iq_control_valid(c))return;
    void* from=iq_ptr(iq_layer_camera(c->renderer,40),0x38);
    void* to=iq_ptr(iq_layer_camera(c->renderer,iq_int(c->renderer,0x54)),0x38);
    void* t=iq_ptr(c->renderer,0x40);
    if(!from || !to || !t || from==to)return;
    double sx=iq_double(from,0x98),sy=iq_double(from,0xa0);
    if(!isfinite(sx) || !isfinite(sy) || fabs(sx)<1e-9 || fabs(sy)<1e-9)return;
    double rx=iq_double(to,0x98)/sx,ry=iq_double(to,0xa0)/sy;
    iq_write_double(t,0x80,iq_double(to,0x80)+(iq_double(t,0x80)-iq_double(from,0x80))*rx);
    iq_write_double(t,0x88,iq_double(to,0x88)+(iq_double(t,0x88)-iq_double(from,0x88))*ry);
    iq_write_double(t,0x98,iq_double(t,0x98)*rx);
    iq_write_double(t,0xa0,iq_double(t,0xa0)*ry);
}
static void iq_text(IQControl* c,int slot,const char* field,const wchar_t* text) {
    if(!iq_control_valid(c) || !wcscmp(c->cached[slot],text))return;
    void* clip=iq_ptr(c->renderer,0x80);IQGameString key=iq_borrow(field);
    void* child=((void*(__cdecl*)(void*,void*))(void*)(game_base+0x99a0e0))(clip,&key);
    if(!child)return;
    IQGameString value={{0},0,7};
    ((void*(__cdecl*)(void*,const wchar_t*,size_t))(void*)(game_base+0x5b150))(&value,text,wcslen(text));
    /* SetText takes ownership of the wstring and destroys it before returning. */
    ((void(__cdecl*)(void*,void*,unsigned char,unsigned char))(void*)(game_base+0x98e8a0))(child,&value,0,0);
    wcsncpy(c->cached[slot],text,199);c->cached[slot][199]=0;
}
static void iq_query_label(wchar_t* label,const wchar_t* query,const wchar_t* placeholder,int focus) {
    if(!focus){swprintf(label,200,L"%ls",*query?query:placeholder);return;}
    /* Keep the caret visible when the query exceeds the field width. */
    int start=iq_cursor>35?iq_cursor-35:0;
    if(iq_select_all)swprintf(label,200,L"> %ls <",query+start);
    else swprintf(label,200,L"%.*ls|%.35ls",iq_cursor-start,query+start,query+iq_cursor);
}
static void iq_ui_update(void) {
    if(!iq_is_open()){iq_hide_all();filter_popup=0;if(iq_focus)iq_focus_set(0);return;}
    int selected=active_filter.type*IQ_RARITY_COUNT+active_filter.rarity;
    for(int i=0;i<IQ_TYPE_COUNT*IQ_RARITY_COUNT;i++)if(i!=selected)iq_control_hide(&iq_controls.bars[i]);
    for(int i=0;i<IQ_POPUP_COUNT;i++)if(!filter_popup || i!=iq_popup_index())iq_control_hide(&iq_controls.popups[i]);
    for(int side=0;side<2;side++)for(int dir=0;dir<2;dir++) {
        IQControl* arrow=&iq_controls.arrows[side][dir];
        int maxrow=side && iq.equipment?0:iq_scroll_limit(side);
        if(!(dir?iq.row[side]<maxrow:iq.row[side]>0)){iq_control_hide(arrow);continue;}
        void* t=iq_panel_transform(side);
        double ax=iq_double(t,0x80)+iq_controls.grid_x[side]+(side?-38*iq_controls.scale:iq.span[side]*0.9+6*iq_controls.scale);
        double ay=iq_double(t,0x88)+iq_controls.grid_y[side]+iq.span[side]*(dir?0.48:0.58);
        iq_control_show(arrow,dir?"IQDown":"IQUp",ax,ay);
    }
    void* t=iq_panel_transform(0);
    double x=iq_double(t,0x80)+iq_controls.offset_x,y=iq_double(t,0x88)+iq_controls.offset_y,s=iq_controls.scale;
    char name[24];wchar_t label[200];
    int focused=iq_focus==1;iq_control_hide(&iq_controls.search[!focused]);
    snprintf(name,sizeof(name),"IQSearch%d",focused);iq_control_show(&iq_controls.search[focused],name,x,y);
    iq_query_label(label,iq_search.query,L"Search name, description, set...",focused);iq_text(&iq_controls.search[focused],0,"query",label);
    snprintf(name,sizeof(name),"IQBar%d%d",active_filter.type,active_filter.rarity);
    iq_control_show(&iq_controls.bars[selected],name,x,y-42*s);
    int selected_sets=0,last=0;
    for(int j=0;j<iq_set_count;j++)if(iq_search.sets[j/64]&(UINT64_C(1)<<(j%64))){selected_sets++;last=j;}
    if(iq_search.set_mode==3 && selected_sets==1){iq_set_name(label,iq_sets[last].name);if(wcslen(label)>6)wcscpy(label+4,L"..");}
    else if(iq_search.set_mode==3)swprintf(label,200,L"%d sets",selected_sets);
    else swprintf(label,200,L"%ls",iq_search.set_mode==1?L"Any set":iq_search.set_mode==2?L"No set":L"All sets");
    iq_text(&iq_controls.bars[selected],0,"sets",label);
    iq_control_show(&iq_controls.count,"IQCount",x+12*s,y-80*s);
    if(iq.equipment)swprintf(label,200,L"Storage: %d%ls",iq.items[0],iq.items[0]==0?L"    No matches":L"");
    else swprintf(label,200,L"Storage: %d    Trash: %d%ls",iq.items[0],iq.items[1],iq.items[0]+iq.items[1]==0?L"    No matches":L"");
    iq_text(&iq_controls.count,0,"count",label);
    if(filter_popup==1 || filter_popup==2) {
        snprintf(name,sizeof(name),filter_popup==1?"IQRarity%d":"IQType%d",filter_popup==1?active_filter.rarity:active_filter.type);
        iq_control_show(&iq_controls.popups[iq_popup_index()],name,x+(filter_popup==1?238:0)*s,y-82*s);
        if(filter_popup==1) {
            void* t=iq_ptr(iq_controls.popups[iq_popup_index()].renderer,0x40);
            iq_write_double(t,0x98,s*iq_control_units()*iq_rarity_popup_scale);
            iq_write_double(t,0xa0,s*iq_control_units()*iq_rarity_popup_scale);
        }
    }
    if(filter_popup==3) {
        snprintf(name,sizeof(name),"IQSets%d",iq_search.set_mode);
        IQControl* popup=&iq_controls.popups[iq_popup_index()];iq_control_show(popup,name,x+211*s,y-82*s);iq_build_set_rows();
        iq_query_label(label,iq_set_query,L"Find a set...",iq_focus==2);iq_text(popup,0,"query",label);
        for(int row=0;row<9;row++) {
            int at=iq_set_offset+row;label[0]=0;
            if(at<iq_set_rows_count) {
                int j=iq_set_rows[at];int checked=iq_search.set_mode==3 && (iq_search.sets[j/64]&(UINT64_C(1)<<(j%64)))!=0;
                wchar_t short_name[160];iq_set_name(short_name,iq_sets[j].name);
                swprintf(label,200,L"[%lc] %.33ls (%d)",checked?L'x':L' ',short_name,iq_set_counts[j+3]);
            } else if(!row)wcscpy(label,L"No matching sets");
            snprintf(name,sizeof(name),"row%d",row);iq_text(popup,row+1,name,label);
        }
    }
    if(filter_popup==3)for(int dir=0;dir<2;dir++) {
        void* clip=iq_ptr(iq_controls.popups[iq_popup_index()].renderer,0x80);
        IQGameString key=iq_borrow(dir?"disabled1":"disabled0");
        void* child=clip?((void*(__cdecl*)(void*,void*))(void*)(game_base+0x99a0e0))(clip,&key):NULL;
        if(child) {
            int disabled=dir?iq_set_offset+9>=iq_set_rows_count:iq_set_offset==0;
            if(disabled)*((unsigned char*)child+8)|=0x20;
            else *((unsigned char*)child+8)&=(unsigned char)~0x20;
        }
    }
    iq_rarity_art(x,y,s);
    iq_feedback_update();
}
static void iq_refilter(void) {
    memset(iq.items,0,sizeof(iq.items));memset(iq.row,0,sizeof(iq.row));
    for(int i=0;i<iq.count;i++) {
        IQDrawer* d=&iq.drawers[i];
        d->ordinal=iq_filter_match(active_filter,d->traits,d->rarity) && iq_extended_match(d->drawer,d->item_id,1)?iq.items[d->side]++:-1;
        iq_present(d);
    }
    char msg[180];snprintf(msg,sizeof(msg),"Filter type=%d rarity=%d queryLength=%zu setMode=%d results=%d/%d",active_filter.type,active_filter.rarity,wcslen(iq_search.query),iq_search.set_mode,iq.items[0],iq.items[1]);report(msg);
    iq_ui_update();
}
static int iq_control_point(IQControl* c,double* output) {
    if(!iq_control_valid(c) || !iq_camera || iq_generation(iq_camera)!=iq_camera_generation)return 0;
    double point[2];original_mouse_position(iq_camera,point);
    void* clip=iq_ptr(c->renderer,0x80);
    ((void(__cdecl*)(void*,double*,double*,void*,double))(void*)(game_base+0x97b130))(c->renderer,output,point,clip,1.0);
    return isfinite(output[0])&&isfinite(output[1]);
}
/* Feedback uses the same local rectangles as input, including popup modality. */
typedef struct { IQControl* control; double point[2]; int valid; } IQHoverSample;
static int iq_feedback_rect(IQHoverSample* sample,IQControl* control,int index,double x,double y,double w,double h) {
    /* Share the inverse transform across a control's options, only this update. */
    if(sample->control!=control) {
        sample->control=control;
        sample->valid=iq_control_valid(control) && *((unsigned char*)control->renderer+0x51) &&
            iq_control_point(control,sample->point);
    }
    double* p=sample->point;
    if(!sample->valid || p[0]<x || p[0]>=x+w || p[1]<y || p[1]>=y+h)return 0;
    int pressed=iq_controls.captured_click && (GetAsyncKeyState(VK_LBUTTON)&0x8000);
    if(index<control->feedback_count)*((unsigned char*)control->feedback[pressed][index]+8)|=0x20;
    return 1;
}
static void iq_feedback_update(void) {
    IQHoverSample sample={0};
    DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);if(pid!=GetCurrentProcessId())return;
    if(filter_popup) {
        IQControl* popup=&iq_controls.popups[iq_popup_index()];
        if(filter_popup==1) {
            if(iq_feedback_rect(&sample,popup,0,10,61,310,36/0.70))return;
            for(int i=0;i<4;i++)if(iq_feedback_rect(&sample,popup,i+1,10+(i%2)*160,126+(i/2)*160,150,150))return;
        } else if(filter_popup==2) {
            for(int i=0;i<IQ_TYPE_COUNT;i++)if(iq_feedback_rect(&sample,popup,i,10,43+i*43,260,36))return;
        } else {
            for(int i=0;i<3;i++)if(iq_feedback_rect(&sample,popup,i,10+i*115,38,110,36))return;
            if(iq_feedback_rect(&sample,popup,3,316,82,34,36) || iq_feedback_rect(&sample,popup,4,10,82,306,36))return;
            for(int i=0;i<9 && iq_set_offset+i<iq_set_rows_count;i++)if(iq_feedback_rect(&sample,popup,5+i,10,126+i*33,340,31))return;
            if(iq_set_offset>0 && iq_feedback_rect(&sample,popup,14,10,429,62,36))return;
            if(iq_set_offset+9<iq_set_rows_count && iq_feedback_rect(&sample,popup,15,80,429,77,36))return;
            if(iq_feedback_rect(&sample,popup,16,166,429,78,36) || iq_feedback_rect(&sample,popup,17,253,429,97,36))return;
        }
        return;
    }
    IQControl* search=&iq_controls.search[iq_focus==1];
    if(iq_feedback_rect(&sample,search,0,533,0,38,36))return;
    IQControl* bar=&iq_controls.bars[active_filter.type*IQ_RARITY_COUNT+active_filter.rarity];
    const double fields[][2]={{0,230},{238,135},{381,114},{503,68}};
    for(int i=0;i<4;i++)if(iq_feedback_rect(&sample,bar,i,fields[i][0],0,fields[i][1],36))return;
    for(int side=0;side<2;side++)for(int dir=0;dir<2;dir++)if(iq_feedback_rect(&sample,&iq_controls.arrows[side][dir],0,0,0,32,36))return;
}
static void iq_edit_insert(const wchar_t* text) {
    wchar_t* q=iq_focus==2?iq_set_query:iq_search.query;
    if(iq_select_all){q[0]=0;iq_cursor=0;iq_select_all=0;}
    size_t n=wcslen(q);
    for(;*text && n+1<IQ_QUERY_LIMIT;text++) {
        if(*text<32)continue;
        memmove(q+iq_cursor+1,q+iq_cursor,(n-(size_t)iq_cursor+1)*sizeof(wchar_t));q[iq_cursor++]=*text;n++;
    }
    iq_set_offset=0;if(iq_focus==1)iq_refilter();else iq_ui_update();
}
static int iq_keyboard(void* event,int type) {
    if(type==769){int sc=iq_int(event,0x18);if(sc>=0 && sc<512 && iq_captured_keys[sc]){iq_captured_keys[sc]=0;return 1;}}
    if(!iq_focus)return 0;
    if(type==771) {
        const char* text=iq_ptr(event,0x18);char utf8[512]={0};int n=0;
        while(n<511 && iq_read(text+n,utf8+n,1) && utf8[n])n++;
        wchar_t wide[512];int count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8,n,wide,511);
        if(count>0){wide[count]=0;iq_edit_insert(wide);}return 1;
    }
    if(type==770 || type==769)return 1;
    if(type!=768)return 0;
    int sc=iq_int(event,0x18);if(sc>=0 && sc<512)iq_captured_keys[sc]=1;
    wchar_t* q=iq_focus==2?iq_set_query:iq_search.query;int n=(int)wcslen(q);
    unsigned short mod=0;iq_read((unsigned char*)event+0x20,&mod,2);
    if(sc==41 || sc==40){iq_sound(IQ_SOUND_CLOSE);iq_focus_set(0);if(sc==41)filter_popup=0;iq_ui_update();return 1;}
    if(sc==4 && (mod&0xc0)){iq_select_all=1;iq_ui_update();return 1;}
    if(sc==25 && (mod&0xc0)) {
        HMODULE exe=GetModuleHandleW(NULL);
        char*(__cdecl *get)(void)=(void*)GetProcAddress(exe,"SDL_GetClipboardText");
        void(__cdecl *release)(void*)=(void*)GetProcAddress(exe,"SDL_free");
        if(get && release){char* t=get();wchar_t w[IQ_QUERY_LIMIT];if(t){int len=(int)strlen(t);if(len>380)len=380;int count=MultiByteToWideChar(CP_UTF8,0,t,len,w,IQ_QUERY_LIMIT-1);if(count>0){w[count]=0;iq_edit_insert(w);}release(t);}}
        return 1;
    }
    if(sc==42 || sc==76) {
        if(iq_select_all){q[0]=0;iq_cursor=0;iq_select_all=0;}
        else if(sc==42 && iq_cursor>0){memmove(q+iq_cursor-1,q+iq_cursor,(size_t)(n-iq_cursor+1)*2);iq_cursor--;}
        else if(sc==76 && iq_cursor<n)memmove(q+iq_cursor,q+iq_cursor+1,(size_t)(n-iq_cursor)*2);
        iq_set_offset=0;if(iq_focus==1)iq_refilter();else iq_ui_update();return 1;
    }
    if(sc==80 && iq_cursor>0)iq_cursor--;else if(sc==79 && iq_cursor<n)iq_cursor++;
    else if(sc==74)iq_cursor=0;else if(sc==77)iq_cursor=n;
    if(sc==80 || sc==79 || sc==74 || sc==77)iq_select_all=0;
    iq_ui_update();return 1;
}
static int iq_ui_event(void* event) {
    int type=iq_int(event,0);
    if(type==1026 && iq_controls.captured_click){iq_controls.captured_click=0;return 1;}
    if(!iq_is_open()){iq_hide_all();filter_popup=0;if(iq_focus)iq_focus_set(0);return 0;}
    if(iq_keyboard(event,type))return 1;
    if(type==768 && filter_popup && iq_int(event,0x18)==41){iq_sound(IQ_SOUND_CLOSE);filter_popup=0;iq_ui_update();return 1;}
    if(type==1027 && filter_popup) {
        if(filter_popup==3){float wheel=0;iq_read((unsigned char*)event+0x1c,&wheel,4);if(isfinite(wheel)&&wheel){int delta=wheel>0?-3:3;if(iq_int(event,0x20)==1)delta=-delta;iq_set_offset+=delta;iq_ui_update();}}
        return 1;
    }
    if(type!=1025)return 0;
    unsigned char button=0;iq_read((unsigned char*)event+0x18,&button,1);double p[2];
    if(filter_popup==3) {
        iq_controls.captured_click=1;
        if(button!=1 || !iq_control_point(&iq_controls.popups[iq_popup_index()],p) || p[0]<0 || p[0]>=360 || p[1]<0 || p[1]>=479){iq_sound(IQ_SOUND_CLOSE);filter_popup=0;iq_focus_set(0);iq_ui_update();return 1;}
        if(p[0]>=10 && p[0]<350 && p[1]>=82 && p[1]<118){if(p[0]>=316){iq_sound(IQ_SOUND_RESET);iq_set_query[0]=0;iq_set_offset=0;}iq_focus_set(2);iq_ui_update();return 1;}
        iq_focus_set(0);
        if(p[0]>=10 && p[0]<350 && p[1]>=38 && p[1]<74) {
            int mode=(int)((p[0]-10)/115);
            if(p[0]-(10+mode*115)<110){iq_sound(IQ_SOUND_SELECT);iq_search.set_mode=mode;memset(iq_search.sets,0,sizeof(iq_search.sets));iq_refilter();}
            return 1;
        }
        if(p[0]>=10 && p[0]<350 && p[1]>=126 && p[1]<423) {
            int at=iq_set_offset+(int)((p[1]-126)/33);
            if(at<iq_set_rows_count){int j=iq_set_rows[at];iq_search.set_mode=3;iq_search.sets[j/64]^=UINT64_C(1)<<(j%64);iq_sound((iq_search.sets[j/64]&(UINT64_C(1)<<(j%64)))?IQ_SOUND_CHECK:IQ_SOUND_UNCHECK);if(!(iq_search.sets[0]|iq_search.sets[1]|iq_search.sets[2]|iq_search.sets[3]))iq_search.set_mode=0;}
            iq_refilter();return 1;
        }
        if(p[1]>=429 && p[1]<465) {
            if(p[0]>=10 && p[0]<72 && iq_set_offset>0){iq_sound(IQ_SOUND_SELECT);iq_set_offset-=9;}
            else if(p[0]>=80 && p[0]<157 && iq_set_offset+9<iq_set_rows_count){iq_sound(IQ_SOUND_SELECT);iq_set_offset+=9;}
            else if(p[0]>=166 && p[0]<244){iq_sound(IQ_SOUND_RESET);iq_search.set_mode=0;memset(iq_search.sets,0,sizeof(iq_search.sets));iq_set_query[0]=0;iq_set_offset=0;iq_refilter();}
            else if(p[0]>=253 && p[0]<350){iq_sound(IQ_SOUND_CLOSE);filter_popup=0;}
        }
        iq_ui_update();return 1;
    }
    if(filter_popup) {
        IQFilter previous=active_filter;int choice=0;iq_controls.captured_click=1;
        if(button==1 && iq_control_point(&iq_controls.popups[iq_popup_index()],p)) {
            if(filter_popup==1){int index=iq_rarity_choice(p[0],p[1]);if(index>=0){active_filter.rarity=index;choice=1;}}
            else if(filter_popup==2 && p[0]>=10 && p[0]<270 && p[1]>=43 && p[1]<301){int row=(int)((p[1]-43)/43);if((p[1]-43)-row*43<36 && row<IQ_TYPE_COUNT){active_filter.type=row;choice=1;}}
        }
        iq_sound(choice?IQ_SOUND_SELECT:IQ_SOUND_CLOSE);filter_popup=0;if(previous.type!=active_filter.type || previous.rarity!=active_filter.rarity)iq_refilter();else iq_ui_update();return 1;
    }
    if(button!=1){if(iq_focus){iq_focus_set(0);iq_controls.captured_click=1;return 1;}return 0;}
    if(iq_control_point(&iq_controls.search[iq_focus==1],p) && p[0]>=0 && p[0]<571 && p[1]>=0 && p[1]<36) {
        iq_controls.captured_click=1;
        if(p[0]>=533){iq_sound(IQ_SOUND_RESET);iq_search.query[0]=0;iq_focus_set(0);iq_refilter();}else {if(iq_focus!=1)iq_sound(IQ_SOUND_SELECT);iq_focus_set(1);iq_ui_update();}return 1;
    }
    int selected=active_filter.type*IQ_RARITY_COUNT+active_filter.rarity;
    if(iq_control_point(&iq_controls.bars[selected],p) && p[1]>=0 && p[1]<36 && p[0]>=0 && p[0]<571) {
        iq_focus_set(0);iq_controls.captured_click=1;
        if(p[0]<230)filter_popup=2;else if(p[0]>=238 && p[0]<373)filter_popup=1;
        else if(p[0]>=381 && p[0]<495){filter_popup=3;iq_set_offset=0;}
        else if(p[0]>=503){iq_sound(IQ_SOUND_RESET);active_filter=(IQFilter){0,0};memset(&iq_search,0,sizeof(iq_search));iq_set_query[0]=0;iq_refilter();}
        if(filter_popup)iq_sound(IQ_SOUND_SELECT);iq_ui_update();return 1;
    }
    if(iq_focus){iq_focus_set(0);iq_controls.captured_click=1;iq_ui_update();return 1;}
    for(int side=0;side<2;side++)for(int dir=0;dir<2;dir++) {
        int maxrow=side && iq.equipment?0:iq_scroll_limit(side);if(!(dir?iq.row[side]<maxrow:iq.row[side]>0))continue;
        if(iq_control_point(&iq_controls.arrows[side][dir],p) && p[0]>=0 && p[0]<32 && p[1]>=0 && p[1]<36){iq_controls.captured_click=1;iq_sound(IQ_SOUND_SELECT);iq_scroll(side,dir?1:-1);iq_ui_update();return 1;}
    }
    return 0;
}
