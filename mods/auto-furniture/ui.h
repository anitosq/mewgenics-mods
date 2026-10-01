/* Native scene rendering and event chain, using the game's existing controls. */
#include "ui-layout.h"
typedef unsigned char (*InputEvent)(void*,void*);
typedef unsigned char (*HitTest)(void*);
static InputEvent original_input;
static HitTest original_hit;
static Update original_furniture_hover;
static int ui_enabled;
typedef struct {
    byte* renderer;uint64_t generation;char text[4+6*AF_STATS][180];
    byte* feedback[3][24];int feedback_count,visible;
} Control;
static struct {
    void* owner;void* scene;uint64_t generation;int allowed,modal,action,focus,captured;
    unsigned selected;char minimum[AF_STATS][8],maximum[AF_STATS][8];char room[128];unsigned pin_page,pin_total;
    uint64_t pin_list[MAX_ITEMS];char pin_labels[MAX_ITEMS][128];
    byte* rooms[MAX_ROOMS];unsigned count;
    Control icons[MAX_ROOMS],panel,pins,pin_rows[9],hint,shade,stats[AF_STATS],result_stats[AF_STATS];
} ui_state={.focus=-1,.selected=AF_ALL_STATS};
static double number(void* p,size_t offset) {double n=0;if(p)read_bytes((byte*)p+offset,&n,8);return n;}
static uint64_t generation(void* p) {uint64_t n=0;if(p)read_bytes((byte*)p-8,&n,8);return n;}
static int control_valid(Control* c) {
    byte dead=1;
    return c->renderer&&generation(c->renderer)==c->generation&&read_bytes(c->renderer+15,&dead,1)&&!dead;
}
static void control_hide(Control* c) {
    if(!c->visible)return;
    c->visible=0;
    if(control_valid(c))c->renderer[0x51]=0;
}
static void* layer_camera(int layer) {
    byte* list=ptr(ptr(ui_state.scene,0x20),0x70);int n=integer(list,12);byte* data=ptr(list,16);
    if(n<0||n>64)return NULL;
    for(int i=0;i<n;i++) {
        byte* camera=ptr(data,(size_t)i*8);byte* mask=ptr(camera,0xc8);
        if(mask&&((unsigned)integer(mask,(size_t)(layer/32)*4)&(1u<<(layer%32))))return camera;
    }
    return NULL;
}
static void control_show(Control* c,const char* symbol,double x,double y,double scale,int layer) {
    if(!control_valid(c)) {
        memset(c,0,sizeof(*c));void* entity=FN(void*(*)(void*),0x96b3e0)(ui_state.scene);if(!entity)return;
        c->renderer=FN(byte*(*)(void*,void*,const char*),0x5a3d0)(ui_state.scene,entity,symbol);
        if(!c->renderer)return;c->generation=generation(c->renderer);
        memcpy(c->renderer+0x54,&layer,4);
        if(c->renderer[0x11]) {
            Update notify=(Update)ptr(ptr(c->renderer,0),0x30);if(notify)notify(c->renderer);
        }
    }
    byte* t=ptr(c->renderer,0x40);if(!t)return;
    memcpy(t+0x80,&x,8);memcpy(t+0x88,&y,8);scale*=32;
    memcpy(t+0x98,&scale,8);memcpy(t+0xa0,&scale,8);c->renderer[0x51]=1;c->visible=1;
    if(!c->feedback_count&&!strncmp(symbol,"AF",2)) {
        void* clip=ptr(c->renderer,0x80);
        for(int i=0;clip&&i<24;i++) {
            for(int state=0;state<3;state++) {
                char name[16];snprintf(name,16,"fx%d_%d",state,i);
                GameString key={{0},strlen(name),15};memcpy(key.data,name,key.size+1);
                c->feedback[state][i]=FN(byte*(*)(void*,void*),0x99a0e0)(clip,&key);
            }
            if(!c->feedback[0][i]||!c->feedback[1][i]||!c->feedback[2][i])break;
            c->feedback_count++;
        }
    }
    for(int i=0;i<c->feedback_count;i++)for(int state=0;state<3;state++)c->feedback[state][i][8]&=(byte)~0x20;
}
static void control_feedback(Control* c,int index,int state) {
    if(control_valid(c)&&index>=0&&index<c->feedback_count)c->feedback[state][index][8]|=0x20;
}
static int control_world(Control* c,double x,double y,double world[2]) {
    if(!control_valid(c))return 0;
    byte* clip=ptr(c->renderer,0x80);void* transform=ptr(c->renderer,0x40);
    if(!clip||!transform)return 0;
    float matrix[6];FN(void*(*)(void*,float*),0x9b1880)(clip,matrix);
    double local[2]={matrix[0]*x+matrix[3]*y+matrix[4],matrix[2]*x+matrix[1]*y+matrix[5]};
    FN(void(*)(void*,double*,double*),0x9b4f60)(transform,world,local);
    return isfinite(world[0])&&isfinite(world[1]);
}
static void control_stat(Control* c,int stat,double cx,double cy,double scale) {
    static const char* symbols[]={"FontIcon_comfort","FontIcon_stimulation","FontIcon_health","FontIcon_evolution","FontIcon_appeal"};
    /* Centers of the installed shapes, in SWF pixels. */
    static const double centers[AF_STATS][2]={{31.5,-31.3},{37.95,-30.975},{29.825,-31.6},{31.775,-31.925},{32.225,-31.6}};
    double target[2],center[2];
    if(!control_world(&ui_state.panel,cx,cy,target))return;
    control_show(c,symbols[stat],0,0,scale,42);
    if(control_world(c,centers[stat][0],centers[stat][1],center)) {
        byte* transform=ptr(c->renderer,0x40);
        double x=target[0]-center[0],y=target[1]-center[1];
        memcpy(transform+0x80,&x,8);memcpy(transform+0x88,&y,8);
    }
    if(control_valid(c)) {
        byte* clip=FN(byte*(*)(void*),0x97af30)(c->renderer);
        if(clip)FN(void(*)(void*,double,double,double,double),0x9bf090)(clip+0x78,0.0,0.0,0.0,1.0);
    }
}
static void control_text(Control* c,int slot,const char* name,const char* value) {
    if(!control_valid(c)||!strcmp(c->text[slot],value))return;
    GameString key={{0},strlen(name),15};if(key.size>15)return;memcpy(key.data,name,key.size+1);
    void* clip=ptr(c->renderer,0x80);if(!clip)return;
    void* child=FN(void*(*)(void*,void*),0x99a0e0)(clip,&key);if(!child)return;
    wchar_t wide[180];int n=MultiByteToWideChar(CP_UTF8,0,value,-1,wide,180);if(!n)return;
    GameString text={{0},0,7};FN(void*(*)(void*,const wchar_t*,size_t),0x5b150)(&text,wide,(size_t)n-1);
    FN(void(*)(void*,void*,byte,byte),0x98e8a0)(child,&text,0,0);
    snprintf(c->text[slot],180,"%s",value);
}
static int control_point(Control* c,double p[2]) {
    if(!control_valid(c)||!c->renderer[0x51])return 0;
    void* camera=layer_camera(integer(c->renderer,0x54));void* clip=ptr(c->renderer,0x80);
    if(!camera||!clip)return 0;
    double world[2];FN(double*(*)(void*,double*),0x9796d0)(camera,world);
    FN(void(*)(void*,double*,double*,void*,double),0x97b130)(c->renderer,p,world,clip,1.0);
    return isfinite(p[0])&&isfinite(p[1]);
}
static int inside(double p[2],double x,double y,double w,double h) {
    return p[0]>=x&&p[0]<x+w&&p[1]>=y&&p[1]<y+h;
}
static int ui_target(const UIBox* boxes,unsigned count,double p[2]) {
    for(unsigned i=0;i<count;i++)if(inside(p,boxes[i].x,boxes[i].y,boxes[i].w,boxes[i].h))return (int)i;
    return -1;
}
static int ui_available(int modal,int at) {
    if(modal==2)return at!=1&&at!=2?1:at==1?ui_state.pin_page>0:(ui_state.pin_page+1)*9<ui_state.pin_total;
    if(at>=6&&at<16)return !!(ui_state.selected&(1u<<((at-6)/2)));
    if(at==18)return !plan_worker&&ui_state.selected;
    if(at==19)return !plan_worker&&plan_ready&&af_within_caps(&problem,search.best.stats);
    if(at==20)return !plan_worker&&undo_ready;
    if(at>=21)return !plan_worker;
    return 1;
}
static int ui_live(void) {
    if(!ui_enabled||initialized!=2||!ui_state.allowed||!ui_state.owner||
       generation(ui_state.owner)!=ui_state.generation)return 0;
    byte covered=1,paused=1,editing=0;
    return read_bytes((byte*)ui_state.owner+0x78,&editing,1)&&editing&&
           read_bytes((byte*)ui_state.scene+0x4da,&covered,1)&&!covered&&
           read_bytes((byte*)ui_state.scene+0x4b0,&paused,1)&&!paused;
}
enum {UI_SELECT,UI_RESET,UI_CLOSE,UI_CHECK,UI_UNCHECK};
static void ui_sound(int action) {
    static const char* events[]={"SelectorWidget_Button_Left_Click","Button_Settings_Back_Click",
        "CloseButton_Click","ToggleButton_On_Press","ToggleButton_Off_Press"};
    if(!ui_live()||!ptr(ui_state.owner,0x18))return;
    void* audio=FN(void*(*)(void*),0x4a300)(ui_state.owner);if(!audio)return;
    GameString event={{0},0,15};
    FN(void*(*)(void*,const char*,size_t),0x520d0)(&event,events[action],strlen(events[action]));
    /* Native Play owns the string and respects the game's SFX mixer. */
    FN(void(*)(void*,void*,double,double,double,byte),0x95b770)(audio,&event,1.0,1.0,0.0,0);
}
static int ui_pressed(void) {return ui_state.captured&&(GetAsyncKeyState(VK_LBUTTON)&0x8000);}
static void ui_feedback(Control* c,int modal) {
    const UIBox* boxes=modal==2?pin_boxes:panel_boxes;unsigned count=modal==2?4:24;
    for(unsigned i=0;i<count;i++)if(!ui_available(modal,(int)i))control_feedback(c,(int)i,2);
    if(modal==1&&ui_state.focus>=0)control_feedback(c,6+ui_state.focus,1);
    DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);if(pid!=GetCurrentProcessId())return;
    double p[2];if(!control_point(c,p))return;
    int at=ui_target(boxes,count,p);
    if(at>=0&&ui_available(modal,at))control_feedback(c,at,ui_pressed()?1:0);
}
static void ui_close(void) {
    InterlockedExchange(&plan_cancel,1);
    ui_state.modal=0;ui_state.focus=-1;control_hide(&ui_state.panel);control_hide(&ui_state.pins);
    control_hide(&ui_state.shade);
    for(int i=0;i<AF_STATS;i++){control_hide(&ui_state.stats[i]);control_hide(&ui_state.result_stats[i]);}
    for(int i=0;i<9;i++)control_hide(&ui_state.pin_rows[i]);
}
static void ui_invalidate(void) {InterlockedExchange(&plan_cancel,1);plan_ready=0;snprintf(ui_status,sizeof(ui_status),"Ready.");}
static void ui_reset(void) {
    ui_state.selected=AF_ALL_STATS;ui_state.focus=-1;ui_state.action=0;ui_state.pin_page=0;
    memset(ui_state.minimum,0,sizeof(ui_state.minimum));memset(ui_state.maximum,0,sizeof(ui_state.maximum));
    include_utilities=0;pinned_count=0;ui_invalidate();
}
static int ui_bounds(double minimum[AF_STATS],double maximum[AF_STATS],unsigned* targets,unsigned* caps) {
    *targets=*caps=0;
    for(int i=0;i<AF_STATS;i++)if(ui_state.selected&(1u<<i)) {
        int lo=af_parse_bound(ui_state.minimum[i],&minimum[i]);
        int hi=af_parse_bound(ui_state.maximum[i],&maximum[i]);
        if(lo<0||hi<0){report("Min and Max must be zero or higher, or blank.");return 0;}
        if(lo&&hi) {
            double corrected=af_correct_max(minimum[i],maximum[i]);
            if(corrected!=maximum[i]) {
                maximum[i]=corrected;snprintf(ui_state.maximum[i],8,"%.7g",corrected);ui_invalidate();
            }
        }
        if(lo)*targets|=1u<<i;if(hi)*caps|=1u<<i;
    }
    return 1;
}
static void ui_blur(void) {
    if(ui_state.focus>=0) {
        double minimum[AF_STATS]={0},maximum[AF_STATS]={0};unsigned targets,caps;
        ui_bounds(minimum,maximum,&targets,&caps);
    }
    ui_state.focus=-1;
}
static void item_label(const Item* item,char out[128]) {
    snprintf(out,128,"%s",item->name);
    byte* manager=ptr(base,0x13c79d0);if(!manager)return;
    byte* def=FN(byte*(*)(void*,void*),0x942da0)(manager+0xd48,item->entry+8);
    byte* name=property(def,"name");char key_text[128];
    if(integer(name,0xa8)!=1||!string(name+0x68,key_text))return;
    GameString key={{0},strlen(key_text),15},value={{0},0,7};
    if(key.size<=15)memcpy(key.data,key_text,key.size+1);
    else {char* text=key_text;memcpy(key.data,&text,8);key.capacity=key.size;}
    FN(void*(*)(void*,void*,void*),0x962430)(base+0x13c5530,&value,&key);
    wchar_t wide[128]={0};size_t n=value.size<127?(size_t)value.size:127;
    if(value.size<=value.capacity&&read_bytes(value.capacity>7?ptr(&value,0):value.data,wide,n*2)) {
        char translated[128];
        if(WideCharToMultiByte(CP_UTF8,0,wide,-1,translated,128,NULL,NULL)&&translated[0])snprintf(out,128,"%s",translated);
    }
    FN(void(*)(void*),0x52290)(&value);
}
static void ui_pins(void) {
    unsigned n=0;void* inventory=NULL;ui_state.pin_total=0;ui_state.pin_page=0;
    if(!capture(current,&n,&inventory))return;
    for(unsigned i=0;i<n;i++)if(!strcmp(current[i].room,ui_state.room)&&normal_furniture(&current[i])) {
        unsigned at=ui_state.pin_total++;ui_state.pin_list[at]=current[i].id;
        char label[128];item_label(&current[i],label);
        snprintf(ui_state.pin_labels[at],128,"%.90s%s [%d,%d]",label,(current[i].variant&2)?" (rare)":"",
            current[i].placement[0],current[i].placement[1]);
    }
    ui_state.modal=2;ui_state.focus=-1;
}
static void ui_action(void* ui) {
    int action=ui_state.action;ui_state.action=0;
    if(!action)return;
    if(action==1) {
        double minimum[AF_STATS]={0},maximum[AF_STATS]={0};unsigned targets=0,caps=0;
        if(!ui_bounds(minimum,maximum,&targets,&caps))return;
        if(!ui_state.selected){report("Select at least one stat.");return;}
        preview(ui,ui_state.room,ui_state.selected,targets,minimum,caps,maximum);
    } else if(action==2)apply_plan(ui);
    else if(action==3){ui_invalidate();undo(ui);}
    else if(action==4)ui_pins();
    else if(action==5||action==6){ui_invalidate();return_room(ui,action==6?"all":ui_state.room);}
}
static void ui_update(void* ui) {
    if(!ui_enabled)return;
    void* scene=ptr(ptr(ui,0x18),8);uint64_t gen=generation(ui);
    if(ui_state.owner!=ui||ui_state.generation!=gen||ui_state.scene!=scene) {
        memset(&ui_state,0,sizeof(ui_state));ui_state.owner=ui;ui_state.scene=scene;ui_state.generation=gen;
        ui_reset();
        void* inv=NULL;unsigned n=0;ui_state.allowed=capture(current,&n,&inv);
    }
    if(!ui_live()) {
        for(unsigned i=0;i<ui_state.count;i++)control_hide(&ui_state.icons[i]);
        ui_close();control_hide(&ui_state.hint);return;
    }
    if(!rooms(scene,ui_state.rooms,&ui_state.count))return;
    ui_action(ui);
    for(unsigned i=0;i<ui_state.count;i++) {
        byte* room=ui_state.rooms[i];double local[2]={integer(room,0xf0)*0.5-0.56,1.35},world[2];
        FN(void(*)(void*,double*,double*),0x9b4f60)(ptr(room,0x38),world,local);
        control_show(&ui_state.icons[i],"AFRoom",world[0],world[1],0.035,30);
    }
    void* camera=layer_camera(42);byte* t=ptr(camera,0x38);if(!t)return;
    double s=fabs(number(t,0x98))*0.03;if(!isfinite(s)||s<=0)return;
    double x=number(t,0x80)-300*s,y=number(t,0x88)+(ui_state.modal==2?245:302)*s;
    control_hide(&ui_state.hint);
    if(ui_state.modal!=2)for(int i=0;i<9;i++)control_hide(&ui_state.pin_rows[i]);
    if(!ui_state.modal) {
        control_hide(&ui_state.panel);control_hide(&ui_state.pins);
        for(unsigned i=0;i<ui_state.count;i++) {
            double p[2];if(control_point(&ui_state.icons[i],p)&&inside(p,0,0,32,32)) {
                control_feedback(&ui_state.icons[i],0,0);
                byte* icon=ptr(ui_state.icons[i].renderer,0x40);
                control_show(&ui_state.hint,"AFHint",number(icon,0x80)-64*0.035,
                    number(icon,0x88)-36*0.035,0.035,30);
                control_text(&ui_state.hint,0,"hint","Auto Furniture");break;
            }
        }
        return;
    }
    control_show(&ui_state.shade,"AFShade",number(t,0x80),number(t,0x88),s,41);
    if(ui_state.modal==2) {
        for(int i=0;i<AF_STATS;i++){control_hide(&ui_state.stats[i]);control_hide(&ui_state.result_stats[i]);}
        control_hide(&ui_state.panel);control_show(&ui_state.pins,"AFPins",x,y,s,42);
        for(unsigned i=0;i<9;i++) {
            unsigned at=ui_state.pin_page*9+i;Control* row=&ui_state.pin_rows[i];
            if(at>=ui_state.pin_total){control_hide(row);continue;}
            control_show(row,"AFPinRow",x,y-(60+i*40)*s,s,42);
            control_text(row,0,"item",ui_state.pin_labels[at]);
            control_text(row,1,"check",pinned(ui_state.pin_list[at])?"X":"");
            double p[2];if(control_point(row,p)&&inside(p,18,0,564,32))control_feedback(row,0,ui_pressed()?1:0);
        }
        char page[80];snprintf(page,80,"Page %u of %u",ui_state.pin_page+1,ui_state.pin_total?(ui_state.pin_total+8)/9:1);
        control_text(&ui_state.pins,18,"page",page);
        control_text(&ui_state.pins,19,"empty",ui_state.pin_total?"":"No furniture in this room.");
        ui_feedback(&ui_state.pins,2);return;
    }
    control_hide(&ui_state.pins);control_show(&ui_state.panel,"AFPanel",x,y,s,42);
    const char* room_label=!strcmp(ui_state.room,"Floor1_Large")?"Floor 1 Left":
        !strcmp(ui_state.room,"Floor1_Small")?"Floor 1 Right":ui_state.room;
    char title[160];snprintf(title,sizeof(title),"Auto Furniture: %s",room_label);
    for(char* p=title;*p;p++)if(*p=='_')*p=' ';
    control_text(&ui_state.panel,0,"title",title);
    for(int i=0;i<AF_STATS;i++) {
        control_stat(&ui_state.stats[i],i,73,107+i*44,s*0.4);
        char key[16],value[40];snprintf(key,16,"check%d",i);
        control_text(&ui_state.panel,1+i*5,key,ui_state.selected&(1u<<i)?"X":"");
        snprintf(key,16,"min%d",i);snprintf(value,40,"%s%s",ui_state.minimum[i],ui_state.focus==i*2?"|":"");
        control_text(&ui_state.panel,2+i*5,key,value);
        snprintf(key,16,"max%d",i);snprintf(value,40,"%s%s",ui_state.maximum[i],ui_state.focus==i*2+1?"|":"");
        control_text(&ui_state.panel,3+i*5,key,value);
        snprintf(key,16,"old%d",i);snprintf(value,40,plan_ready?"%.1f":"-",plan_original_stats[i]);
        control_text(&ui_state.panel,4+i*5,key,value);
        snprintf(key,16,"new%d",i);snprintf(value,40,plan_ready?"%.1f":"-",search.best.stats[i]);
        control_text(&ui_state.panel,5+i*5,key,value);
        snprintf(key,16,"delta%d",i);value[0]=0;
        if(plan_ready) {
            double delta=search.best.stats[i]-plan_original_stats[i];
            snprintf(value,40,fabs(delta)<0.05?"%.1f":"%+.1f",fabs(delta)<0.05?0.0:delta);
            control_stat(&ui_state.result_stats[i],i,104+i*94,490,s*0.26);
        } else control_hide(&ui_state.result_stats[i]);
        control_text(&ui_state.panel,4+5*AF_STATS+i,key,value);
    }
    control_text(&ui_state.panel,3+5*AF_STATS,"change",plan_ready?"Change:":"");
    control_text(&ui_state.panel,1+5*AF_STATS,"utility",include_utilities?"X":"");
    control_text(&ui_state.panel,2+5*AF_STATS,"status",ui_status);
    ui_feedback(&ui_state.panel,1);
}
static unsigned char ui_input(void* input,void* event) {
    if(!ui_live())return original_input(input,event);
    int type=integer(event,0);
    if(type==1026&&ui_state.captured){ui_state.captured=0;return 0;}
    if(ui_state.modal&&type>=768&&type<=771) {
        if(type==768) {
            int key=integer(event,0x18);
            if(key==41){ui_sound(UI_CLOSE);if(ui_state.modal==2)ui_state.modal=1;else ui_close();}
            else if(ui_state.modal==1&&key==43) {
                int next=ui_state.focus;ui_blur();
                for(int i=0;i<2*AF_STATS;i++) {
                    next=(next+1)%(2*AF_STATS);
                    if(ui_state.selected&(1u<<(next/2))){ui_state.focus=next;break;}
                }
            }
            else if(ui_state.modal==1&&(key==40||key==88))ui_blur();
            else if(ui_state.modal==1&&ui_state.focus>=0) {
                char* text=ui_state.focus%2?ui_state.maximum[ui_state.focus/2]:ui_state.minimum[ui_state.focus/2];size_t n=strlen(text);char c=0;
                if(key>=30&&key<=38)c=(char)('1'+key-30);else if(key==39||key==98)c='0';
                else if(key>=89&&key<=97)c=(char)('1'+key-89);
                else if(key==55||key==99)c='.';else if(key==45||key==86)c='-';
                if(key==42&&n)text[n-1]=0;
                else if(key==76)text[0]=0;
                else if(c&&n<6&&(!(c=='.')||!strchr(text,'.'))&&(!(c=='-')||!n)) {text[n]=c;text[n+1]=0;}
                ui_invalidate();
            }
        }
        return 0;
    }
    if(type==1025) {
        byte button=0;read_bytes((byte*)event+0x18,&button,1);double p[2];
        if(ui_state.modal) {
            ui_state.captured=1;ui_blur();if(button!=1)return 0;
            Control* c=ui_state.modal==2?&ui_state.pins:&ui_state.panel;
            if(!control_point(c,p))return 0;
            int at=ui_target(ui_state.modal==2?pin_boxes:panel_boxes,ui_state.modal==2?4:24,p);
            if(at==0){ui_sound(UI_CLOSE);if(ui_state.modal==2)ui_state.modal=1;else ui_close();return 0;}
            if(ui_state.modal==2) {
                if(inside(p,18,60,564,360)) {
                    if((int)(p[1]-60)%40>=32)return 0;
                    unsigned at=ui_state.pin_page*9+(unsigned)((p[1]-60)/40);
                    if(at<ui_state.pin_total) {
                        uint64_t id=ui_state.pin_list[at];unsigned i=0;while(i<pinned_count&&pinned_ids[i]!=id)i++;
                        if(i<pinned_count)pinned_ids[i]=pinned_ids[--pinned_count];else if(pinned_count<MAX_ITEMS)pinned_ids[pinned_count++]=id;
                        ui_sound(pinned(id)?UI_CHECK:UI_UNCHECK);
                        ui_invalidate();
                    }
                } else if(at>=1&&ui_available(2,at)) {
                    ui_sound(at==3?UI_CLOSE:UI_SELECT);
                    if(at==1)ui_state.pin_page--;else if(at==2)ui_state.pin_page++;else ui_state.modal=1;
                }
                return 0;
            }
            if(at<0||!ui_available(1,at))return 0;
            if(at>=1&&at<=5) {
                ui_state.selected^=1u<<(at-1);ui_sound(ui_state.selected&(1u<<(at-1))?UI_CHECK:UI_UNCHECK);
                ui_invalidate();return 0;
            }
            if(at>=6&&at<16){ui_state.focus=at-6;ui_sound(UI_SELECT);return 0;}
            if(at==16){include_utilities=!include_utilities;ui_sound(include_utilities?UI_CHECK:UI_UNCHECK);ui_invalidate();}
            if(at==17){ui_sound(UI_RESET);ui_reset();return 0;}
            if(at>=18){ui_sound(UI_SELECT);ui_state.action=at-17;}
            return 0;
        }
        if(button==1)for(unsigned i=0;i<ui_state.count;i++)if(control_point(&ui_state.icons[i],p)&&inside(p,0,0,32,32)) {
            byte* live[MAX_ITEMS];unsigned n=0;if(!pieces(ui_state.scene,live,&n))return original_input(input,event);
            if(string(ui_state.rooms[i]+0x40,ui_state.room)) {
                ui_sound(UI_SELECT);ui_state.modal=1;ui_state.captured=1;ui_state.focus=-1;ui_invalidate();return 0;
            }
        }
    }
    if(ui_state.modal&&type>=1024&&type<=1027)return 0;
    return original_input(input,event);
}
static unsigned char ui_hit(void* button) {
    if(ui_state.modal&&ui_live())return 0;
    return original_hit(button);
}
static void ui_furniture_hover(void* house) {
    /* Placed furniture uses its own picker, not the native button hit test. */
    if(ui_live()) {
        if(ui_state.modal)return;
        for(unsigned i=0;i<ui_state.count;i++) {
            double p[2];
            if(control_point(&ui_state.icons[i],p)&&inside(p,0,0,32,32))return;
        }
    }
    original_furniture_hover(house);
}
