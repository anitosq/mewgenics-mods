/* Mod-owned control visibility, independent of drawer animation and input. */
typedef struct {
    void* renderer;
    uint64_t generation;
    IQReference reference;
    wchar_t cached[32][200];
    void* feedback[2][18];
    int feedback_count,hidden;
} IQControl;

static int iq_control_valid(IQControl* c) {return iq_reference_valid(c->reference);}
static void iq_control_hide(IQControl* c) {
    if(c->hidden || !iq_control_valid(c))return;
    *((unsigned char*)c->renderer+0x51)=0;
    c->hidden=1;
}
