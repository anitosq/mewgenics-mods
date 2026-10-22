#include <assert.h>
#ifdef NDEBUG
#error Compile this regression test with assertions enabled (use -UNDEBUG).
#endif
#include <stdio.h>
#include <string.h>
#include "../src/native/inventory_grid.h"
#include "../src/native/inventory_set_name.h"

static void expect_set(const wchar_t* localized,const wchar_t* expected) {
    wchar_t out[160];iq_set_name(out,localized);
    if(wcscmp(out,expected)){fwprintf(stderr,L"set name mismatch: %ls\n",out);assert(0);}
}

int main(void) {
    /* Column cap: native growth up to the cap, Auto (0) never caps. */
    assert(iq_grid_columns(4,6)==4 && iq_grid_columns(9,6)==6 && iq_grid_columns(9,10)==9);
    assert(iq_grid_columns(20,0)==20 && iq_grid_columns(12,5)==5);
    int cap=5;const int cycle[]={6,7,8,10,0,5};
    for(int i=0;i<6;i++){cap=iq_grid_next(cap);assert(cap==cycle[i]);}
    assert(iq_grid_next(9)==IQ_GRID_DEFAULT);
    assert(iq_grid_equipment_rows(4)==4 && iq_grid_equipment_rows(5)==5 && iq_grid_equipment_rows(6)==5);
    assert(iq_grid_equipment_rows(8)==6 && iq_grid_equipment_rows(10)==8 && iq_grid_equipment_rows(20)==16);
    wchar_t label[32];
    iq_grid_label(label,32,10);assert(!wcscmp(label,L"Grid: 10x10"));
    iq_grid_label(label,32,0);assert(!wcscmp(label,L"Grid: Auto"));
    /* settings.ini round trip and damaged input. */
    char text[64];
    for(int i=0;i<6;i++){assert(iq_grid_format(text,sizeof(text),cycle[i])>0);assert(iq_grid_parse(text)==cycle[i]);}
    assert(iq_grid_parse("")==6 && iq_grid_parse("grid_columns=9")==6 && iq_grid_parse("grid_columns=8x")==6);
    assert(iq_grid_parse("grid_columns=-8")==6 && iq_grid_parse("grid_columns=autox")==6);
    assert(iq_grid_parse("# note\r\n  grid_columns=10\r\n")==10 && iq_grid_parse("other=1\ngrid_columns=auto")==0);
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
    return 0;
}
