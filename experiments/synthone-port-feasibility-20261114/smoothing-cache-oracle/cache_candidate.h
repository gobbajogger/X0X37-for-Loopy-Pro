/* Isolated implementation of upstream develop's cached index traversal and
 * source-identical smoothing recurrence. Not production firmware integration. */
static unsigned build_indexes(unsigned *out) {
 unsigned n=0;
 for(unsigned i=0;i<PARAM_COUNT;i++)if(smooth_flags[i])out[n++]=i;
 return n;
}
static void prepare_cache(sp_port *p) {
 p->c2=pow(0.5,p->onedsr/p->htime);
 p->c1=1.0-p->c2;
 p->prvhtim=p->htime;
}
static float cached_tick(sp_port *p,float target) {
 return p->yt1=p->c1*target+p->c2*p->yt1;
}
