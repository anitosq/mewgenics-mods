#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#ifdef NDEBUG
#error UI checks require assertions
#endif

static unsigned reads,hits;
static BOOL counted_read(HANDLE process,LPCVOID source,LPVOID target,SIZE_T size,SIZE_T* got) {
    reads++;
    return ReadProcessMemory(process,source,target,size,got);
}
#define ReadProcessMemory counted_read
static void* test_function(unsigned rva);
#define FN(type,rva) ((type)test_function(rva))
#include "native.c"

static unsigned text_writes,child_lookups;
static int missing_child;
typedef struct {char name[32];wchar_t text[AF_TEXT_BYTES];} TestField;
static TestField fields[64];static unsigned field_count;
static void* test_child(void* clip,GameString* name) {
    (void)clip;child_lookups++;
    if(missing_child)return NULL;
    for(unsigned i=0;i<field_count;i++)if(!strcmp(fields[i].name,name->data))return &fields[i];
    assert(field_count<64);TestField* field=&fields[field_count++];
    strcpy(field->name,name->data);return field;
}
static void* test_wide(GameString* string,const wchar_t* value,size_t n) {
    string->size=n;string->capacity=n>7?n:7;
    if(n<=7)memcpy(string->data,value,(n+1)*sizeof(wchar_t));else memcpy(string->data,&value,sizeof(value));
    return string;
}
static void test_set_text(TestField* field,GameString* value,byte a,byte b) {
    (void)a;(void)b;const wchar_t* text=(const wchar_t*)value->data;
    if(value->capacity>7)memcpy(&text,value->data,sizeof(text));
    assert(value->size<AF_TEXT_BYTES);memcpy(field->text,text,value->size*sizeof(wchar_t));
    field->text[value->size]=0;text_writes++;
}
static void* test_function(unsigned rva) {
    if(rva==0x99a0e0)return (void*)test_child;
    if(rva==0x5b150)return (void*)test_wide;
    if(rva==0x98e8a0)return (void*)test_set_text;
    assert(!"Unexpected native rendering call");return NULL;
}
static void attach_control(Control* c,byte* memory,byte* transform) {
    memset(c,0,sizeof(*c));memset(memory,0,0x100);
    uint64_t gen=7;memcpy(memory,&gen,8);c->renderer=memory+8;c->generation=gen;
    memcpy(c->renderer+0x40,&transform,8);memcpy(c->renderer+0x80,&transform,8);
}
static void content_cache(void) {
    byte memory[0x100],row_memory[0x100],transform[0xb0]={0};
    memset(&ui_state,0,sizeof(ui_state));ui_state.focus=-1;ui_state.selected=AF_ALL_STATS;
    strcpy(ui_state.room,"Attic");strcpy(ui_status,"Ready.");plan_ready=0;
    attach_control(&ui_state.panel,memory,transform);
    missing_child=1;ui_panel_content();assert(!ui_state.panel.content_ready);
    missing_child=0;ui_panel_content();assert(ui_state.panel.content_ready&&text_writes==52);
    unsigned writes=text_writes;reads=child_lookups=0;
    for(int i=0;i<600;i++)ui_panel_content();
    assert(reads==0&&child_lookups==0&&text_writes==writes);
    puts("600 unchanged panel content updates: 0 native reads, lookups or text writes");
    strcpy(ui_state.minimum[0],"12");ui_state.focus=0;ui_panel_content();
    assert(!strcmp(ui_state.panel.text[2],"12|")&&text_writes>writes);
    ui_state.focus=-1;ui_state.selected=0;include_utilities=1;ui_panel_content();
    assert(!strcmp(ui_state.panel.text[2],"12")&&!strcmp(ui_state.panel.text[1],"")&&!strcmp(ui_state.panel.text[26],"X"));
    plan_ready=1;plan_original_stats[0]=2;search.best.stats[0]=5;ui_panel_content();
    assert(!strcmp(ui_state.panel.text[4],"2.0")&&!strcmp(ui_state.panel.text[5],"5.0")&&!strcmp(ui_state.panel.text[29],"+3.0"));
    plan_ready=0;ui_panel_content();assert(!strcmp(ui_state.panel.text[5],"-"));
    af_copy(af_strings[AF_CALCULATE],AF_TEXT_BYTES,"Calculate translated");af_text_revision++;
    ui_panel_content();
    unsigned at=0;while(at<field_count&&strcmp(fields[at].name,"calculate"))at++;
    assert(at<field_count&&!wcscmp(fields[at].text,L"Calculate translated"));
    memset(af_strings,0,sizeof(af_strings));af_text_revision++;
    writes=text_writes;attach_control(&ui_state.panel,memory,transform);ui_panel_content();assert(text_writes==writes+52);
    ui_state.panel.visible=1;control_hide(&ui_state.panel);assert(!ui_state.panel.content_ready);
    ui_panel_content();assert(ui_state.panel.content_ready);
    writes=child_lookups;text_writes=0;
    control_show(&ui_state.panel,"AFShade",1,2,0.5,41);
    control_show(&ui_state.panel,"AFHint",3,4,0.5,30);
    assert(child_lookups==writes&&*(double*)(transform+0x80)==3&&*(double*)(transform+0x88)==4);
    field_count=0;attach_control(&ui_state.pins,memory,transform);attach_control(&ui_state.pin_rows[0],row_memory,transform);
    ui_state.pin_total=1;ui_state.pin_list[0]=123;strcpy(ui_state.pin_labels[0],"Chair");
    pinned_count=0;ui_pins_content();assert(ui_state.pins.content_ready&&!strcmp(ui_state.pin_rows[0].text[1],""));
    writes=text_writes;reads=child_lookups=0;
    for(int i=0;i<600;i++)ui_pins_content();
    assert(!reads&&!child_lookups&&text_writes==writes);
    puts("600 unchanged Pins content updates: 0 native reads, lookups or text writes");
    pinned_ids[0]=123;pinned_count=1;ui_state.pin_revision++;ui_pins_content();
    assert(!strcmp(ui_state.pin_rows[0].text[1],"X"));
    ui_state.pin_total=0;ui_state.pin_revision++;ui_pins_content();assert(ui_state.pins.text[19][0]);
    ui_state.pin_total=10;ui_state.pin_page=1;ui_state.pin_list[9]=456;strcpy(ui_state.pin_labels[9],"Table");
    ui_pins_content();assert(!strcmp(ui_state.pin_rows[0].text[0],"Table")&&!strcmp(ui_state.pin_rows[0].text[1],""));
    attach_control(&ui_state.pin_rows[0],row_memory,transform);missing_child=1;ui_pins_content();
    assert(!ui_state.pins.content_ready);missing_child=0;ui_pins_content();assert(ui_state.pins.content_ready);
    memset(&ui_state,0,sizeof(ui_state));ui_state.focus=-1;ui_state.selected=AF_ALL_STATS;
    pinned_count=0;include_utilities=0;plan_ready=0;
}

