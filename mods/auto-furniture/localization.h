/* Game-thread lookups only. The solver and diagnostic logs keep English text. */
#include "translations.h"
static char af_strings[AF_TEXT_COUNT][AF_TEXT_BYTES];
static unsigned af_text_revision;

static void af_copy(char* out,size_t cap,const char* text) {
    if(!cap)return;
    size_t n=strlen(text);if(n>=cap)n=cap-1;
    while(n&&((unsigned char)text[n]&0xc0)==0x80)n--;
    memcpy(out,text,n);out[n]=0;
}
static int af_placeholders(const char* text) {
    unsigned counts[2]={0};
    for(;*text;text++)if(*text=='%') {
        text++;if(*text!='1'&&*text!='2')return -1;
        if(++counts[*text-'1']>15)return -1;
    }
    return (int)(counts[0]|(counts[1]<<4));
}
static void af_format(char* out,size_t cap,const char* format,const char* first,const char* second) {
    if(!cap)return;
    size_t used=0;
    for(const char* p=format;*p&&used+1<cap;) {
        const char* value=NULL;
        if(p[0]=='%'&&p[1]=='1')value=first;
        else if(p[0]=='%'&&p[1]=='2')value=second;
        if(value) {af_copy(out+used,cap-used,value);used+=strlen(out+used);p+=2;}
        else out[used++]=*p++;
    }
    out[used]=0;
    /* Drop an incomplete UTF-8 character at the output boundary. */
    if(used==cap-1) {
        size_t start=used;
        while(start&&((unsigned char)out[start-1]&0xc0)==0x80)start--;
        if(start) {
            unsigned char lead=(unsigned char)out[start-1];
            size_t bytes=lead>=0xf0?4:lead>=0xe0?3:lead>=0xc0?2:1;
            if(used-(start-1)<bytes)out[start-1]=0;
        }
    }
}
static void localized_value(const GameString* value,const char* key_text,char* out,size_t cap) {
    if(!value->size||value->size>=AF_TEXT_BYTES||value->size>value->capacity)return;
    wchar_t wide[AF_TEXT_BYTES]={0};char translated[AF_TEXT_BYTES];
    if(!read_bytes(value->capacity>7?ptr(value,0):value->data,wide,(size_t)value->size*2))return;
    if(wcslen(wide)!=value->size)return;
    int n=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide,-1,translated,sizeof(translated),NULL,NULL);
    if(n&&((size_t)n<=cap)&&translated[0]&&strcmp(translated,key_text))af_copy(out,cap,translated);
}
static void localize(const char* key_text,char* out,size_t cap) {
    GameString key={{0},strlen(key_text),15},value={{0},0,7};
    if(key.size<=15)memcpy(key.data,key_text,key.size+1);
    else {memcpy(key.data,&key_text,8);key.capacity=key.size;}
    FN(void*(*)(void*,void*,void*),0x962430)(base+0x13c5530,&value,&key);
    localized_value(&value,key_text,out,cap);
    FN(void(*)(void*),0x52290)(&value);
}
static const char* af_text(int id) {
    return af_strings[id][0]?af_strings[id]:af_catalog[id].english;
}
static void af_load_texts(void) {
    for(int i=0;i<AF_TEXT_COUNT;i++) {
        af_copy(af_strings[i],AF_TEXT_BYTES,af_catalog[i].english);
        localize(af_catalog[i].key,af_strings[i],AF_TEXT_BYTES);
        if(af_placeholders(af_strings[i])!=af_placeholders(af_catalog[i].english)||
           strchr(af_strings[i],'[')||strchr(af_strings[i],']'))
            af_copy(af_strings[i],AF_TEXT_BYTES,af_catalog[i].english);
    }
    af_text_revision++;
}
static void af_status(char out[AF_TEXT_BYTES]) {
    if(ui_return_count) {
        char count[24];snprintf(count,sizeof(count),"%u",ui_return_count);
        af_format(out,AF_TEXT_BYTES,af_text(AF_RETURNED),count,NULL);return;
    }
    for(int i=0;i<AF_TEXT_COUNT;i++)if(!strcmp(ui_status,af_catalog[i].english)) {
        af_copy(out,AF_TEXT_BYTES,af_text(i));return;
    }
    af_copy(out,AF_TEXT_BYTES,ui_status);
}
static void af_room_label(const char* room,char out[AF_TEXT_BYTES]) {
    const char* floor=NULL;int side=AF_FLOOR_LEFT;
    if(!strcmp(room,"Floor1_Large"))floor="1";
    else if(!strcmp(room,"Floor1_Small")){floor="1";side=AF_FLOOR_RIGHT;}
    else if(!strcmp(room,"Floor2_Small"))floor="2";
    else if(!strcmp(room,"Floor2_Large")){floor="2";side=AF_FLOOR_RIGHT;}
    if(floor){af_format(out,AF_TEXT_BYTES,af_text(side),floor,NULL);return;}
    if(!strcmp(room,"Attic")||!strcmp(room,"SmallAttic")||!strcmp(room,"LargeAttic")) {
        af_copy(out,AF_TEXT_BYTES,af_text(AF_ATTIC));return;
    }
    if(!strncmp(room,"Basement",8)&&room[8]>='0'&&room[8]<='4'&&!room[9]) {
        char level[2]={(char)(room[8]+1),0};
        af_format(out,AF_TEXT_BYTES,af_text(AF_BASEMENT),level,NULL);return;
    }
    af_copy(out,AF_TEXT_BYTES,room);
    for(char* p=out;*p;p++)if(*p=='_')*p=' ';
}
