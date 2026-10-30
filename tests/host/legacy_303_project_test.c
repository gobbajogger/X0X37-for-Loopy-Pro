/* SPDX-License-Identifier: GPL-3.0-only */
/* A project saved by the removed bass303 engine (sound rev 10, streams 3
 * and 6) must load safely in the Chicago/Miami build: no crash, the part
 * falls back to the Chicago power-on sound, and the old file stays on the
 * store untouched (no implicit save). */
#define main simulator_main
#include "../../host/x0x_host.c"
#undef main
static unsigned checks;
#define CHECK(x) do{checks++;if(!(x)){fprintf(stderr,"FAIL legacy-303 line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t legacy[65536];
static sound_t chicago_default;

static size_t read_fixture(const char *path)
{
    FILE *f=fopen(path,"rb"); CHECK(f!=0);
    size_t n=fread(legacy,1,sizeof legacy,f); fclose(f);
    CHECK(n>32+sizeof(settings_t)); return n;
}
static void check_chicago_default(void)
{
    CHECK(!memcmp(proj.sound.v[T_303],chicago_default.v[T_303],sizeof proj.sound.v[T_303]));
    engine_init(proj.pat,&proj.arr.song,proj.arr.lane,&proj.sound);
    engine_apply_sound(&proj.sound);
    int32_t audio[512]; for(int i=0;i<64;i++) engine_render(audio,256);   /* renders; no crash */
}
static void file_import(size_t n)
{
    /* Editor/web import: the inner file is rejected, the stage is not adopted. */
    uint8_t h[32]; memcpy(h,legacy,32);
    size_t body=n-32; if(body>sizeof project_stage) body=sizeof project_stage;
    memset(&project_stage,0,sizeof project_stage); memcpy(&project_stage,legacy+32,body);
    CHECK(project_file_validate(h)!=0);
    project_defaults(); check_chicago_default();
}
static void flash_load(void)
{
    /* Device boot: old settings/sound objects on the store. */
    uint32_t fmt=get32(legacy+16);
    uint8_t buf[4+sizeof(settings_t)+sizeof(sound_t)];
    put32(buf,fmt); memcpy(buf+4,legacy+32,sizeof(settings_t));
    CHECK(!plat_store_save(OBJ_SET,buf,4+sizeof(settings_t)));
    put32(buf,fmt); memcpy(buf+4,legacy+32+sizeof(settings_t),sizeof(sound_t)-64);
    CHECK(!plat_store_save(OBJ_SOUND,buf,4+sizeof(sound_t)-64));
    unsigned writes=store_writes;
    CHECK(project_load()!=0);
    CHECK(project_storage_unsupported());
    check_chicago_default();
    CHECK(store_writes==writes);                                   /* old file kept */
}
int main(int argc,char **argv)
{
    CHECK(argc>=2);
    engine_sound_defaults(&chicago_default);
    for(int i=1;i<argc;i++){ size_t n=read_fixture(argv[i]); file_import(n); flash_load(); }
    printf("ok legacy-303-project: %u checks, %d fixtures map to the Chicago default\n",checks,argc-1);
    return 0;
}