static unsigned char native_hit(void* button) {(void)button;hits++;return 42;}

static void piece_entry_snapshot(void) {
    byte pieces_memory[3][0x2e0]={{0}},items[2]={0};
    byte* live[]={pieces_memory[0],pieces_memory[1],pieces_memory[2]};
    byte* entries[3];byte* first=&items[0];byte* second=&items[1];
    memcpy(live[0]+0x2d8,&first,sizeof(first));
    memcpy(live[1]+0x2d8,&second,sizeof(second));
    memcpy(live[2]+0x2d8,&first,sizeof(first));
    reads=0;piece_entries(live,3,entries);
    assert(reads==3&&entries[0]==first&&entries[1]==second&&entries[2]==first);
    for(int item=0;item<400;item++) {
        unsigned matches=0;
        for(unsigned j=0;j<3;j++)matches+=entries[j]==first;
        assert(matches==2); /* Duplicate matches remain visible to the transaction guard. */
    }
    assert(reads==3);
    memcpy(live[0]+0x2d8,&second,sizeof(second));
    memset(live[2]+0x2d8,0,sizeof(first));
    piece_entries(live,3,entries);
    assert(reads==6&&entries[0]==second&&entries[1]==second&&!entries[2]);
    piece_entries(NULL,0,NULL);assert(reads==6);
    byte* unreadable=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    assert(unreadable);live[0]=unreadable;
    piece_entries(live,1,entries);assert(reads==7&&!entries[0]);
    assert(VirtualFree(unreadable,0,MEM_RELEASE));
}

