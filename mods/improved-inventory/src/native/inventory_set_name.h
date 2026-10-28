/* Short set labels from the game's localized SETBONUS_*_NAME strings.
 * Every shipped language wraps the set name in its own "Set Bonus!" phrase;
 * strip the known affixes only, so unknown text still shows unchanged. */
#include <wchar.h>
#include <wctype.h>
static int iq_set_affix(wchar_t* text,const wchar_t* affix,int suffix) {
    size_t n=wcslen(text),a=wcslen(affix);
    if(n<=a)return 0;
    wchar_t* at=suffix?text+n-a:text;
    for(size_t i=0;i<a;i++)if(towlower(at[i])!=towlower(affix[i]))return 0;
    if(suffix)*at=0;else memmove(text,text+a,(n-a+1)*sizeof(wchar_t));
    return 1;
}
static void iq_set_trim(wchar_t* text) {
    size_t n=wcslen(text),start=0;
    while(n && (iswspace(text[n-1]) || text[n-1]==L'!' || text[n-1]==0xff01))n--;
    text[n]=0;
    while(text[start]==L' ' || text[start]==0xa1)start++;
    if(start)memmove(text,text+start,(n-start+1)*sizeof(wchar_t));
}
static void iq_set_name(wchar_t* out,const wchar_t* name) {
    /* Normalize the entities and invisible breaks used by fr and zh-cn. */
    size_t n=0;
    for(const wchar_t* in=name;*in && n<159;) {
        if(!wcsncmp(in,L"&nbsp;",6)){out[n++]=L' ';in+=6;}
        else if(*in==0xa0 || *in==0x202f){out[n++]=L' ';in++;}
        else if(*in==0x200b)in++;
        else out[n++]=*in++;
    }
    out[n]=0;iq_set_trim(out);
    wchar_t original[160];wcscpy(original,out);
    static const wchar_t* prefixes[]={
        L"Bonus d'ensemble :",L"Bonus d'ensemble:",            /* fr */
        L"Bonificaci\xf3n de conjunto de ",L"Bonificaci\xf3n de conjunto ", /* sp */
        L"Setbonus:",L"Bonus Set:",                            /* de, it */
        L"B\xf4nus de Conjunto de ",L"B\xf4nus de Conjunto ",  /* pt-br */
        L"\x0411\x043e\x043d\x0443\x0441 \x043d\x0430\x0431\x043e\x0440\x0430 ",L"\x0411\x043e\x043d\x0443\x0441 " /* ru */
    };
    static const wchar_t* suffixes[]={
        L" Set Bonus",L" \x043d\x0430\x0431\x043e\x0440\x0430",        /* en, ru */
        L" \xc138\xd2b8 \xbcf4\xb108\xc2a4",L"\x30bb\x30c3\x30c8\x30dc\x30fc\x30ca\x30b9", /* ko, ja */
        L"\x5957\x88c5\x52a0\x6210"                                    /* zh-cn */
    };
    for(size_t i=0;i<sizeof(prefixes)/sizeof(*prefixes);i++)if(iq_set_affix(out,prefixes[i],0))break;
    for(size_t i=0;i<sizeof(suffixes)/sizeof(*suffixes);i++)if(iq_set_affix(out,suffixes[i],1))break;
    iq_set_trim(out);
    if(!out[0]){wcscpy(out,original);return;}
    /* The C locale's towupper covers ASCII only; fr/ru names start lowercase. */
    wchar_t c=out[0];
    if((c>=L'a' && c<=L'z') || (c>=0xe0 && c<=0xfe && c!=0xf7) || (c>=0x430 && c<=0x44f))out[0]=(wchar_t)(c-0x20);
    else if(c>=0x450 && c<=0x45f)out[0]=(wchar_t)(c-0x50);
}
