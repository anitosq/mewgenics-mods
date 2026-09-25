/* Read-only game-thread metadata; exact executable is guarded at startup. */
typedef struct { unsigned char data[16];uint64_t length,capacity; } IQGameString;
static void* iq_property(void* definition,const char* name);
static IQGameString iq_borrow(const char* text) {
    IQGameString s={{0},strlen(text),15};
    if(s.length<=15)memcpy(s.data,text,s.length+1);
    else {memcpy(s.data,&text,sizeof(text));s.capacity=s.length;}
    return s;
}
static int iq_string_read(const void* p,char* out,size_t capacity) {
    IQGameString s;
    if(!iq_read(p,&s,sizeof(s)) || s.length>=capacity || s.length>s.capacity)return 0;
    const void* data=s.capacity>15?iq_ptr(&s,0):s.data;
    if(!iq_read(data,out,(size_t)s.length))return 0;
    out[s.length]=0;return 1;
}
static void iq_wstring_read(const IQGameString* s,wchar_t* out,size_t capacity) {
    out[0]=0;
    if(s->length>100000 || s->length>s->capacity)return;
    size_t n=s->length<capacity-1?(size_t)s->length:capacity-1;
    const void* data=s->capacity>7?iq_ptr(s,0):s->data;
    if(iq_read(data,out,n*2))out[n]=0;
}
static void iq_destroy_string(IQGameString* s) {
    ((void(__cdecl*)(void*))(void*)(game_base+0x52290))(s);
}
static void iq_append(wchar_t* to,size_t cap,const wchar_t* from) {
    size_t n=wcslen(to);if(n+1<cap)to[n++]=L' ';
    size_t i=0;while(n+1<cap && from[i])to[n++]=from[i++];to[n]=0;
}
static void iq_localize(const char* key,wchar_t* out,size_t cap) {
    IQGameString k=iq_borrow(key),value={{0},0,7};
    /* Tooltip code uses this localization object and returns an owned wstring. */
    ((void*(__cdecl*)(void*,void*,void*))(void*)(game_base+0x962430))(game_base+0x13c5530,&value,&k);
    iq_wstring_read(&value,out,cap);iq_destroy_string(&value);
}
static int iq_register_set(void* manager,const char* id) {
    for(int i=0;i<iq_set_count;i++)if(!strcmp(iq_sets[i].id,id))return i;
    if(iq_set_count==IQ_SET_LIMIT)return -1;
    int i=iq_set_count++;strcpy(iq_sets[i].id,id);
    IQGameString key=iq_borrow(id);
    void* def=((void*(__cdecl*)(void*,void*))(void*)(game_base+0x942da0))((unsigned char*)manager+0x668,&key);
    void* name=iq_property(def,"name");char label[256];wchar_t translated[160];
    if(iq_int(name,0xa8)==1 && iq_string_read((unsigned char*)name+0x68,label,sizeof(label))) {
        iq_localize(label,translated,160);iq_plain_text(iq_sets[i].name,160,translated);
    }
    if(!iq_sets[i].name[0])MultiByteToWideChar(CP_UTF8,0,id,-1,iq_sets[i].name,160);
    return i;
}
static void iq_search_metadata(IQMetadata* m,void* manager,void* item,void* definition) {
    wchar_t raw[4096],plain[4096];
    const uintptr_t readers[]={0x7b4be0,0x7b5480};
    for(int i=0;i<2;i++) {
        IQGameString value={{0},0,7};
        ((void*(__cdecl*)(void*,void*,void*))(void*)(game_base+readers[i]))(manager,&value,item);
        iq_wstring_read(&value,raw,4096);iq_destroy_string(&value);
        iq_plain_text(plain,4096,raw);iq_append(m->text,4096,plain);
    }
    void* set=iq_property(definition,"set");int type=iq_int(set,0xa8);
    void* values[IQ_SET_LIMIT];int count=0;
    if(type==0)m->sets_known=1;
    else if(type==1){values[count++]=set;m->sets_known=1;}
    else if(type==4) {
        uintptr_t first=(uintptr_t)iq_ptr(set,0x38),end=(uintptr_t)iq_ptr(set,0x40);
        if(end>=first && end-first<=IQ_SET_LIMIT*0xb0 && (end-first)%0xb0==0) {
            m->sets_known=1;
            for(uintptr_t p=first;p<end;p+=0xb0)values[count++]=(void*)p;
        }
    }
    for(int n=0;n<count;n++) {
        char id[128];
        if(iq_int(values[n],0xa8)!=1 || !iq_string_read((unsigned char*)values[n]+0x68,id,sizeof(id))) {m->sets_known=0;continue;}
        int set_index=iq_register_set(manager,id);
        if(set_index<0){m->sets_known=0;continue;}
        m->sets[set_index/64]|=UINT64_C(1)<<(set_index%64);
        iq_append(m->text,4096,iq_sets[set_index].name);
    }
}
static IQMetadata* iq_find_metadata(void* drawer,uint64_t id) {
    uint64_t generation=iq_generation(drawer);
    for(int i=0;i<IQ_MAX_DRAWERS;i++)
        if(iq_metadata[i].drawer==drawer && iq_metadata[i].id==id && iq_metadata[i].generation==generation)return &iq_metadata[i];
    return NULL;
}
static int iq_extended_match(void* drawer,uint64_t id,int include_sets) {
    IQMetadata* m=iq_find_metadata(drawer,id);
    if(!m)return !iq_search.query[0] && (!include_sets || !iq_search.set_mode);
    return iq_query_match(m->text,iq_search.query) && (!include_sets || iq_set_match(&iq_search,m->sets,m->sets_known));
}