static void translation_catalog(const char* path) {
    FILE* file=fopen(path,"rb");assert(file);
    char english[AF_TEXT_BYTES],translated[AF_TEXT_BYTES],formatted[AF_TEXT_BYTES],actual[AF_TEXT_BYTES];
    unsigned checked=0;
    while(fgets(english,sizeof(english),file)) {
        assert(fgets(translated,sizeof(translated),file)&&fgets(formatted,sizeof(formatted),file));
        english[strcspn(english,"\r\n")]=0;translated[strcspn(translated,"\r\n")]=0;
        formatted[strcspn(formatted,"\r\n")]=0;
        int id=0;while(id<AF_TEXT_COUNT&&strcmp(af_catalog[id].english,english))id++;
        assert(id<AF_TEXT_COUNT);
        wchar_t wide[AF_TEXT_BYTES];
        int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,translated,-1,wide,AF_TEXT_BYTES);
        assert(n>1);
        GameString value={{0},(uint64_t)n-1,7};
        if(value.size<=7)memcpy(value.data,wide,(size_t)n*2);
        else {void* storage=wide;value.capacity=value.size;memcpy(value.data,&storage,8);}
        af_copy(actual,sizeof(actual),english);
        localized_value(&value,af_catalog[id].key,actual,sizeof(actual));
        assert(!strcmp(actual,translated));
        assert(af_placeholders(actual)==af_placeholders(english));
        af_copy(af_strings[id],AF_TEXT_BYTES,actual);
        report(english);af_status(actual);
        assert(!strcmp(actual,translated)&&!strcmp(ui_status,english));
        af_format(actual,sizeof(actual),af_text(id),"2048","4096");
        assert(!strcmp(actual,formatted));
        if(id==AF_RETURNED) {
            ui_return_count=2048;af_status(actual);assert(!strcmp(actual,formatted));
            report("APPLY PASS: verified.");af_status(actual);assert(!strcmp(actual,formatted));
            ui_return_count=0;
        }
        if(id==AF_UNDO_DONE) {
            report("UNDO PASS: verified.");report("UNDO SETTLED: verified.");
            af_status(actual);assert(!strcmp(actual,translated));
        }
        checked++;
    }
    assert(!ferror(file)&&checked==AF_TEXT_COUNT*10);fclose(file);
    memset(af_strings,0,sizeof(af_strings));
    printf("Native translation round trips: %u passed.\n",checked);
}

