static unsigned char __cdecl iq_button_hit(void* button) {
    if(layout_test) {
        int at=iq_index_get(&iq_button_index,button);
        if(at>=0 && at<iq.count) {
            IQDrawer* d=&iq.drawers[at];
            /* Most native buttons are unrelated. Do not inspect every drawer
             * (or even the scene) for those calls. Never trust cached pointers
             * alone when suppressing a matching native button. */
            if(d->button==button && (filter_popup || !iq_visible(d)) &&
               iq_is_open() && iq_reference_valid(d->reference) &&
               iq_drawer_button(d->drawer)==button) {
                uint64_t id=0;
                if(iq_read((unsigned char*)d->drawer+(iq.equipment?0x58:0x68),&id,8) &&
                   id==d->item_id)return 0;
            }
        }
    }
    return original_button_hit(button);
}
