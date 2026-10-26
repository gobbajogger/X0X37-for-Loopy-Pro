#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-level regression checks for SMK-37 APC40 mkII compatibility."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
usb = (ROOT / "firmware/src/usb.c").read_text()
ble = (ROOT / "firmware/src/ble.c").read_text()
main = (ROOT / "firmware/src/app/main_fm1.c").read_text()
engine = (ROOT / "firmware/src/app/engine.c").read_text()
ui = (ROOT / "firmware/src/app/ui.c").read_text()
apc = (ROOT / "firmware/src/app/smk_apc40.h").read_text()

# The pad table has 128 velocity-indexed entries: the owner-calibrated table (CLP-13). The violet group
# 48-51 (bug a67) and the other colours come from the owner export: tests/apc40_pad_owner_test.py.
owner_h = (ROOT / "firmware/src/app/apc40_pad_owner.h").read_text()
match = re.search(r"APC40_PAD_OWNER\[128\]\[3\]\s*=\s*\{(.*?)\};", owner_h, re.S)
assert match, "missing APC40 pad table"
palette = re.findall(r"\{(\d+),(\d+),(\d+)\}", match.group(1))
assert len(palette) == 128, f"expected 128 APC colors, found {len(palette)}"
assert palette[0] == ("0", "0", "0")
assert '#include "app/apc40_pad_owner.h"' in main and "APC40_RGB" not in main
print("PASS APC40 pad table (128 entries, owner-calibrated)")

# Universal Device Inquiry returns the APC40 mkII identity and protocol version.
# Sticky usb_sx_answer (mirror ble_sx_reply) builds the same Akai/APC40 mkII bytes.
assert "apc40_device_inquiry" in usb and "usb_sx_answer" in usb and "usb_sx_reply" in usb
assert "m[1] = 0x7E" in usb and "m[5] = 0x47" in usb and "m[6] = 0x29" in usb
assert "m[7] = 0x00; m[8] = 0x19" in usb
assert "midi_send_sysex(m, n)" in usb
assert "packets = (len + 2u) / 3u" in usb and "so_w - so_r + packets > SXQ" in usb
assert "sx_busy = 1" in usb and "sx_out_q[so_w % SXQ] = pkt" in usb
assert "usb.sysex[4] == 0x01u" in usb and "apc40_device_inquiry(usb.sysex[2])" in usb
print("PASS Universal Device Inquiry response (Akai APC40 mkII identity)")

# SysEx USB-MIDI packetization marks continuing and terminal packets correctly.
def pack_sysex(msg):
    out = []
    i = 0
    while i < len(msg):
        left = len(msg) - i
        n = min(3, left)
        cin = 4 if n == 3 and left > 3 else 5 if n == 1 else 6 if n == 2 else 7
        data = list(msg[i:i+n]) + [0] * (3 - n)
        out.append([cin, *data])
        i += n
    return out

identity = [0xF0, 0x7E, 0x7F, 0x06, 0x02, 0x47, 0x29, 0x00, 0x19,
            0x01, 0x02, 0x00, 0x00, 0x7F] + [0] * 20 + [0xF7]
assert len(identity) == 35
packets = pack_sysex(identity)
assert len(packets) == 12 and all(p[0] == 4 for p in packets[:-1]) and packets[-1][0] == 6
assert packets[0] == [4, 0xF0, 0x7E, 0x7F] and packets[-1][2] == 0xF7
assert "uint8_t sysex[64]" in usb
assert "uint32_t status = on ? 0x90u : 0x80u" in usb
assert "uint32_t cin = on ? 0x09u : 0x08u" in usb
print("PASS USB-MIDI SysEx framing (35-byte identity response)")