int main(int argc,char** argv) {
    ULONGLONG deadline=GetTickCount64()+10000;
    plan_cancel=0;assert(!search_stopped(&deadline));
    plan_cancel=1;assert(search_stopped(&deadline));
    plan_cancel=0;deadline=GetTickCount64();assert(search_stopped(&deadline));
    content_cache();
    piece_entry_snapshot();
    if(argc==2)translation_catalog(argv[1]);
    else assert(argc==1);
    char label[128]="Calculate";
    GameString localized={{0},2,7};
    memcpy(localized.data,L"\x8ba1\x7b97",4);
    localized_value(&localized,"AUTO_FURNITURE_CALCULATE",label,sizeof(label));
    assert(!strcmp(label,"\xe8\xae\xa1\xe7\xae\x97"));
    wchar_t english[]=L"Calculate";void* storage=english;
    localized.size=localized.capacity=9;memcpy(localized.data,&storage,8);
    localized_value(&localized,"AUTO_FURNITURE_CALCULATE",label,sizeof(label));
    assert(!strcmp(label,"Calculate"));
    wchar_t missing[]=L"AUTO_FURNITURE_CALCULATE";storage=missing;
    localized.size=localized.capacity=wcslen(missing);memcpy(localized.data,&storage,8);
    localized_value(&localized,"AUTO_FURNITURE_CALCULATE",label,sizeof(label));
    assert(!strcmp(label,"Calculate"));
    localized.size=0;localized_value(&localized,"key",label,sizeof(label));
    assert(!strcmp(label,"Calculate"));
    localized.size=128;localized_value(&localized,"key",label,sizeof(label));
    assert(!strcmp(label,"Calculate"));
    localized.size=4;localized.capacity=1;localized_value(&localized,"key",label,sizeof(label));
    assert(!strcmp(label,"Calculate"));
    localized=(GameString){{0},1,7};
    memcpy(localized.data,L"\xd800",2);localized_value(&localized,"key",label,sizeof(label));
    assert(!strcmp(label,"Calculate"));

    char text[AF_TEXT_BYTES];wchar_t wide[AF_TEXT_BYTES];
    for(int i=0;i<200;i++)wide[i]=0x8ba1;
    wide[100]=0;storage=wide;
    localized=(GameString){{0},100,200};memcpy(localized.data,&storage,8);
    localized_value(&localized,"key",text,sizeof(text));
    assert(strlen(text)==300);
    strcpy(label,"fallback");localized_value(&localized,"key",label,sizeof(label));
    assert(!strcmp(label,"fallback"));
    wide[100]=0x8ba1;wide[200]=0;localized.size=200;
    strcpy(text,"fallback");localized_value(&localized,"key",text,sizeof(text));
    assert(!strcmp(text,"fallback"));

    const char* chinese="\xe8\xae\xa1\xe7\xae\x97";
    for(size_t cap=1;cap<12;cap++) {
        af_copy(text,cap,chinese);
        assert(strlen(text)<cap&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,wide,AF_TEXT_BYTES));
        af_format(text,cap,"%2 / %1",chinese,chinese);
        assert(strlen(text)<cap&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,wide,AF_TEXT_BYTES));
        af_format(text,cap,chinese,NULL,NULL);
        assert(strlen(text)<cap&&MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,wide,AF_TEXT_BYTES));
    }
    af_format(text,sizeof(text),"%2 / %1 / %1","one","two");
    assert(!strcmp(text,"two / one / one"));
    assert(af_placeholders("%2 / %1 / %1")==0x12);
    assert(af_placeholders("%n")==-1&&af_placeholders("%3")==-1&&af_placeholders("bad %")==-1);
    af_room_label("Floor1_Large",text);assert(!strcmp(text,"Floor 1 Left"));
    af_room_label("Floor1_Small",text);assert(!strcmp(text,"Floor 1 Right"));
    af_room_label("Floor2_Small",text);assert(!strcmp(text,"Floor 2 Left"));
    af_room_label("Floor2_Large",text);assert(!strcmp(text,"Floor 2 Right"));
    af_room_label("Basement0",text);assert(!strcmp(text,"Basement 1"));
    af_room_label("Basement4",text);assert(!strcmp(text,"Basement 5"));
    af_room_label("LargeAttic",text);assert(!strcmp(text,"Attic"));
    af_room_label("Custom_Room",text);assert(!strcmp(text,"Custom Room"));
    af_copy(af_strings[AF_READY],AF_TEXT_BYTES,chinese);
    report("Ready.");af_status(text);assert(!strcmp(text,chinese));
    assert(!strcmp(ui_status,"Ready."));
    ui_return_count=23;af_status(text);assert(!strcmp(text,"Items returned: 23."));
    report("APPLY PASS: verified.");assert(ui_return_count==23);
    report("Searching...");af_status(text);assert(!ui_return_count&&!strcmp(text,"Searching..."));
    report("Diagnostic text");af_status(text);assert(!strcmp(text,"Diagnostic text"));
    memset(af_strings,0,sizeof(af_strings));

    byte owner_memory[0x100]={0},scene[0x500]={0},entity[16]={0};
    byte renderer_memory[0x100]={0},transform[0xb0]={0};
    byte* owner=owner_memory+8;byte* renderer=renderer_memory+8;
    uint64_t gen=7;
    memcpy(owner_memory,&gen,8);memcpy(renderer_memory,&gen,8);
    memcpy(owner+0x18,&(void*){entity},8);memcpy(entity+8,&(void*){scene},8);
    memcpy(renderer+0x40,&(void*){transform},8);
    initialized=2;ui_enabled=1;original_hit=native_hit;
    ui_state.owner=owner;ui_state.scene=scene;ui_state.generation=gen;ui_state.allowed=1;
    owner[0x78]=1;

    reads=hits=0;
    for(int i=0;i<400;i++)assert(ui_hit(NULL)==42);
    printf("Closed panel, 400 hits: %u reads, %u native calls\n",reads,hits);
    assert(reads==0&&hits==400);
    ui_state.modal=1;reads=hits=0;
    assert(ui_hit(NULL)==0&&reads==4&&hits==0);
    scene[0x4b0]=1;
    assert(ui_hit(NULL)==42&&hits==1);
    scene[0x4b0]=0;ui_state.generation++;
    assert(ui_hit(NULL)==42&&hits==2);
    ui_state.generation=gen;ui_state.modal=0;

    Control* c=&ui_state.panel;c->renderer=renderer;c->generation=gen;
    control_show(c,"",1,2,0.5,42);
    assert(renderer[0x51]==1);
    reads=0;control_hide(c);
    assert(renderer[0x51]==0&&reads==2);
    reads=0;control_hide(c);assert(reads==0);
    control_show(c,"",3,4,0.5,42);
    assert(renderer[0x51]==1);
    /* A recycled renderer must not be written through a retained reference. */
    gen++;memcpy(renderer_memory,&gen,8);reads=0;control_hide(c);
    assert(renderer[0x51]==1&&reads==1);
    control_hide(c);assert(reads==1);
    c->generation=gen;control_show(c,"",3,4,0.5,42);
    renderer[15]=1;control_hide(c);assert(renderer[0x51]==1);
    renderer[15]=0;control_show(c,"",3,4,0.5,42);

    owner[0x78]=0;ui_state.modal=1;plan_cancel=0;
    reads=0;ui_update(owner);
    assert(!renderer[0x51]&&!ui_state.modal&&plan_cancel&&reads==7);
    reads=0;
    for(int i=0;i<400;i++)ui_update(owner);
    printf("Inactive UI, 400 updates: %u reads\n",reads);
    assert(reads==2000);
    /* Closing again after a re-open still hides the panel and cancels work. */
    control_show(c,"",1,2,0.5,42);ui_state.modal=1;plan_cancel=0;
    ui_close();assert(!renderer[0x51]&&!ui_state.modal&&plan_cancel);
    puts("Auto Furniture UI checks passed.");
    return 0;
}
