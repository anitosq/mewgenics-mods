#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
static struct {uint64_t generation;void* vtable;unsigned char body[64];} object;
static int readable=1;
static int iq_read(const void* p,void* out,size_t n) {
    uintptr_t start=(uintptr_t)&object,at=(uintptr_t)p;
    if(!readable || at<start || at>start+sizeof(object) || n>start+sizeof(object)-at)return 0;
    memcpy(out,p,n);return 1;
}
#include "../src/native/inventory_lifetime.h"
int main(void) {
    object.generation=10;object.vtable=(void*)(uintptr_t)0x1234;
    IQReference r=iq_reference(&object.vtable);
    assert(iq_reference_valid(r));
    readable=0;assert(!iq_reference_valid(r));assert(!iq_reference_valid(iq_reference(&object.vtable)));
    readable=1;object.generation++;assert(!iq_reference_valid(r));
    r=iq_reference(&object.vtable);assert(iq_reference_valid(r));
    /* Destructors replace the vtable before pooled memory is recycled. */
    object.vtable=(void*)(uintptr_t)0x5678;assert(!iq_reference_valid(r));
    assert(!iq_reference_valid((IQReference){0}));
    puts("Native lifetime: live, unreadable, recycled and destructing objects checked.");
    return 0;
}
