/* InventoryScreen2 uses world-space InventoryItemBox drawers. Only the
 * pre-adventure Storage grid is adapted; equipped slots remain native. */
static LayoutGrid original_equipment_grid;
static DrawerUpdate original_equipment_update,original_equipment_drawer_update;
static BindItem original_equipment_bind;

static void iq_capture_equipment(void* owner) {
    if(!layout_test)return;
    if(iq_int(owner,0x58)!=1) {
        if(iq.owner==owner){iq_hide_all();memset(&iq,0,sizeof(iq));}
        return;
    }
    iq_view_revision++;
    memset(&pending_view,0,sizeof(pending_view));
    if(iq.owner==owner && iq_view_live())pending_view.row[0]=iq.row[0];
    memset(&iq,0,sizeof(iq));
#define iq pending_view
    iq.owner=owner;iq.equipment=1;iq.panels[0]=owner;
    iq.owner_ref=iq_reference(owner);iq.panel_refs[0]=iq.owner_ref;
    iq.entity_ref=iq_reference(iq_ptr(owner,0x18));
    iq.scene_ref=iq_reference(iq_ptr(iq_ptr(owner,0x18),8));
    if(!iq_reference_valid(iq.scene_ref))return;
    int n=iq_int(owner,0xac),columns=iq_int(owner,0x5c),backgrounds=iq_int(owner,0xbc);
    void** drawers=iq_ptr(owner,0xb0);void** cells=iq_ptr(owner,0xc0);
    if(n<0 || n>IQ_MAX_DRAWERS || (n && !drawers) || columns<4 || columns>20 ||
       backgrounds!=columns*columns || !cells)return;
    IQGameString key=iq_borrow("inventory");double bounds[4]={0};
    ((void*(__cdecl*)(void*,double*,void*))(void*)(game_base+0x97cb30))(iq_ptr(owner,0x40),bounds,&key);
    double width=bounds[1]-bounds[0];
    if(!isfinite(width) || width<=0 || width>10000)return;
    int cols=columns<IQ_COLUMNS?columns:IQ_COLUMNS;
    iq.columns[0]=cols;iq.columns[1]=1;
    iq.left[0]=bounds[0];iq.bottom[0]=bounds[2];iq.span[0]=width;
    for(int j=0;j<backgrounds;j++) {
        void* renderer=NULL;if(!iq_read(cells+j,&renderer,8))return;
        void* transform=iq_ptr(renderer,0x40);if(!transform)return;
        IQBackground* b=&iq.backgrounds[iq.background_count++];
        b->renderer=renderer;b->transform=transform;b->ordinal=j;
        b->renderer_generation=iq_generation(renderer);b->transform_generation=iq_generation(transform);
        b->x=b->offset_x=iq_double(transform,0x80);b->y=b->offset_y=iq_double(transform,0x88);
        b->sx=iq_double(transform,0x98);b->sy=iq_double(transform,0xa0);
        iq_read((unsigned char*)renderer+0x51,&b->visible,1);
    }
    /* The native item targets supply the icon scale; background art has its
       own scale. Capture both rather than assuming their symbols match. */
    iq.scale[0]=4.0/cols;
    for(int j=0;j<n;j++) {
        void* drawer=NULL;if(!iq_read(drawers+j,&drawer,8))return;
        uint64_t id=0;if(!iq_read((unsigned char*)drawer+0x58,&id,8))return;
        int64_t assigned=((int64_t(__cdecl*)(void*,uint64_t))(void*)(game_base+0x34bcb0))(owner,id);
        if(assigned!=-1)continue;
        IQDrawer* d=&iq.drawers[iq.count++];d->drawer=drawer;d->reference=iq_reference(drawer);d->item_id=id;
        d->traits=iq_traits(drawer,id);d->rarity=iq_int(drawer,0xfc);
        d->ordinal=iq_filter_match(active_filter,d->traits,d->rarity) && iq_extended_match(drawer,id,1)?iq.items[0]++:-1;
        iq.scale[0]=iq_double(drawer,0xc8)*columns/cols;
    }
    int maxrow=iq_last_row(iq.items[0],cols,cols>5?5:cols);if(iq.row[0]>maxrow)iq.row[0]=maxrow;
#undef iq
    iq=pending_view;
    iq_index_view();
    for(int j=0;j<iq.background_count;j++) {
        IQBackground* b=&iq.backgrounds[j];int visible=j<cols*iq_rows(0);
        *((unsigned char*)b->renderer+0x51)=(unsigned char)visible;
        if(!visible)continue;
        double pitch=width/cols*0.90,factor=(double)columns/cols*0.90;
        iq_write_double(b->transform,0x80,iq.left[0]+pitch*(j%cols+0.5));
        iq_write_double(b->transform,0x88,iq.bottom[0]+width-pitch*(cols-j/cols-0.5)+width*0.072);
        iq_write_double(b->transform,0x98,b->sx*factor);iq_write_double(b->transform,0xa0,b->sy*factor);
    }
    for(int j=0;j<iq.count;j++)iq_present(&iq.drawers[j]);
    iq_ui_capture();
    char msg[160];snprintf(msg,sizeof(msg),"Equipment layout: %d available, %d matching, %d columns; bounds %.1f %.1f %.1f",iq.count,iq.items[0],cols,iq.left[0],iq.bottom[0],width);report(msg);
}
static void __cdecl iq_equipment_grid(void* owner) {
    iq_before_grid(owner);
    original_equipment_grid(owner);
    iq_capture_equipment(owner);
}
static void __cdecl iq_equipment_bind(void* drawer,void* item) {
    original_equipment_bind(drawer,item);iq_cache_item(drawer,item);
}
static void __cdecl iq_equipment_drawer_update(void* drawer) {
    original_equipment_drawer_update(drawer);
    if(!layout_test || !iq.equipment)return;
    IQDrawer* d=iq_find(drawer);if(d)iq_present(d);
}
static void __cdecl iq_equipment_update(void* owner) {
    original_equipment_update(owner);
    if(layout_test && iq.equipment && iq.owner==owner)iq_ui_update();
}
