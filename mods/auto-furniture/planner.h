#include "solver.h"
typedef struct {char data[16];uint64_t size,capacity;} GameString;
static const char* stat_names[AF_STATS]={"Comfort","Stimulation","Health","Evolution","Appeal"};
static AFProblem problem;
static AFSearch search;
static Item plan_items[MAX_ITEMS];
static unsigned plan_count,plan_indices[AF_ITEMS];
static byte* plan_room;
static int plan_ready;
static void* plan_ui;
static void* plan_scene;
static uint64_t plan_generation;
static int include_utilities;
static uint64_t pinned_ids[MAX_ITEMS];
static unsigned pinned_count;
static double plan_original_stats[AF_STATS];
static HANDLE plan_worker;
static volatile LONG plan_cancel;
static DWORD WINAPI search_worker(void* ignored) {
    (void)ignored;ULONGLONG until=GetTickCount64()+1200;
    while(search.iterations<3000&&GetTickCount64()<until&&!InterlockedCompareExchange(&plan_cancel,0,0))af_step(&problem,&search);
    if(!InterlockedCompareExchange(&plan_cancel,0,0))af_fill_utilities(&problem,&search);
    return 0;
}
static void poll_search(void) {
    if(!plan_worker||WaitForSingleObject(plan_worker,0)!=WAIT_OBJECT_0)return;
    CloseHandle(plan_worker);plan_worker=NULL;
    if(InterlockedCompareExchange(&plan_cancel,0,0)){plan_ready=0;return;}
    plan_ready=1;
    if(!af_within_caps(&problem,search.best.stats)) {
        report("No layout found within Max. Raise a limit or unpin furniture.");return;
    }
    if(log_message)log_message("AutoFurniture","Best found: %.1f %.1f %.1f %.1f %.1f; %u trials",
        search.best.stats[0],search.best.stats[1],search.best.stats[2],search.best.stats[3],search.best.stats[4],search.iterations);
    int shortfall=0;
    for(int i=0;i<AF_STATS;i++)if((problem.targets&(1u<<i))&&search.best.stats[i]+1e-9<problem.minimum[i])shortfall=1;
    report(shortfall?"Calculation complete. Some Min targets are still unmet.":"Calculation complete.");
}
static int pinned(uint64_t id) {
    for(unsigned i=0;i<pinned_count;i++)if(pinned_ids[i]==id)return 1;return 0;
}
static byte* property(void* definition,const char* name) {
    GameString key={{0},strlen(name),15};if(key.size>15)return NULL;
    memcpy(key.data,name,key.size+1);
    return FN(byte*(*)(void*,void*),0x942da0)(definition,&key);
}
static int metadata(const Item* item,AFShape* shape,int* utility) {
    byte* manager=ptr(base,0x13c79d0);if(!manager)return 0;
    byte* definition=FN(byte*(*)(void*,void*),0x942da0)(manager+0xd48,item->entry+8);
    if(!definition||integer(definition,0xa8)!=3)return 0;
    byte* removed=property(definition,"removed");byte removed_flag=0;
    if(integer(removed,0xa8)==5&&read_bytes(removed+0x60,&removed_flag,1)&&removed_flag)return 0;
    byte* start=ptr(definition,0x38);byte* end=ptr(definition,0x40);
    if(!start||end<start||(end-start)%0xb0||(end-start)/0xb0>256)return 0;
    *utility=0;int effects=0;
    for(byte* p=start;p<end;p+=0xb0) {
        char name[128];if(!string(p+0x88,name))return 0;
        if(!strcmp(name,"name")||!strcmp(name,"desc")||!strcmp(name,"set")||
           !strcmp(name,"special")||!strcmp(name,"can_be_rare")||!strcmp(name,"removed"))continue;
        if(integer(p,0xa8)!=2)return 0;
        double value=0;if(!read_bytes(p+0x58,&value,8)||!isfinite(value))return 0;
        effects++;int j=0;while(j<AF_STATS&&strcmp(name,stat_names[j]))j++;
        if(j<AF_STATS)shape->stats[j]+=value*((item->variant&2)?2:1);
        else if(value!=0)*utility=1;
    }
    if(!effects)shape->stats[0]=(item->variant&2)?2:1;
    return 1;
}
static int read_shape(const Item* item,AFShape* shape,byte* asset) {
    byte* data=FN(byte*(*)(void*,void*),0x1b09c0)(asset+0x38,item->entry+8);
    byte cells[576];if(!data||!read_bytes(data+4,cells,576))return 0;
    for(int y=0;y<24;y++)for(int x=0;x<24;x++)if(cells[y*24+x]) {
        byte value=cells[y*24+x];if(value>5)return 0;
        shape->cells[shape->count++]=(AFCell){(byte)x,(byte)y,value};
        if(value<=2||value==5)shape->solid++;
    }
    return shape->count>0;
}
static int room_stats(byte* room,double stats[AF_STATS]) {
    memset(stats,0,AF_STATS*sizeof(*stats));FN(void(*)(void*),0x2eac20)(room);
    byte* first=ptr(room,0x140);byte* last=ptr(room,0x148);
    if(last<first||(last-first)%40||(last-first)/40>512)return 0;
    for(byte* p=first;p<last;p+=40) {
        char name[128];double value=0;
        if(!string(p,name)||!read_bytes(p+32,&value,8)||!isfinite(value))return 0;
        for(int j=0;j<AF_STATS;j++)if(!strcmp(name,stat_names[j]))stats[j]+=value;
    }
    return 1;
}
static void preview(void* ui,const char* room_name,unsigned selected,unsigned targets,const double minimum[AF_STATS],unsigned caps,const double maximum[AF_STATS]) {
    if(plan_worker){report("Searching...");return;}
    plan_ready=0;
    void* inv=NULL;byte* live[MAX_ITEMS];unsigned n=0;byte* scene=ptr(ptr(ui,0x18),8);
    if(!capture(plan_items,&plan_count,&inv)||!pieces(scene,live,&n)){report("Cannot read the room. Finish moving furniture and try again.");return;}
    plan_ui=ui;plan_scene=scene;
    if(!read_bytes((byte*)ui-8,&plan_generation,8))return;
    plan_room=NULL;
    byte* all_rooms[MAX_ROOMS];unsigned nr=0;
    if(!rooms(scene,all_rooms,&nr))return;
    for(unsigned j=0;j<nr;j++) {
        byte* room=all_rooms[j];char name[128];
        if(room&&string(room+0x40,name)&&!strcmp(name,room_name)){plan_room=room;break;}
    }
    if(!plan_room){report("Room not found. Close this panel and reopen it.");return;}
    memset(&problem,0,sizeof(problem));
    problem.width=integer(plan_room,0xf0);problem.height=integer(plan_room,0xf4);
    problem.selected=selected;problem.targets=targets;memcpy(problem.minimum,minimum,sizeof(problem.minimum));
    problem.caps=caps;if(maximum)memcpy(problem.maximum,maximum,sizeof(problem.maximum));
    if(problem.width<1||problem.width>64||problem.height<1||problem.height>64||
       !read_bytes(ptr(plan_room,0x108),problem.grid,(size_t)problem.width*problem.height)) {
        report("Cannot read the room layout.");return;
    }
    /* The asset loader consumes its string, just as FurniturePiece::Init does. */
    GameString path={{0},0,15};const char* filename="data/furniture_info.data";
    FN(void*(*)(void*,const char*,size_t),0x520d0)(&path,filename,strlen(filename));
    byte* asset=FN(byte*(*)(void*,void*),0x1b0cb0)(NULL,&path);
    if(!asset){report("Cannot load furniture shapes.");return;}
    byte* entries[MAX_ITEMS];piece_entries(live,n,entries);
    AFLayout initial={0};unsigned fixed=0;unsigned char immovable[AF_ITEMS]={0};
    for(unsigned i=0;i<plan_count;i++) {
        Item* item=&plan_items[i];int here=!strcmp(item->room,room_name);
        if(!here&&item->room[0])continue;
#ifndef AF_RELEASE
        if(item->id==MARKER_ID)continue;
#endif
        AFShape shape={0};int utility=0;
        if(!metadata(item,&shape,&utility)||!read_shape(item,&shape,asset)) {
            report("This room or inventory contains unsupported furniture.");return;
        }
        AFPlacement v={item->placement[0],item->placement[1],item->placement[2],item->placement[3],here};
        if(here) {
            byte* piece=NULL;for(unsigned j=0;j<n;j++)if(entries[j]==item->entry)piece=live[j];
            int actual[2];uint64_t cell=0;
            if(!piece){report("A placed item could not be found. Reopen the room and try again.");return;}
            FN(void(*)(void*,int*,uint64_t,void*),0x2ef3a0)(piece,actual,cell,plan_room);
            if(actual[0]!=v.x||actual[1]!=v.y){report("Furniture positions do not match the room. Reopen it and try again.");return;}
            cell=UINT64_C(23)|(UINT64_C(23)<<32);
            FN(void(*)(void*,int*,uint64_t,void*),0x2ef3a0)(piece,actual,cell,plan_room);
            if(actual[0]!=v.x+23*v.sx||actual[1]!=v.y+23*v.sy){report("A placed item has an unsupported rotation or scale.");return;}
        }
        shape.utility=utility&&include_utilities;
        int at=problem.count++;problem.shape[at]=shape;plan_indices[at]=i;initial.placement[at]=v;
        immovable[at]=!strcmp(item->name,"poop")||!strcmp(item->name,"autofeeder")||
            (utility&&!include_utilities)||pinned(item->id);
    }
    af_fix_dependencies(&problem,&initial,immovable);
    int movable=0;
    for(int i=0;i<problem.count;i++) {
        if(immovable[i]) {
            if(initial.placement[i].used) {
                AFShape* shape=&problem.shape[i];AFPlacement v=initial.placement[i];
                for(int c=0;c<shape->count;c++) {
                    AFCell cell=shape->cells[c];int x=v.x+cell.x*v.sx,y=v.y+cell.y*v.sy;
                    if(x<0||x>=problem.width||y<0||y>=problem.height){report("A fixed item extends outside the room.");return;}
                }
                af_stamp(&problem,problem.grid,shape,v);fixed++;
            }
        } else {
            problem.shape[movable]=problem.shape[i];plan_indices[movable]=plan_indices[i];
            initial.placement[movable++]=initial.placement[i];
        }
    }
    problem.count=movable;
    if(!room_stats(plan_room,problem.base)){report("Cannot read the room's stats.");return;}
    for(int i=0;i<problem.count;i++)if(initial.placement[i].used)
        for(int j=0;j<AF_STATS;j++)problem.base[j]-=problem.shape[i].stats[j];
    if(!af_valid_problem(&problem)||!af_rebuild(&problem,&initial,search.grid,0)) {
        report("Cannot verify the current layout. Furniture has not been moved.");return;
    }
    byte native_grid[AF_CELLS];
    if(!read_bytes(ptr(plan_room,0xe8),native_grid,(size_t)problem.width*problem.height)||
       memcmp(native_grid,search.grid,(size_t)problem.width*problem.height)) {
        report("Furniture positions do not match the room. Nothing has been moved.");return;
    }
    if(log_message)log_message("AutoFurniture","PREVIEW BASE: %.1f %.1f %.1f %.1f %.1f; %d available, %u fixed, grid %dx%d",
        initial.stats[0],initial.stats[1],initial.stats[2],initial.stats[3],initial.stats[4],problem.count,fixed,problem.width,problem.height);
    memcpy(plan_original_stats,initial.stats,sizeof(plan_original_stats));
    if(!af_start(&problem,&search,&initial)){report("Cannot search this layout. Furniture has not been moved.");return;}
    InterlockedExchange(&plan_cancel,0);
    plan_worker=CreateThread(NULL,0,search_worker,NULL,0,NULL);
    report(plan_worker?"Searching...":"Search could not start.");
}
static void apply_plan(void* ui) {
    uint64_t generation=0;
    if(plan_worker||!plan_ready||ui!=plan_ui||ptr(ptr(ui,0x18),8)!=plan_scene||
       !read_bytes((byte*)ui-8,&generation,8)||generation!=plan_generation) {
        report("Result expired. Calculate again before applying.");return;
    }
    if(!af_within_caps(&problem,search.best.stats)){report("This layout exceeds Max and cannot be applied.");return;}
    for(int i=0;i<problem.count;i++)if(search.best.placement[i].used&&search.best.placement[i].sy!=1) {
        report("Cannot apply a layout with upside-down furniture.");return;
    }
    double stats[AF_STATS];
    if(!room_stats(plan_room,stats)||memcmp(stats,plan_original_stats,sizeof(stats))) {
        plan_ready=0;report("Room stats changed. Calculate again.");return;
    }
    unsigned n=0;void* inv=NULL;
    if(!capture(current,&n,&inv)||n!=plan_count){report("Inventory changed. Calculate again.");return;}
    for(unsigned i=0;i<n;i++) {
        unsigned j=0;while(j<plan_count&&current[i].id!=plan_items[j].id)j++;
        if(j==plan_count||!equal(&current[i],&plan_items[j])){report("Furniture changed. Calculate again.");return;}
    }
    if(!af_rebuild(&problem,&search.best,search.grid,0)||!transaction_begin(ui)||count!=plan_count) {
        report("The room changed or is busy. Finish moving furniture, then calculate again.");return;
    }
    for(unsigned i=0;i<count;i++) {
        unsigned j=0;while(j<plan_count&&before[i].id!=plan_items[j].id)j++;
        if(j==plan_count||!equal(&before[i],&plan_items[j])){report("Furniture changed. Calculate again.");return;}
    }
    char room_name[128];if(!string(plan_room+0x40,room_name))return;
    for(int i=0;i<problem.count;i++) {
        unsigned at=0;while(at<count&&after[at].id!=plan_items[plan_indices[i]].id)at++;
        if(at==count)return;
        AFPlacement v=search.best.placement[i];Item* item=&after[at];
        if(!v.used){item->room[0]=0;continue;}
        if(!item->room[0])item->placement[4]=(int)count+i;
        strcpy(item->room,room_name);
        item->placement[0]=v.x;item->placement[1]=v.y;item->placement[2]=v.sx;item->placement[3]=v.sy;
    }
    normalize_orders(after,count);plan_ready=0;
    if(!transaction_apply())return;
    byte native_grid[AF_CELLS];
    if(!read_bytes(ptr(plan_room,0xe8),native_grid,(size_t)problem.width*problem.height)||
       memcmp(native_grid,search.grid,(size_t)problem.width*problem.height)) {
        int restored=transaction_restore(before)&&transaction_grids(0,0);
        undo_ready=0;transaction_check=0;
        report(restored?"Placement differed from the calculated layout. Original layout restored.":"Could not restore the layout. Stop editing and check your save backup.");return;
    }
    report("Layout applied.");
}