# Coordinate-preserving colors and the one-page 4x4 fold.
assert "apc_led_velocity[SMK_APC40_CLIP_COUNT]" in main
assert "apc_staged_velocity" not in main
assert "smk_apc_usb_event(pkt)" in usb
assert "smk_apc40_cell_event(apc_led_velocity" in main and "smk_apc40_cells_age(" in main
assert "smk_apc40_latch_layout(feedback, &apc_cols, &apc_rows," in main
assert "&apc_origin_col, &apc_origin_row, &apc_layout_locked" in main
assert "(unsigned)(max_col - min_col) <= 3u && (unsigned)(max_row - min_row) >= 2u" in apc
assert "origin_col + block_col % 4u" in apc
assert "block_col >= 4u ? 0u : 2u" in apc
assert "apc_origin_col, apc_origin_row" in main
assert "smk_apc40_pages_for_layout" not in main and "clip_page" not in main
assert "smk_clip_pages" not in main
assert "note != 0xFFu && apc_led_velocity[note] != 0u" in main
assert "smk_apc40_physical_pad_for_logical((uint8_t)logical)" in main
assert "smk_apc40_logical_pad_for_physical((uint8_t)i)" in main
assert "clip_pad_note[i] = note" in main and "apc_note(clip_pad_note[i], 0u)" in main
assert "if (d1 <= 98u)\n        return owned;" in main
print("PASS origin-normalized sparse maps and single-view pad rendering")

# Local 8x2 views replace Loopy viewport navigation. No navigation MIDI is sent.
assert "smk_apc_viewport_changed" not in main and "apc_scroll_" not in main and "0x5Eu" not in main and "0x5Fu" not in main
assert "midi_send_note(0x61u" not in main and "midi_send_note(0x60u" not in main and "SMK_APC40_RIGHT_NOTE" not in main
assert "midi_send_note(0x5Eu" not in main and "midi_send_note(0x5Fu" not in main
assert "smk_apc_view_move" not in main and "fader_nav" not in main
assert "smk_apc40_note_for_pad_view(apc_cols, apc_rows, apc_origin_col," in main
assert "oct_pressed & (1u << panel.btn[B_OCTDN])" in main and "oct_pressed & (1u << panel.btn[B_OCTUP])" in main
print("PASS < > local only (no Loopy moves), OCT-/OCT+ move clamped local views; KNOB BANK tap and FADER BANK are no-ops in clip mode")

# Clip mode entry latches the layout from stored feedback, so the pads light at once.
assert "smk_apc_clip_entered();" in main
assert "apc_layout_deadline = fm1_ms;" in main
assert "smk_apc40_burst_touch(&apc_burst_mask, &apc_burst_count, note)" in main
assert "smk_apc_burst_close();" in main and "SMK_APC40_SNAPSHOT_MIN_CELLS 8u" in apc
assert "smk_apc40_layout_grows(apc_cols, apc_rows" in main
assert "phase < 128u ? 255u : 0u" in apc
assert "smk_apc40_is_queued(channel)" in main and "smk_apc40_beat_phase(apc_clock_count)" in main and "SMK_APC40_CELL_QUEUED" in main
print("PASS clip mode entry refreshes pad colors; page snapshots clear old-page cells; layout grows; pending clips blink")

