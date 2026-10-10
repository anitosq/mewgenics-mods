#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "../src/native/inventory_filter.h"
#include "../src/native/inventory_search.h"
enum { IQ_RARITY_COUNT=5, IQ_POPUP_COUNT=IQ_RARITY_COUNT+IQ_TYPE_COUNT+4, IQ_SET_PAGE=7 };
static struct {int equipment,items[2],row[2],columns[2];double span[2];} iq;
static IQFilter active_filter;
static IQSearch iq_search;
static int filter_popup,iq_focus,iq_cursor,iq_select_all,iq_set_count,iq_set_offset;
static uint64_t iq_view_revision,iq_metadata_revision;
static wchar_t iq_set_query[IQ_QUERY_LIMIT];
static struct {wchar_t name[160];} iq_sets[IQ_SET_LIMIT];
static struct {int counts[IQ_SET_LIMIT][2];} iq_owned;
static void iq_build_ownership(void) {}
#include "../src/native/inventory_set_rows.h"
#include "../src/native/inventory_label_cache.h"

/* Fake native rendering; exercise the production toolbar update and cache. */
typedef struct {void* renderer;int hidden;double x,y;wchar_t text[32][200];} IQControl;
static struct {
    IQControl bars[IQ_TYPE_COUNT*IQ_RARITY_COUNT],popups[IQ_POPUP_COUNT],arrows[2][2],search[2],count;
    double offset_x,offset_y,scale,grid_x[2],grid_y[2];
} iq_controls;
static int open=1,text_calls,formats,feedback,shows,disabled_lookups,missing_text,missing_child;
static IQControl* recreate;
static double panel[2][2];
static unsigned char disabled[2][16];
static const double iq_rarity_popup_scale=0.7;
typedef struct {const char* name;} IQGameString;
static IQGameString iq_borrow(const char* name) {return (IQGameString){name};}
static void* __cdecl child_lookup(void* clip,void* key) {
    (void)clip;disabled_lookups++;
    return missing_child?NULL:disabled[!strcmp(((IQGameString*)key)->name,"disabled1")];
}
static unsigned char* game_base;
static int iq_is_open(void) {return open;}
static void iq_focus_set(int focus) {iq_focus=focus;iq_cursor=iq_select_all=0;}
static void iq_control_hide(IQControl* c) {c->hidden=1;}
static void iq_hide_all(void) {
    for(int i=0;i<IQ_POPUP_COUNT;i++)iq_control_hide(&iq_controls.popups[i]);
    for(int i=0;i<2;i++)iq_control_hide(&iq_controls.search[i]);
}
static int iq_popup_index(void) {return filter_popup==1?active_filter.rarity:filter_popup==2?IQ_RARITY_COUNT+active_filter.type:IQ_RARITY_COUNT+IQ_TYPE_COUNT+iq_search.set_mode;}
static int iq_rows(int side) {return iq.equipment?5:iq.columns[side];}
static int iq_scroll_limit(int side) {return iq_last_row(iq.items[side],iq.columns[side],iq_rows(side));}
static void* iq_panel_transform(int side) {return panel[side];}
static double iq_double(void* p,size_t off) {return ((double*)p)[off==0x88];}
static void* iq_ptr(void* p,size_t off) {(void)off;return p;}
static void iq_write_double(void* p,size_t off,double v) {(void)p;(void)off;(void)v;}
static double iq_control_units(void) {return 1;}
static void iq_control_show(IQControl* c,const char* name,double x,double y) {
    (void)name;shows++;
    if(!c->renderer || recreate==c) {
        c->renderer=c;memset(c->text,0,sizeof(c->text));iq_label_cache.valid=0;recreate=NULL;
    }
    c->hidden=0;c->x=x;c->y=y;
}
static int iq_text(IQControl* c,int slot,const char* name,const wchar_t* text) {
    (void)name;text_calls++;
    if(missing_text && slot==8)return 0;
    wcscpy(c->text[slot],text);return 1;
}
static void iq_set_name(wchar_t* out,const wchar_t* in) {wcscpy(out,in);}
static void iq_rarity_art(double x,double y,double scale) {(void)x;(void)y;(void)scale;}
static void iq_feedback_update(void) {feedback++;}
static int counted_format(wchar_t* out,size_t n,const wchar_t* format,...) {
    formats++;va_list args;va_start(args,format);int result=vswprintf(out,n,format,args);va_end(args);return result;
}
#define swprintf counted_format
#include "../src/native/inventory_ui_update.h"
#undef swprintf
static void changed(void) {
    text_calls=0;iq_ui_update();assert(text_calls>0 && iq_labels_match());
    text_calls=formats=0;iq_ui_update();assert(!text_calls && !formats);
}
int main(void) {
    game_base=(unsigned char*)((uintptr_t)child_lookup-0x99a0e0);
    iq.items[0]=400;iq.items[1]=113;iq.columns[0]=iq.columns[1]=6;
    iq.span[0]=iq.span[1]=600;iq_controls.scale=1;
    iq_set_count=10;
    for(int i=0;i<10;i++){swprintf(iq_sets[i].name,160,L"Set %02d",i);iq_owned.counts[i][0]=i+1;}
    iq_ui_update();assert(text_calls==3);
    text_calls=formats=feedback=shows=0;
    for(int frame=0;frame<600;frame++){panel[0][0]=frame;iq_ui_update();}
    assert(!text_calls && !formats && feedback==600 && shows>=1800);
    assert(iq_controls.search[0].x==599);
    puts("600 unchanged toolbar updates: 0 text submissions / label formats; motion and feedback retained.");
    iq_focus=1;changed();assert(!wcscmp(iq_controls.search[1].text[0],L"|"));
    wcscpy(iq_search.query,L"abc");iq_cursor=3;changed();assert(!wcscmp(iq_controls.search[1].text[0],L"abc|"));
    iq_cursor=1;changed();assert(!wcscmp(iq_controls.search[1].text[0],L"a|bc"));
    iq_select_all=1;changed();assert(!wcscmp(iq_controls.search[1].text[0],L"> abc <"));
    active_filter.type=1;changed();active_filter.rarity=2;changed();
    iq.items[0]=399;iq.items[1]=114;changed();assert(wcsstr(iq_controls.count.text[0],L"399"));
    iq_view_revision++;changed();iq_metadata_revision++;changed();
    filter_popup=3;iq_focus=0;changed();
    IQControl* popup=&iq_controls.popups[iq_popup_index()];
    assert(!wcscmp(popup->text[1],L"[ ] Set 00") && !wcscmp(popup->text[8],L"1"));
    text_calls=formats=feedback=disabled_lookups=0;
    for(int frame=0;frame<600;frame++)iq_ui_update();
    assert(!text_calls && !formats && disabled_lookups==1200 && feedback==600);
    puts("600 unchanged set-popup updates: 0 text submissions / label formats; native button-state updates retained.");
    iq_set_offset=999;changed();assert(iq_set_offset==3);
    assert(!wcscmp(popup->text[1],L"[ ] Set 03") && (disabled[1][8]&0x20));
    wcscpy(iq_set_query,L"Set 00");changed();assert(iq_set_offset==0);
    iq_search.set_mode=3;iq_search.sets[0]=1;changed();popup=&iq_controls.popups[iq_popup_index()];
    assert(!wcscmp(popup->text[1],L"[x] Set 00"));
    iq_owned.counts[0][0]=9;iq_metadata_revision++;changed();assert(!wcscmp(popup->text[8],L"9"));
    iq_set_count++;wcscpy(iq_sets[10].name,L"Set 10");changed();
    wcscpy(iq_set_query,L"absent");changed();assert(!wcscmp(popup->text[1],L"No matching sets") && !popup->text[8][0]);
    iq_set_query[0]=0;changed();
    recreate=popup;changed();assert(!wcscmp(popup->text[1],L"[x] Set 00"));
    recreate=&iq_controls.search[0];changed();
    recreate=&iq_controls.count;changed();
    missing_text=1;iq_metadata_revision++;iq_ui_update();assert(!iq_label_cache.valid);
    text_calls=0;iq_ui_update();assert(text_calls>0 && !iq_label_cache.valid);
    missing_text=0;changed();
    missing_child=1;iq_set_offset=1;iq_ui_update();assert(iq_label_cache.valid);
    missing_child=0;text_calls=0;iq_ui_update();assert(!text_calls && !(disabled[0][8]&0x20));
    iq.equipment=1;changed();assert(iq_controls.count.hidden);
    assert(!wcscmp(popup->text[23],L"Available") && !popup->text[24][0] && !popup->text[15][0]);
    open=0;iq_focus=2;iq_ui_update();assert(!iq_focus && !filter_popup && popup->hidden);
    open=1;changed();filter_popup=3;changed();
    iq.equipment=0;iq.items[0]=iq.items[1]=0;iq_view_revision++;changed();
    assert(wcsstr(iq_controls.count.text[0],L"No matches"));
    puts("Toolbar content: typing, selection, transfers, metadata, paging, recreation, delayed children, equipment and reopen passed.");
}
