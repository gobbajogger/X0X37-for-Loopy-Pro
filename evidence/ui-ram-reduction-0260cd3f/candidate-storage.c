extern int ble_central_role;
extern struct { char nm[21]; } ble_r;
extern unsigned plat_ble_peer_names(char names[2][21]);
const char *plat_ble_name(char *names)
{
    if (!ble_central_role) return ble_r.nm;
    char peers[2][21];
    unsigned count = plat_ble_peer_names(peers), n = 0;
    for (unsigned p = 0; p < count; p++) {
        if (p) { names[n++] = ','; names[n++] = ' '; }
        for (unsigned i = 0; peers[p][i]; i++) names[n++] = peers[p][i];
    }
    names[n] = 0;
    return names;
}
typedef unsigned char uint8_t;
typedef struct {const char *name;uint8_t max,def;const char *const *names;} x0x_param_t;
const x0x_param_t *name_descriptor(void) {
                                   /* read only: formatted into the caller's value buffer */
        static const x0x_param_t d = {"NAME", 0, 0, 0};
        return &d;
}