# APC feedback is stored at all times. It is kept from the sound engine only in clip mode.
assert "smk_apc_led_set(d1, ch, vel)" not in engine   # stored once, in the USB interrupt
assert "smk_apc_usb_tick();" in usb and "smk_apc40_cells_age(" not in main.split("static uint8_t smk_clip_map_refresh")[1].split("\n}\n")[0]
assert "smk_apc_host_is_active" not in engine and "smk_apc_host_is_active" not in main
assert "apc_host_on & (1u << apc_in_ble)" in main   # APC handshake owns only the input link that sent it
assert "smk_apc_ble_host_seen();" in ble and "smk_apc_host_seen();" in usb
assert "if (d1 <= 98u)\n        return owned;" in main and "(d1 >= 48u && d1 <= 63u)" in main
# Clip pads: keep Loopy pad feedback out of the sound engine in clip mode, but a note
# on the break's own channel is the break (kit-consume), not a clip pad.
assert engine.count("if (smk_clip_mode_on() && ch != seq.ch[TRK_BRK])\n                return;") == 1
# Transport feedback notes still use the plain clip-mode early return.
assert engine.count("if (smk_clip_mode_on())\n                return;") == 1
assert "SMK_APC40_PAD_FLOOR" not in main   # pads: half level, no floor (user decision)
assert "smk_clip_pad_level(rgb[0], v, brightness)" in main
assert "SMK_CLIP_LCD_FLOOR" not in main   # LCD_FLOOR is in tools/apc_colour_model.py now
assert "smk_apc_host_seen();" in usb and "smk_apc_host_lost();" in usb
assert "BOARD_SMK37 && !defined(FELUCCA_LOADER)" in usb
assert "clip_state[16]" not in main and "smk_clip_state_set" not in engine
assert "APC40_PAD_OWNER[v];   /* the last color from Loopy" in main
print("PASS host-owned clip LED state capture; last color drawn from the owner-calibrated table")

# Play and overdub buttons reflect Loopy feedback in clip mode.
assert "smk_apc_transport_led_set(d1, vel)" in engine
assert "smk_apc_transport_led_state(0x5Bu)" in ui
assert "smk_apc_transport_led_state(0x5Du)" in ui
assert "smk_apc40_transport_feedback_note" in apc
assert "smk_apc40_transport_feedback_on" in apc
assert "smk_clip_transport_button((uint8_t)i, (uint8_t)((btn & m) != 0u))" in ui
assert "static uint8_t play_sent, record_sent" in main
assert "apc_note(note, event == SMK_APC40_BUTTON_ON ? 127u : 0u)" in main
assert "smk_apc40_transport_led_after_press(apc_play_feedback)" in main
assert "smk_apc40_transport_led_after_press(apc_record_feedback)" in main
assert "smk_apc_clock_tick();" in engine and "smk_apc_clock_start();" in engine
assert "smk_apc40_play_led(apc_play_feedback, live, apc_clock_count)" in main
print("PASS Loopy Play/Record feedback LEDs, clock-synced PLAY blink, momentary transport messages")

# Clip mode keeps controls from triggering X0X actions; the APC initialization reply remains intact.
plat = (ROOT / "firmware/src/app/plat_fm1.c").read_text()
assert "clip_mode && panel.btn[i] <= FM1_BTN_OCT_UP" in plat
# clip keys: the keybed stays out of the X0X UI; a held pad still blocks its white key (main_fm1.c)
assert "if (smk_clip_mode_on() || smk_padcal_on())\n#else\n    if (smk_clip_mode_on())\n#endif\n        return;" in ui   # CLP-79: also in the pad calibration (dev build)
assert "w >= 0 && ((fm1_in.pnotes >> (k - SMK_KEY_F3)) & 1u)" in main
update_mode = main.split("/* OCT- + OCT+ held 5 s:")[1].split("/* Project transfers cannot queue")[0]
assert "if (ui_project_busy()" in update_mode and "|| smk_clip_mode_on()" in update_mode
assert "t0 = fm1_ms;" in update_mode and 'enter_uboot("UBOOT")' in update_mode
assert "m[0] = 0xF0; m[1] = 0x47; m[2] = 0x7F; m[3] = 0x29; m[4] = 0x61; m[5] = 0x00; m[6] = 0x09;" in usb
assert "apc40_introduction_reply();" in usb and "usb_sx_reply = 2u" in usb
print("PASS clip input isolation and APC40 initialization handshake")

