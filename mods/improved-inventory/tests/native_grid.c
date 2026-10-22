#include <assert.h>
#ifdef NDEBUG
#error Compile this regression test with assertions enabled (use -UNDEBUG).
#endif
#include <stdio.h>
#include <string.h>
#include "../src/native/inventory_grid.h"
#include "../src/native/inventory_set_name.h"
#include "../src/native/inventory_adventure_probe.h"

static void expect_set(const wchar_t* localized,const wchar_t* expected) {
    wchar_t out[160];iq_set_name(out,localized);
    if(wcscmp(out,expected)){fwprintf(stderr,L"set name mismatch: %ls\n",out);assert(0);}
}

int main(void) {
    /* Column cap: native growth up to the cap, Auto (0) never caps. */
    assert(iq_grid_columns(4,6)==4 && iq_grid_columns(9,6)==6 && iq_grid_columns(9,10)==9);
    assert(iq_grid_columns(20,0)==20 && iq_grid_columns(12,5)==5);
    /* Storage cycles fixed sizes only; Trash adds Auto. */
    int cap=5;const int storage[]={6,7,8,10,5},trash[]={6,7,8,10,0,5};
    for(int i=0;i<5;i++){cap=iq_grid_next(0,cap);assert(cap==storage[i]);}
    for(int i=0;i<6;i++){cap=iq_grid_next(1,cap);assert(cap==trash[i]);}
    assert(iq_grid_next(0,9)==IQ_GRID_DEFAULT && iq_grid_next(0,0)==IQ_GRID_DEFAULT);
    assert(iq_grid_equipment_rows(4)==4 && iq_grid_equipment_rows(5)==5 && iq_grid_equipment_rows(6)==5);
    assert(iq_grid_equipment_rows(8)==6 && iq_grid_equipment_rows(10)==8 && iq_grid_equipment_rows(20)==16);
    wchar_t label[32];
    iq_grid_label(label,32,0,10);assert(!wcscmp(label,L"Storage 10x10"));
    iq_grid_label(label,32,1,7);assert(!wcscmp(label,L"Trash 7x7"));
    iq_grid_label(label,32,1,0);assert(!wcscmp(label,L"Trash Auto"));
    /* settings.ini round trip and damaged input. */
    char text[96];
    for(int i=0;i<6;i++) {
        int caps[2]={storage[i%5],trash[i]};
        assert(iq_grid_format(text,sizeof(text),caps)>0);
        assert(iq_grid_parse(text,0)==caps[0] && iq_grid_parse(text,1)==caps[1]);
    }
    assert(iq_grid_parse("",0)==6 && iq_grid_parse("trash_columns=9",1)==6 && iq_grid_parse("storage_columns=8x",0)==6);
    assert(iq_grid_parse("trash_columns=-8",1)==6 && iq_grid_parse("trash_columns=autox",1)==6);
    assert(iq_grid_parse("storage_columns=auto",0)==6 && iq_grid_parse("trash_columns=auto",0)==6);
    assert(iq_grid_parse("# note\r\n  storage_columns=10\r\n",0)==10 && iq_grid_parse("other=1\ntrash_columns=auto",1)==0);
    puts("Grid sizes, equipment rows, labels and settings parsing passed.");

    /* SETBONUS_*_NAME strings from the supported build, one per game language. */
    expect_set(L"Alloy Set Bonus!",L"Alloy");
    expect_set(L"Advanced Alloy Set Bonus!",L"Advanced Alloy");
    expect_set(L"Bonus d'ensemble&nbsp;: alliage&nbsp;!",L"Alliage");
    expect_set(L"Bonus d'ensemble\xa0: alliage avanc\xe9\xa0!",L"Alliage avanc\xe9");
    expect_set(L"Bonus d'ensemble&nbsp;: \xe9pineux&nbsp;!",L"\xc9pineux");
    /* Split literals: \xa1 would otherwise absorb the following hex letter. */
    expect_set(L"\xa1" L"Bonificaci\xf3n de conjunto de aleaci\xf3n!",L"Aleaci\xf3n");
    expect_set(L"\xa1" L"Bonificaci\xf3n de conjunto ancestral!",L"Ancestral");
    expect_set(L"\xa1" L"bonificaci\xf3n de conjunto de beb\xe9!",L"Beb\xe9");
    expect_set(L"Setbonus: Legierte R\xfcstung!",L"Legierte R\xfcstung");
    expect_set(L"Bonus Set: Lega Avanzata!",L"Lega Avanzata");
    expect_set(L"B\xf4nus de Conjunto de Liga!",L"Liga");
    expect_set(L"B\xf4nus de Conjunto Ancestral!",L"Ancestral");
    /* ru: "<Alloy> set bonus" and "set of the <Ancestor> bonus" word orders. */
    expect_set(L"\x0411\x043e\x043d\x0443\x0441 \x0421\x043f\x043b\x0430\x0432\x043d\x043e\x0433\x043e \x043d\x0430\x0431\x043e\x0440\x0430!",
               L"\x0421\x043f\x043b\x0430\x0432\x043d\x043e\x0433\x043e");
    expect_set(L"\x0411\x043e\x043d\x0443\x0441 \x043d\x0430\x0431\x043e\x0440\x0430 \x041f\x0440\x0435\x0434\x043a\x0430!",
               L"\x041f\x0440\x0435\x0434\x043a\x0430");
    /* ko, ja and zh-cn (zero-width spaces between words) for "Alloy" and similar. */
    expect_set(L"\xd569\xae08 \xc138\xd2b8 \xbcf4\xb108\xc2a4!",L"\xd569\xae08");
    expect_set(L"\x5408\x91d1\x30bb\x30c3\x30c8\x30dc\x30fc\x30ca\x30b9\xff01",L"\x5408\x91d1");
    expect_set(L"\x5408\x91d1\x200b\x5957\x88c5\x52a0\x6210\xff01",L"\x5408\x91d1");
    expect_set(L"\x795e\x5947\x200b\x5957\x200b\x88c5\x200b\x52a0\x6210\xff01",L"\x795e\x5947");
    /* Unknown wording and fallback IDs stay readable rather than empty. */
    expect_set(L"cowboy_set",L"Cowboy_set");
    expect_set(L"Set Bonus!",L"Set Bonus");
    expect_set(L"",L"");
    puts("Localized set names: en, fr, sp, de, it, pt-br, ru, ko, ja and zh-cn passed.");
    /* Beta probe de-duplication: bounded, first sighting only. */
    static IQSeen seen;
    assert(iq_seen_insert(&seen,1) && !iq_seen_insert(&seen,1));
    for(uint64_t k=2;k<=IQ_SEEN_CAPACITY;k++)assert(iq_seen_insert(&seen,k));
    assert(!iq_seen_insert(&seen,IQ_SEEN_CAPACITY+1) && seen.count==IQ_SEEN_CAPACITY);
    assert(iq_probe_hash("IQBar00",0)==iq_probe_hash("IQBar00",0) && iq_probe_hash("IQBar00",0)!=iq_probe_hash("IQBar00",0x34be50));
    assert(iq_probe_hash("a",0)!=iq_probe_hash("b",0) && iq_probe_hash(NULL,0)==iq_probe_hash("",0));
    puts("Adventure probe de-duplication passed.");
    return 0;
}
