/* Game objects are pooled. A readable address alone is not a live reference.
 * iq_read is a fallible read supplied by the host (or the fixture). */
typedef struct { void* pointer; uint64_t generation; void* vtable; } IQReference;

static IQReference iq_reference(void* pointer) {
    IQReference r={0};
    /* Generation and vtable are adjacent in the supported x64 object layout. */
    struct {uint64_t generation;void* vtable;} identity;
    if(pointer && iq_read((unsigned char*)pointer-8,&identity,sizeof(identity)))
        r=(IQReference){pointer,identity.generation,identity.vtable};
    return r;
}
static int iq_reference_valid(IQReference r) {
    IQReference current=iq_reference(r.pointer);
    return current.pointer && current.generation==r.generation && current.vtable==r.vtable;
}
