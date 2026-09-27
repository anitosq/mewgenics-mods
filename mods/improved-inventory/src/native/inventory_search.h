/* Pure search/set predicates. No game pointers or save writes. */
#include <wchar.h>
#include <wctype.h>
#include <stdint.h>
#define IQ_SET_LIMIT 256
#define IQ_QUERY_LIMIT 128
typedef struct { wchar_t query[IQ_QUERY_LIMIT]; int set_mode; uint64_t sets[4]; } IQSearch;
static int iq_contains(const wchar_t* text,const wchar_t* query,size_t length) {
    if(!length)return 1;
    for(;*text;text++) {
        size_t i=0;while(i<length && text[i] && towlower(text[i])==towlower(query[i]))i++;
        if(i==length)return 1;
    }
    return 0;
}
static int iq_query_match(const wchar_t* text,const wchar_t* query) {
    /* Every whitespace-separated term must occur, in any order. */
    while(*query) {
        while(iswspace(*query))query++;
        const wchar_t* start=query;while(*query && !iswspace(*query))query++;
        if(query!=start && !iq_contains(text,start,(size_t)(query-start)))return 0;
    }
    return 1;
}
static int iq_set_match(const IQSearch* filter,const uint64_t* sets,int known) {
    if(!filter->set_mode)return 1;
    if(!known)return 0;
    uint64_t any=0,selected=0;
    for(int i=0;i<4;i++){any|=sets[i];selected|=sets[i]&filter->sets[i];}
    return filter->set_mode==1?any!=0:filter->set_mode==2?any==0:selected!=0;
}
static void iq_plain_text(wchar_t* out,size_t capacity,const wchar_t* in) {
    size_t n=0;
    while(*in && n+1<capacity) {
        if(*in==L'[') {
            const wchar_t* end=wcschr(in,L']');
            if(end) {
                /* Icon names remain searchable (e.g. [img:shield]). */
                if(!wcsncmp(in,L"[img:",5)) {
                    out[n++]=L' ';in+=5;
                    while(in<end && n+1<capacity)out[n++]=*in++;
                }
                in=end+1;continue;
            }
        }
        out[n++]=*in++;
    }
    out[n]=0;
}
