#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#ifdef NDEBUG
#error Assertions must be enabled.
#endif
typedef int (*ApiInstall)(uintptr_t,int,void*,void**,int,const char*);
#include "../src/native/inventory_hooks.h"
static int calls,fail_at;
static int mock_install(uintptr_t rva,int stolen,void* fn,void** original,int priority,const char* name) {
    (void)rva;(void)stolen;(void)fn;(void)priority;(void)name;
    if(++calls==fail_at)return 0;
    *original=(void*)(uintptr_t)calls;
    return 1;
}
int main(void) {
    unsigned char image[12*64]={0},expected[64]={0};
    void* originals[12]={0};IQHook hooks[12];
    for(int i=0;i<12;i++)hooks[i]=(IQHook){i*64,expected,NULL,&originals[i],"test"};
    int active=1;
    image[11*64]=1;
    assert(iq_install_plan(image,hooks,12,mock_install,&active)==-1);
    assert(calls==0 && active==0); /* A late mismatch must not install earlier hooks. */
    image[11*64]=0;
    for(fail_at=1;fail_at<=12;fail_at++) {
        calls=0;active=1;
        assert(iq_install_plan(image,hooks,12,mock_install,&active)==0);
        assert(calls==fail_at && active==0);
    }
    calls=0;fail_at=0;
    assert(iq_install_plan(image,hooks,12,mock_install,&active)==1);
    assert(calls==12 && active==1);
    puts("Hook preflight and every partial-install failure leave UI inactive.");
}
