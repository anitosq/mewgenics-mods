#define MAX_ROOMS 32
typedef struct {byte* room;uint64_t generation;unsigned size;byte before[4096],after[4096];} RoomState;
static RoomState room_states[MAX_ROOMS];
static unsigned room_count;
static uint64_t original_piece_order[MAX_ITEMS];
static unsigned original_piece_count;
static int transaction_check;
static int same_placement(const Item* a,const Item* b) {
    return !strcmp(a->room,b->room)&&!memcmp(a->placement,b->placement,16);
}
static int normal_furniture(const Item* item) {
    return strcmp(item->name,"poop")&&strcmp(item->name,"autofeeder");
}
static int transaction_rooms(void* scene) {
    byte* all[MAX_ROOMS];unsigned n=0;if(!rooms(scene,all,&n))return 0;
    room_count=0;
    for(unsigned i=0;i<n;i++) {
        byte* room=all[i];
        unsigned j=0;while(j<room_count&&room_states[j].room!=room)j++;
        if(j<room_count)continue;
        if(room_count==MAX_ROOMS)return 0;
        RoomState* r=&room_states[room_count++];memset(r,0,sizeof(*r));r->room=room;
        int dims[2];
        if(!read_bytes(room+0xf0,dims,8)||dims[0]<1||dims[1]<1||dims[0]>64||dims[1]>64)return 0;
        r->size=(unsigned)(dims[0]*dims[1]);
        if(!read_bytes(room-8,&r->generation,8)||!read_bytes(ptr(room,0xe8),r->before,r->size))return 0;
    }
    return room_count>0;
}
static int transaction_grids(int which,int save) {
    for(unsigned i=0;i<room_count;i++) {
        RoomState* r=&room_states[i];uint64_t generation=0;byte grid[4096];int dims[2];
        if(!read_bytes(r->room-8,&generation,8)||generation!=r->generation||
           !read_bytes(r->room+0xf0,dims,8)||dims[0]<1||dims[1]<1||dims[0]>64||dims[1]>64||
           (unsigned)(dims[0]*dims[1])!=r->size||!read_bytes(ptr(r->room,0xe8),grid,r->size))return 0;
        byte* expected=which?r->after:r->before;
        if(save)memcpy(expected,grid,r->size);else if(memcmp(expected,grid,r->size))return 0;
    }
    return 1;
}
static void normalize_orders(Item* items,unsigned n) {
    int order[MAX_ITEMS];
    for(unsigned i=0;i<n;i++) {
        order[i]=0;if(!items[i].room[0])continue;
        for(unsigned j=0;j<n;j++)if(items[j].room[0]&&
            (items[j].placement[4]<items[i].placement[4]||
             (items[j].placement[4]==items[i].placement[4]&&j<i)))order[i]++;
    }
    for(unsigned i=0;i<n;i++)if(items[i].room[0])items[i].placement[4]=order[i];
}
static int transaction_begin(void* ui) {
    byte* live[MAX_ITEMS];unsigned n=0;byte* scene=ptr(ptr(ui,0x18),8);
    if(refresh_pending)return 0;
    undo_ready=0;
    if(refresh_pending||!capture(before,&count,&saved_inventory)||!pieces(scene,live,&n)||
       !transaction_rooms(scene)||!read_bytes((byte*)ui-8,&saved_generation,8))return 0;
    saved_ui=ui;saved_scene=scene;
    original_piece_count=n;
    for(unsigned i=0;i<n;i++)if(!read_bytes(ptr(live[i],0x2d8),&original_piece_order[i],8))return 0;
    memcpy(after,before,count*sizeof(Item));return 1;
}
/* Called only on the house's update thread. Every precondition precedes mutation. */
static int transaction_restore(const Item* target) {
    byte* live[MAX_ITEMS];byte* matches[MAX_ITEMS]={0};unsigned n=0,total=0;void* inv=NULL;
    if(!capture(current,&total,&inv)||total!=count||inv!=saved_inventory||!pieces(saved_scene,live,&n))return 0;
    for(unsigned i=0;i<count;i++) {
        Item* source=NULL;
        for(unsigned j=0;j<total;j++)if(current[j].id==target[i].id)source=&current[j];
        if(!source||source->entry!=target[i].entry||source->variant!=target[i].variant||strcmp(source->name,target[i].name))return 0;
        for(unsigned j=0;j<n;j++)if(ptr(live[j],0x2d8)==source->entry) {
            if(matches[i])return 0;matches[i]=live[j];
        }
        if(!!source->room[0]!=!!matches[i])return 0;
        if(source->room[0]&&!ptr(matches[i],0x48))return 0;
    }
    unsigned char changed[MAX_ITEMS]={0};
    for(unsigned i=0;i<count;i++) {
        Item* source=NULL;for(unsigned j=0;j<total;j++)if(current[j].id==target[i].id)source=&current[j];
        changed[i]=!same_placement(source,&target[i]);
        if(target==before&&(source->room[0]||target[i].room[0]))changed[i]=1;
    }
    for(unsigned i=0;i<count;i++)if(changed[i]&&matches[i])FN(Update,0x2eeef0)(matches[i]);
    for(unsigned i=0;i<count;i++)if(changed[i]&&matches[i]) {
        matches[i][0x2b3]=0;FN(Update,0x94eba0)(matches[i]);
    }
    for(unsigned i=0;i<count;i++) {
        const Item* item=&target[i];
        FN(void*(*)(void*,const char*,size_t),0x520d0)(item->entry+0x30,item->room,strlen(item->room));
        memcpy(item->entry+0x50,item->placement,20);
        if(!changed[i]&&matches[i])memcpy(matches[i]+0x324,&item->placement[4],4);
    }
    /* Init restores the native transform and stamps its room. No ownership allocation. */
    /* Undo rebuilds the original component order too. Native room updates stamp
       in that order, which determines precedence for overlapping type-5 cells. */
    unsigned spawn_count=target==before?original_piece_count:count;
    for(unsigned at=0;at<spawn_count;at++) {
        unsigned i=at;
        if(target==before) {
            i=0;while(i<count&&target[i].id!=original_piece_order[at])i++;
            if(i==count)return 0;
        }
        if(!changed[i]||!target[i].room[0])continue;
        void* entity=FN(void*(*)(void*),0x96b3e0)(saved_scene);
        if(!entity)return 0;
        byte* piece=FN(byte*(*)(void*,void*,const uint64_t*),0x1aca10)(saved_scene,entity,&target[i].id);
        char room[128];byte* actual=ptr(piece,0x48);
        if(!actual||!string(actual+0x40,room)||strcmp(room,target[i].room))return 0;
    }
    for(unsigned i=0;i<room_count;i++)FN(Update,0x2ea270)(room_states[i].room);
    return unchanged(target);
}
static int transaction_apply(void) {
    if(!unchanged(before)||!transaction_grids(0,0)){report("Furniture changed. Try again.");return 0;}
    undo_ready=0;
    if(!transaction_restore(after)) {
        int restored=transaction_restore(before)&&transaction_grids(0,0);
        report(restored?"Change failed. Original layout restored.":"Could not restore the layout. Stop editing and check your save backup.");
        refresh_pending=3;return 0;
    }
    if(!transaction_grids(1,1)) {
        report("Could not verify the change. Stop editing and check your save backup.");return 0;
    }
    refresh_pending=3;transaction_check=1;undo_ready=1;return 1;
}
static void return_room(void* ui,const char* name) {
    unsigned n=0;void* inv=NULL;int available=0;
    if(!capture(current,&n,&inv)){report("Cannot read the furniture inventory. Try again.");return;}
    for(unsigned i=0;i<n;i++)if(current[i].room[0]&&normal_furniture(&current[i])&&
        (!strcmp(name,"all")||!strcmp(name,current[i].room)))available=1;
    if(!available){report("Nothing to return.");return;}
    if(!name[0]||!transaction_begin(ui)){report("The room is busy or has changed. Finish moving furniture and try again.");return;}
    unsigned selected=0;
    for(unsigned i=0;i<count;i++)if(before[i].room[0]&&normal_furniture(&before[i])&&
        (!strcmp(name,"all")||!strcmp(name,before[i].room))) {after[i].room[0]=0;selected++;}
    if(!selected){report("Nothing to return.");return;}
    normalize_orders(after,count);
    if(transaction_apply()) {
        char message[180];snprintf(message,sizeof(message),"Returned %u item%s to inventory.",selected,selected==1?"":"s");report(message);
        ui_return_count=selected;
    }
}
static void undo(void* ui) {
    uint64_t generation=0;
    if(!undo_ready||refresh_pending||ui!=saved_ui||ptr(ptr(ui,0x18),8)!=saved_scene||
       !read_bytes((byte*)ui-8,&generation,8)||generation!=saved_generation||!unchanged(after)||!transaction_grids(1,0)) {
        report("Cannot undo: furniture, campaign, or room state has changed.");return;
    }
    if(transaction_restore(before)&&transaction_grids(0,0)) {
        report("UNDO PASS: every furniture record and room occupancy restored exactly.");
        undo_ready=0;refresh_pending=3;transaction_check=2;
    } else {
        report("Could not verify Undo. Stop editing and check your save backup.");undo_ready=0;
    }
}
static void transaction_verify(void) {
    if(!transaction_check)return;int undo_check=transaction_check==2;transaction_check=0;
    if(unchanged(undo_check?before:after)&&transaction_grids(!undo_check,0))
        report(undo_check?"UNDO SETTLED: restored state remains exact after native updates.":"APPLY PASS: all ownership, placement, and occupancy checks passed.");
    else {undo_ready=0;report("Furniture changed unexpectedly. Stop editing and check your save backup.");}
}
