/* SPDX-License-Identifier: GPL-3.0-only */
/* Capture only: production boot, project loader, UI painter and framebuffer.
 * Build/run on both the base and candidate through storage_warning_capture.sh.
 * No substitute painter and no asserted test result. */
int main(int argc,char **argv)
{
    if(argc!=2) return 2;
    for(int k=0;k<OBJ_NOBJ;k++)store_len[k]=-1;
    memset(&proj,0,sizeof proj);boot();
    if(project_save()) return 3;
    store[OBJ_PAT0][8]^=1u; /* committed object, invalid raw-stream CRC */
    boot();
    const int views[]={V_HOME,V_SEQ,V_PART};
    const char *names[]={"home","sequence","sound"};
    for(unsigned i=0;i<3;i++) {
        ui.part=PART_303A;set_view(views[i]);
        ui.msg_until=0;memset(blit_hash,0,sizeof blit_hash);ui_frame();
        char path[1024];snprintf(path,sizeof path,"%s/%s.png",argv[1],names[i]);
        png_write(path);
    }
    return 0;
}
