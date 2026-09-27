/* Full captured containers, independent of the displayed filters. Rebuilt only
 * on a grid capture or item rebind. Equipment contains unassigned items only. */
static struct {
    uint64_t view_revision,metadata_revision;
    int valid,counts[IQ_SET_LIMIT][2];
} iq_owned;
static void iq_build_ownership(void) {
    if(iq_owned.valid && iq_owned.view_revision==iq_view_revision &&
       iq_owned.metadata_revision==iq_metadata_revision)return;
    memset(&iq_owned,0,sizeof(iq_owned));
    for(int i=0;i<iq.count;i++) {
        IQDrawer* d=&iq.drawers[i];
        IQMetadata* m=iq_find_metadata(d->drawer,d->item_id);
        if(!m || !m->sets_known || d->side<0 || d->side>1)continue;
        for(int j=0;j<iq_set_count;j++) {
            uint64_t bit=UINT64_C(1)<<(j%64);
            if(!(m->sets[j/64]&bit))continue;
            iq_owned.counts[j][d->side]++;
        }
    }
    iq_owned.view_revision=iq_view_revision;iq_owned.metadata_revision=iq_metadata_revision;
    iq_owned.valid=1;
}
