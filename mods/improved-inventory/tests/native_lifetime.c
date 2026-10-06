#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
static struct {uint64_t generation;void* vtable;unsigned char body[96];} object;
static int readable=1,reads;
static size_t readable_size=sizeof(object);
static int iq_read(const void* p,void* out,size_t n) {
    uintptr_t start=(uintptr_t)&object,at=(uintptr_t)p;
    reads++;
    if(!readable || at<start || at>start+readable_size || n>start+readable_size-at)return 0;
    memcpy(out,p,n);return 1;
}
#include "../src/native/inventory_lifetime.h"
#include "../src/native/inventory_control_lifetime.h"
static void check_control_hide(void) {
    IQControl c={0};
    unsigned char* renderer=(unsigned char*)&object.vtable;
    reads=0;iq_control_hide(&c);assert(!reads && !c.hidden);
    c.renderer=renderer;c.reference=iq_reference(renderer);
    renderer[0x51]=1;reads=0;iq_control_hide(&c);
    assert(reads==1 && c.hidden && !renderer[0x51]);
    reads=0;for(int i=0;i<600;i++)iq_control_hide(&c);
    assert(!reads && !renderer[0x51]);
    /* Every show attempt clears hidden, including renderer recreation. */
    c.hidden=0;renderer[0x51]=1;reads=0;iq_control_hide(&c);
    assert(reads==1 && c.hidden && !renderer[0x51]);
    c.hidden=0;renderer[0x51]=1;readable=0;reads=0;iq_control_hide(&c);
    assert(reads==1 && !c.hidden && renderer[0x51]==1);
    readable=1;object.generation++;reads=0;iq_control_hide(&c);
    assert(reads==1 && !c.hidden && renderer[0x51]==1);
    c.reference=iq_reference(renderer);reads=0;iq_control_hide(&c);
    assert(reads==1 && c.hidden && !renderer[0x51]);
    c.hidden=0;renderer[0x51]=1;object.vtable=(void*)(uintptr_t)0x9abc;
    reads=0;iq_control_hide(&c);
    assert(reads==1 && !c.hidden && renderer[0x51]==1);
}
int main(void) {
    object.generation=10;object.vtable=(void*)(uintptr_t)0x1234;
    IQReference r=iq_reference(&object.vtable);
    assert(reads==1);reads=0;
    assert(iq_reference_valid(r));assert(reads==1);
    readable_size=8;assert(!iq_reference_valid(r));assert(!iq_reference(&object.vtable).pointer);
    readable_size=sizeof(object);
    readable=0;assert(!iq_reference_valid(r));assert(!iq_reference_valid(iq_reference(&object.vtable)));
    readable=1;object.generation++;assert(!iq_reference_valid(r));
    r=iq_reference(&object.vtable);assert(iq_reference_valid(r));
    /* Destructors replace the vtable before pooled memory is recycled. */
    object.vtable=(void*)(uintptr_t)0x5678;assert(!iq_reference_valid(r));
    reads=0;assert(!iq_reference_valid((IQReference){0}));assert(!reads);
    check_control_hide();
    puts("Native lifetime: live, unreadable, recycled and destructing objects checked.");
    return 0;
}
