static volatile uint8_t clip_grid_mode;   /* APC40 mkII compatibility; KNOB BANK press toggles */
static volatile uint8_t apc_host_on;      /* bit 0 USB, bit 1 Bluetooth: APC handshake on that input */
/* attempt 30: what clip mode sends to the APC40 host goes to USB and to a Bluetooth MIDI connection
 * (Loopy Pro on the BLE port). ble_out has one producer (the audio ISR): IRQs off around it here. */
static void apc_out(uint32_t pkt)
{
    uint32_t f = irq_save();
    midi_out_event(pkt);
#if X0X_BLE
    ble_out(pkt);
#endif
    irq_restore(f);
}
static void apc_note(uint32_t note, uint32_t vel)
{
    uint32_t on = vel != 0u;
    apc_out((on ? 0x09u : 0x08u) | (on ? 0x90u : 0x80u) << 8 | (note & 0x7Fu) << 16 | (on ? vel : 0x7Fu) << 24);
}