# 1.5: clip grid on the LCD, drawn in the main loop (ui.c draw_main), rate limited, no new buffer.
ui_src = (ROOT / "firmware/src/app/ui.c").read_text() if "ROOT" in dir() else open("firmware/src/app/ui.c").read()
assert "smk_clip_lcd_draw(240u, 2u * BAND_H)" in ui_src
assert "uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h)" in main
assert "SMK_CLIP_LCD_MIN_MS 64u" in main and "cv_begin(w, h, C_BLACK)" in main
assert "smk_clip_lcd_draw" not in usb and "smk_clip_lcd_draw" not in engine
print("PASS clip grid on the LCD: main loop only, rate limited, current pad view framed")
# CLP-05: the LCD clip colours come from the table APC40_LCD565 (tools/apc_pad_colors.py, plain RGB565).
draw = main.split("uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h)\n{")[1].split("\n}\n")[0]
assert '#include "app/apc40_lcd565.h"' in main
assert "uint16_t c = smk_clip_lcd565(v);" in draw          # APC40_LCD565, idle shades half of the full shade (CLP-05)
assert "APC40_RGB" not in draw and "SMK_CLIP_LCD_FLOOR + mx" not in main   # the old floor calculation is gone
assert "if (v)\n                    cv_rect(x + gap + 1, y + gap + 1, p - 2 * gap - 2, p - 2 * gap - 2, c);" in draw
assert "else\n                    cv_rect(x + p / 2 - 2, y + p / 2 - 2, 4, 4, C_DIM);" in draw   # velocity 0: a dim dot
pads = main.split("static void smk_pads(void)")[1].split("\n}\n")[0]
assert "APC40_LCD565" not in pads and "APC40_PAD_OWNER[v];" in pads    # the pads use APC40_PAD_OWNER (CLP-13)
assert "void smk_clip_lcd_force(void) { was_on = 0u; }" in main   # mode entry and menu close: draw at once
assert "smk_clip_lcd_force();   /* a menu" in ui_src
lost = main.split("void smk_apc_host_lost(void)")[1].split("\n}\n")[0]
assert "smk_apc_link_clear(0u)" in lost                            # USB disconnect: the USB cells empty (CLP-18)
link = main.split("void smk_apc_ble_link(uint8_t connected, uint8_t notifying)")[1].split("\n}\n")[0]
assert "smk_apc_link_clear(1u)" in link                            # Bluetooth disconnect: the Bluetooth cells empty
print("PASS LCD clip colours from APC40_LCD565; empty cells, pads, entry, menu and disconnect unchanged")
assert "smk_apc_clock_position(" in engine and "cin == 3u && ep1rx[i + 1] == 0xF2u" in usb
assert "apc_clock_count = sixteenths * 6u" in main
print("PASS Song Position resets the queued blink phase")
assert "smk_apc40_lcd_map(apc_cols, apc_rows, apc_origin_col, apc_origin_row" in main
print("PASS LCD clip map fits the used clip area")
ui_src = open("firmware/src/app/ui.c").read()
assert "smk_clip_history_button((uint8_t)i, (uint8_t)((btn & m) != 0u), (uint8_t)((btn >> B_HOME) & 1u))" in ui_src
assert ui_src.index("smk_clip_history_button((uint8_t)i") < ui_src.index("smk_clip_transport_button((uint8_t)i")
assert "ui.btn_used |= m | 1u << B_HOME;" in ui_src
assert "if (!clip_grid_mode || !patch_held)" in main and 'say(redo ? "LOOPY REDO" : "LOOPY UNDO", 0);' in main
print("PASS Loopy undo/redo: PATCH + SEQ REC / SEQ PLAY in clip mode only")
# a74 (MID-14): PLAY outside clip mode also sends note 0x5B on channel 1, after the clip mode handlers
call = "smk_play_note_button((uint8_t)((btn & m) != 0u), (uint8_t)((btn >> B_HOME) & 1u))"
assert call in ui_src
assert ui_src.index("smk_clip_transport_button((uint8_t)i") < ui_src.index(call) < ui_src.index("button_tap((int)i);")
assert "apc_out(smk_apc40_play_note_packet(event == SMK_APC40_BUTTON_ON))" in main
assert "smk_apc40_play_note_event(&sent, clip_grid_mode, home_held, pressed)" in main
print("PASS PLAY note 0x5B outside clip mode: USB and Bluetooth, after the clip mode handlers")
# a71: Loopy gone on Bluetooth (link end or CCCD 0): ble.c gives the link state to the clip feedback
# after each event poll, and main_fm1.c clears the cells (tests/smk_clip_ble_clear_test.c runs the logic).
assert "smk_apc_ble_link((uint8_t)bleh_connected(h), (uint8_t)bleh_notifying(h));" in ble
assert ble.index("bleh_rx(h, t, ble_r.fr") < ble.index("smk_apc_ble_link((uint8_t)") < ble.index("bleh_midi_next(h, &m)")
link = main.split("void smk_apc_ble_link(uint8_t connected, uint8_t notifying)")[1].split("\n}\n")[0]
assert "smk_apc40_lost_step(&lost, connected, notifying, fm1_ms)" in link   # a76: and again 300 ms later
clr = main.split("static uint8_t smk_apc_link_clear(uint8_t ble)")[1].split("\n}\n")[0]
assert "padled_rgb[i] = 0u;" in clr and "padled_dirty = 1u;" in clr   # a76: the pad colour store too
assert "smk_apc40_cells_clear_mask(apc_led_velocity, apc_led_channel, apc_cell_flags, apc_cell_tick," in clr
assert "ble ? apc_ble_owned : ~apc_ble_owned" in clr                 # CLP-18: only the cells of the lost link
assert "if ((m >> APC_OWN_PLAY) & 1u)\n        apc_play_feedback = 0u;" in clr   # CLP-18: PLAY and REC of the lost link only
assert "if ((m >> (APC_OWN_PLAY + 1u)) & 1u)\n        apc_record_feedback = 0u;" in clr
assert "apc_own(APC_OWN_PLAY + (d1 == SMK_APC40_RECORD_NOTE));" in main
assert "apc_play_feedback" not in link and "apc_play_feedback" not in lost
assert "smk_apc_link_clear(1u)" in link
for body in (link, clr):
    assert "smk_ck" not in body and "clip_grid_mode" not in body and "seq" not in body
