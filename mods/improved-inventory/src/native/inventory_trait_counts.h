/* Stats/Effects menu counts over the full captured view, independent of the
 * displayed filters like the set counts. Rebuilt on a capture or rebind. */
static struct {
    uint64_t view_revision,metadata_revision;
    int valid;
    IQTraitCounts counts;
} iq_trait_counts;
static void iq_build_trait_counts(void) {
    if(iq_trait_counts.valid && iq_trait_counts.view_revision==iq_view_revision &&
       iq_trait_counts.metadata_revision==iq_metadata_revision)return;
    memset(&iq_trait_counts,0,sizeof(iq_trait_counts));
    for(int i=0;i<iq.count;i++) {
        IQMetadata* m=iq_find_metadata(iq.drawers[i].drawer,iq.drawers[i].item_id);
        if(m)iq_trait_tally(&iq_trait_counts.counts,&m->fx);
    }
    iq_trait_counts.view_revision=iq_view_revision;iq_trait_counts.metadata_revision=iq_metadata_revision;
    iq_trait_counts.valid=1;
}
