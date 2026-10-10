static void iq_query_label(wchar_t* label,const wchar_t* query,const wchar_t* placeholder,int focus) {
    if(!focus){swprintf(label,200,L"%ls",*query?query:placeholder);return;}
    /* Keep the caret visible when the query exceeds the field width. */
    int start=iq_cursor>35?iq_cursor-35:0;
    if(iq_select_all)swprintf(label,200,L"> %ls <",query+start);
    else swprintf(label,200,L"%.*ls|%.35ls",iq_cursor-start,query+start,query+iq_cursor);
}
/* Shared toolbar flow; native rendering boundaries are stubbed in its fixture. */
static void iq_ui_update(void) {
    if(!iq_is_open()){iq_hide_all();filter_popup=0;if(iq_focus)iq_focus_set(0);return;}
    if(filter_popup==3)iq_build_set_rows();
    int refresh=!iq_labels_match(),ready=1;
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
    if(refresh || !iq_label_cache.valid) {
        iq_query_label(label,iq_search.query,L"Search name, description, set...",focused);
        ready&=iq_text(&iq_controls.search[focused],0,"query",label);
    }
    snprintf(name,sizeof(name),"IQBar%d%d",active_filter.type,active_filter.rarity);
    iq_control_show(&iq_controls.bars[selected],name,x,y-42*s);
    if(refresh || !iq_label_cache.valid) {
        int selected_sets=0,last=0;
        for(int j=0;j<iq_set_count;j++)if(iq_search.sets[j/64]&(UINT64_C(1)<<(j%64))){selected_sets++;last=j;}
        if(iq_search.set_mode==3 && selected_sets==1){iq_set_name(label,iq_sets[last].name);if(wcslen(label)>6)wcscpy(label+4,L"..");}
        else if(iq_search.set_mode==3)swprintf(label,200,L"%d sets",selected_sets);
        else swprintf(label,200,L"%ls",iq_search.set_mode==1?L"Any set":iq_search.set_mode==2?L"No set":L"All sets");
        ready&=iq_text(&iq_controls.bars[selected],0,"sets",label);
    }
    if(iq.equipment)iq_control_hide(&iq_controls.count);
    else {
        iq_control_show(&iq_controls.count,"IQCount",x+12*s,y-80*s);
        if(refresh || !iq_label_cache.valid) {
            swprintf(label,200,L"Storage: %d    Trash: %d%ls",iq.items[0],iq.items[1],iq.items[0]+iq.items[1]==0?L"    No matches":L"");
            ready&=iq_text(&iq_controls.count,0,"count",label);
        }
    }
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
        IQControl* popup=&iq_controls.popups[iq_popup_index()];iq_control_show(popup,name,x+131*s,y-82*s);
        if(refresh || !iq_label_cache.valid) {
            iq_query_label(label,iq_set_query,L"Find a set...",iq_focus==2);ready&=iq_text(popup,0,"query",label);
            ready&=iq_text(popup,23,"storage_header",iq.equipment?L"Available":L"Storage");
            ready&=iq_text(popup,24,"trash_header",iq.equipment?L"":L"Trash");
            for(int row=0;row<IQ_SET_PAGE;row++) {
                int at=iq_set_offset+row,j=at<iq_set_rows_count?iq_set_rows[at]:-1;label[0]=0;
                if(j>=0) {
                    int checked=iq_search.set_mode==3 && (iq_search.sets[j/64]&(UINT64_C(1)<<(j%64)))!=0;
                    wchar_t short_name[160];iq_set_name(short_name,iq_sets[j].name);
                    if(wcslen(short_name)>22)wcscpy(short_name+20,L"..");
                    swprintf(label,200,L"[%lc] %ls",checked?L'x':L' ',short_name);
                } else if(!row)wcscpy(label,L"No matching sets");
                snprintf(name,sizeof(name),"row%d",row);ready&=iq_text(popup,row+1,name,label);
                label[0]=0;if(j>=0)swprintf(label,200,L"%d",iq_owned.counts[j][0]);
                snprintf(name,sizeof(name),"storage%d",row);ready&=iq_text(popup,row+8,name,label);
                label[0]=0;if(j>=0 && !iq.equipment)swprintf(label,200,L"%d",iq_owned.counts[j][1]);
                snprintf(name,sizeof(name),"trash%d",row);ready&=iq_text(popup,row+15,name,label);
            }
        }
    }
    if(filter_popup==3)for(int dir=0;dir<2;dir++) {
        void* clip=iq_ptr(iq_controls.popups[iq_popup_index()].renderer,0x80);
        IQGameString key=iq_borrow(dir?"disabled1":"disabled0");
        void* child=clip?((void*(__cdecl*)(void*,void*))(void*)(game_base+0x99a0e0))(clip,&key):NULL;
        if(child) {
            int disabled=dir?iq_set_offset+IQ_SET_PAGE>=iq_set_rows_count:iq_set_offset==0;
            if(disabled)*((unsigned char*)child+8)|=0x20;
            else *((unsigned char*)child+8)&=(unsigned char)~0x20;
        }
    }
    if(refresh || !iq_label_cache.valid) {
        if(ready)iq_labels_store();else iq_label_cache.valid=0;
    }
    iq_rarity_art(x,y,s);
    iq_feedback_update();
}
