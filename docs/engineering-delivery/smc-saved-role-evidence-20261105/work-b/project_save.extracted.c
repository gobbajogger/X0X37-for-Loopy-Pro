int project_save(void)
{
#if X0X_BASS_TAGGED_SOUND
    if(engine_bass_sound_validate(&proj.sound))return -9;
    for(int v=0;v<NBASS;v++)if(engine_bass_preset_pending(v))return -8;
#elif X0X_BASS_PRESET_PREVIEW
    if(engine_bass_preview_changed()) return -9; /* no untagged preview writes */
#endif
    if (project_incompatible || !stream_layout_ok(&proj, PROJ_STREAM))
        return -9; /* not one settings/sound/pattern object may be rewritten */
    if (settings_read_failed || settings_load_failed) return -8; /* retry load before saving fallback defaults */
#ifndef X0X_HOST
    extern int store_stage_held(void);
    if (store_stage_held())                          /* boot upload is using the store buffer */
        return -8;
#endif
    capture_settings(&proj.set);
    if (measure_project(&proj, &packed_len))
        return -7;
    if (save_obj(OBJ_SET, &proj.set, sizeof proj.set))
        return -1;
    if (save_obj(OBJ_SOUND, &proj.sound, sizeof proj.sound))
        return -2;
    return save_stream();
}