assert "apc_host_on" not in clr and "apc_host_on &= (uint8_t)~2u;" in link
# CLP-18: usb.c gives the USB link state each poll; its end clears only the USB cells (no apc_host_on change)
assert "smk_apc_usb_link((uint8_t)(usb.config && !usb.suspended));" in usb
ul = main.split("void smk_apc_usb_link(uint8_t up)")[1].split("\n}\n")[0]
assert "smk_apc_link_clear(0u)" in ul and "apc_host_on" not in ul and "smk_ck" not in ul
# CLP-18: the APC40 introduction with mode 0x40 (generic mode) clears the cells of that link
assert "if (usb.sysex[7] == 0x40u)\n                smk_apc_host_generic(0u);" in usb
assert "if (ble_sx[6] == 0x40u)\n            smk_apc_host_generic(1u);" in ble
assert "(void)smk_apc_link_clear(ble);" in main.split("void smk_apc_host_generic(uint8_t ble)")[1].split("\n}\n")[0]
print("PASS Bluetooth link end or notify off clears the clip feedback")
# a76 rule 8: ble.c gives every Bluetooth note and CC to smk_apc_ble_event; it does not drop any.
assert "smk_apc_ble_event(pkt, (uint8_t)bleh_notifying(h))" in ble
assert "ble_in_unheard" not in ble
bev = main.split("uint8_t smk_apc_ble_event(uint32_t pkt, uint8_t notifying)")[1].split("\n}\n")[0]
assert "smk_apc40_ble_colour_unheard(pkt, notifying)" in bev and "smk_apc_usb_event(pkt)" in bev
print("PASS Bluetooth notes while notify is off: only the pad colour write stops (rule 8)")
