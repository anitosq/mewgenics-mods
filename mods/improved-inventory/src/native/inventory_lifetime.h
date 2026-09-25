/* Game objects are pooled. A readable address alone is not a live reference.
 * iq_read is a fallible read supplied by the host (or the fixture). */
typedef struct { void* pointer; uint64_t generation; void* vtable; } IQReference;

static IQReference iq_reference(void* pointer) {
    IQReference r={0};
    if(pointer && iq_read((unsigned char*)pointer-8,&r.generation,8) &&
       iq_read(pointer,&r.vtable,sizeof(r.vtable))) r.pointer=pointer;
    return r;
}
static int iq_reference_valid(IQReference r) {
    uint64_t generation=0;void* vtable=NULL;
    return r.pointer && iq_read((unsigned char*)r.pointer-8,&generation,8) &&
        generation==r.generation && iq_read(r.pointer,&vtable,sizeof(vtable)) && vtable==r.vtable;
}
