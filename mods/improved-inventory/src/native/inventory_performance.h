/* Beta-only sampled callback timing. This is not a frame-time/FPS measurement. */
#if IQ_PROFILE_BUILD
static struct {
    uint64_t calls,samples,start_skips,passes;
    LONGLONG ticks,max_ticks,frequency;
    unsigned depth;
} iq_perf;
static void iq_profile_update(void* instance) {
    if(iq_perf.depth || !layout_test || iq.equipment || iq_index_get(&iq_drawer_index,instance)<0) {
        iq_update_body(instance);return;
    }
    if(!iq_perf.frequency) {
        LARGE_INTEGER frequency;
        if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0){iq_update_body(instance);return;}
        iq_perf.frequency=frequency.QuadPart;
    }
    iq_perf.depth++;
    /* Prime sampling interval avoids selecting the same cells in common grids. */
    int sample=++iq_perf.calls%61==0;LARGE_INTEGER start,end;
    if(sample && !QueryPerformanceCounter(&start))sample=0;
    iq_update_body(instance);
    if(sample && QueryPerformanceCounter(&end)) {
        LONGLONG ticks=end.QuadPart-start.QuadPart;
        if(ticks>=0){iq_perf.samples++;iq_perf.ticks+=ticks;if(ticks>iq_perf.max_ticks)iq_perf.max_ticks=ticks;}
    }
    iq_perf.depth--;
    if(iq.count && instance==iq.drawers[0].drawer && ++iq_perf.passes%300==0) {
        char message[256];
        snprintf(message,sizeof(message),"Inventory beta timing: %llu callbacks, %llu hidden skips; %llu samples, avg %.2f us, max %.2f us (callback only, not FPS).",
            (unsigned long long)iq_perf.calls,(unsigned long long)(iq_hidden_skips-iq_perf.start_skips),(unsigned long long)iq_perf.samples,
            iq_perf.samples?1e6*(double)iq_perf.ticks/(double)iq_perf.frequency/(double)iq_perf.samples:0,
            1e6*(double)iq_perf.max_ticks/(double)iq_perf.frequency);
        report(message);
        iq_perf.calls=iq_perf.samples=iq_perf.passes=0;iq_perf.ticks=iq_perf.max_ticks=0;
        iq_perf.start_skips=iq_hidden_skips;
    }
}
#endif
