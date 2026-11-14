#define SMK_KEY_F3 5u
#define SMK_EXTQ 32u
static struct { uint32_t notes,knotes,pnotes,notes_pressed; uint8_t key_vel[37]; uint64_t keys; } fm1_in;
static volatile uint32_t smk_extq[SMK_EXTQ][2]; /* generation, physical/velocity/pitch */
static volatile uint8_t smk_extq_w, smk_extq_r, smk_extq_overflow, smk_ext_enabled;
static volatile uint32_t smk_ext_epoch;
static volatile int8_t smk_ext_oct;

static void smk__extq(uint32_t k, uint32_t vel)
{
    uint8_t w = smk_extq_w;
    if ((uint8_t)(w + 1u) % SMK_EXTQ == smk_extq_r) {
        smk_extq_overflow=1;return;
    }
    smk_extq[w][0]=smk_ext_enabled?smk_ext_epoch:0;
    smk_extq[w][1] = PLAT_OUTER_PACK(k, vel, 36 + (int)k + 12 * smk_ext_oct);
    smk_extq_w = (uint8_t)((w + 1u) % SMK_EXTQ);
}
static void smk__key_event(uint32_t k, uint32_t vel)     /* vel 0 = release */
{
    if (vel)
        fm1_in.key_vel[k] = (uint8_t)vel;
    if (k >= SMK_KEY_F3 && k < SMK_KEY_F3 + 27u) {
        uint32_t bit = 1u << (k - SMK_KEY_F3);
        if (vel) {
            fm1_in.knotes |= bit;
            fm1_in.notes_pressed |= bit;
        } else {
            fm1_in.knotes &= ~bit;
        }
        fm1_in.notes = fm1_in.knotes | fm1_in.pnotes;
    } else {
        smk__extq(k, vel);
    }
    if (vel)
        fm1_in.keys |= (uint64_t)1u << k;
    else
        fm1_in.keys &= ~((uint64_t)1u << k);
}
static unsigned irq_depth;
static uint32_t irq_save(void){return irq_depth++;}
static void irq_restore(uint32_t f){irq_depth=f;}
uint64_t plat_physical_keys(void)
{
#if BOARD_SMK37
    uint32_t flags=irq_save();uint64_t keys=fm1_in.keys;irq_restore(flags);return keys;
#else
    return (uint64_t)plat_keys()<<5;
#endif
}
int plat_outer_keys(plat_outer_event_t *events, uint64_t *keys, int *overflow,
                    uint32_t epoch, int enabled, int octave, int publish)
{
    int n = 0;
#if BOARD_SMK37
    uint32_t flags = irq_save();
    *keys = fm1_in.keys;
    *overflow = smk_extq_overflow;
    if (publish || *overflow)
        smk_extq_r = smk_extq_w;
    else while (smk_extq_r != smk_extq_w && (unsigned)n < SMK_EXTQ - 1u) {
        events[n].generation = smk_extq[smk_extq_r][0];
        events[n++].word = smk_extq[smk_extq_r][1];
        smk_extq_r = (uint8_t)((smk_extq_r + 1u) % SMK_EXTQ);
    }
    smk_extq_overflow = 0;
    if (publish) {
        smk_ext_epoch = epoch;
        smk_ext_enabled = (uint8_t)!!enabled;
        smk_ext_oct = (int8_t)octave;
    }
    irq_restore(flags);
#else
    (void)events;
    (void)epoch;
    (void)enabled;
    (void)octave;
    (void)publish;
    *keys = plat_physical_keys();
    *overflow = 0;
#endif
    return n;
}
