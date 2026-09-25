#include <assert.h>
#include "../src/native/inventory_search.h"
int main(void) {
    wchar_t clean[200];
    iq_plain_text(clean,200,L"[c:ff0000]Vision Plugin[/c] +[img:shield] Bionic Set");
    assert(iq_query_match(clean,L"bionic SHIELD"));
    assert(iq_query_match(clean,L"  plugin  vision "));
    assert(!iq_query_match(clean,L"bionic poison"));
    assert(!iq_query_match(clean,L"ff0000"));
    assert(iq_query_match(clean,L"   "));
    iq_plain_text(clean,5,L"1234567");assert(!wcscmp(clean,L"1234"));
    uint64_t sets[4]={5,0,0,0},none[4]={0};IQSearch f={0};
    assert(iq_set_match(&f,none,0));
    f.set_mode=1;assert(iq_set_match(&f,sets,1));assert(!iq_set_match(&f,none,1));
    f.set_mode=2;assert(iq_set_match(&f,none,1));assert(!iq_set_match(&f,none,0));
    f.set_mode=3;f.sets[0]=6;assert(iq_set_match(&f,sets,1));
    f.sets[0]=2;assert(!iq_set_match(&f,sets,1));
    f.sets[3]=UINT64_C(1)<<63;sets[3]=UINT64_C(1)<<63;assert(iq_set_match(&f,sets,1));
    assert(!iq_set_match(&f,sets,0));
    return 0;
}
