#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Source-level regression checks for SMK-37 APC40 mkII compatibility."""
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
usb = (ROOT / "firmware/src/usb.c").read_text()
main = (ROOT / "firmware/src/app/main_fm1.c").read_text()
engine = (ROOT / "firmware/src/app/engine.c").read_text()
ui = (ROOT / "firmware/src/app/ui.c").read_text()
apc = (ROOT / "firmware/src/app/smk_apc40.h").read_text()

# The complete APC40 mkII palette has 128 velocity-indexed entries.
match = re.search(r"APC40_RGB\[128\]\s*=\s*\{(.*?)\};", main, re.S)
assert match, "missing APC40 palette"
palette = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{6})", match.group(1))]
assert len(palette) == 128, f"expected 128 APC colors, found {len(palette)}"
assert palette[0] == 0x000000 and palette[3] == 0xFFFFFF
assert palette[5] == 0xFF0000 and palette[21] == 0x00FF00 and palette[127] == 0x4B1502
print("PASS APC40 RGB palette (128 entries)")

# Universal Device Inquiry returns the APC40 mkII identity and protocol version.
assert "apc40_device_inquiry" in usb
assert "msg[1] = 0x7E" in usb and "msg[5] = 0x47" in usb and "msg[6] = 0x29" in usb
assert "msg[7] = 0x00; msg[8] = 0x19" in usb
assert "midi_send_sysex(msg, sizeof msg)" in usb
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
assert "max_col - min_col <= 3u && max_row - min_row >= 2u" in apc
assert "origin_col + block_col % 4u" in apc
assert "block_col >= 4u ? 0u : 2u" in apc
assert "apc_origin_col, apc_origin_row" in main
assert "smk_apc40_pages_for_layout" not in main and "clip_page" not in main
assert "smk_clip_pages" not in main
assert "note != 0xFFu && apc_led_velocity[note] != 0u" in main
assert "smk_apc40_physical_pad_for_logical((uint8_t)logical)" in main
assert "smk_apc40_logical_pad_for_physical((uint8_t)i)" in main
assert "sent_note[i] = note" in main and "midi_send_note(sent_note[i], 0u)" in main
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
assert "uint8_t owned = clip_grid_mode;" in main   # APC notes drop only in clip mode
assert engine.count("if (smk_clip_mode_on())\n                return;") == 2
assert "SMK_APC40_PAD_FLOOR" not in main   # pads: half level, no floor (user decision)
assert "((rgb >> 16) * brightness) / 510u" in main
assert "SMK_CLIP_LCD_FLOOR 128u" in main and "SMK_CLIP_LCD_FLOOR + mx * (255u - SMK_CLIP_LCD_FLOOR) / 255u" in main
assert "smk_apc_host_seen();" in usb and "smk_apc_host_lost();" in usb
assert "BOARD_SMK37 && !defined(FELUCCA_LOADER)" in usb
assert "clip_state[16]" not in main and "smk_clip_state_set" not in engine
assert "APC40_RGB[v];   /* the last color from Loopy" in main
print("PASS host-owned clip LED state capture; last color is drawn as sent")

# Play and overdub buttons reflect Loopy feedback in clip mode.
assert "smk_apc_transport_led_set(d1, vel)" in engine
assert "smk_apc_transport_led_state(0x5Bu)" in ui
assert "smk_apc_transport_led_state(0x5Du)" in ui
assert "smk_apc40_transport_feedback_note" in apc
assert "smk_apc40_transport_feedback_on" in apc
assert "smk_clip_transport_button((uint8_t)i, (uint8_t)((btn & m) != 0u))" in ui
assert "static uint8_t play_sent, record_sent" in main
assert "midi_send_note(note, event == SMK_APC40_BUTTON_ON ? 127u : 0u)" in main
assert "smk_apc40_transport_led_after_press(apc_play_feedback)" in main
assert "smk_apc40_transport_led_after_press(apc_record_feedback)" in main
assert "smk_apc_clock_tick();" in engine and "smk_apc_clock_start();" in engine
assert "smk_apc40_play_led(apc_play_feedback, live, apc_clock_count)" in main
print("PASS Loopy Play/Record feedback LEDs, clock-synced PLAY blink, momentary transport messages")

# Clip mode keeps controls from triggering X0X actions; the APC initialization reply remains intact.
plat = (ROOT / "firmware/src/app/plat_fm1.c").read_text()
assert "clip_mode && panel.btn[i] <= FM1_BTN_OCT_UP" in plat
assert "KEY_WHITE[k] >= 0 && ((fm1_in.pnotes >> k) & 1u)" in ui
assert "if (smk_clip_mode_on())" in main and "enter_uboot(\"UBOOT\")" in main
assert "0xF0, 0x47, 0x7F, 0x29, 0x61, 0x00, 0x09" in usb
assert "apc40_introduction_reply();" in usb
print("PASS clip input isolation and APC40 initialization handshake")

# 1.5: clip grid on the LCD, drawn in the main loop (ui.c draw_main), rate limited, no new buffer.
ui_src = (ROOT / "firmware/src/app/ui.c").read_text() if "ROOT" in dir() else open("firmware/src/app/ui.c").read()
assert "smk_clip_lcd_draw(240u, 2u * BAND_H)" in ui_src
assert "uint8_t smk_clip_lcd_draw(uint32_t w, uint32_t h)" in main
assert "SMK_CLIP_LCD_MIN_MS 64u" in main and "cv_begin(w, h, C_BLACK)" in main
assert "smk_clip_lcd_draw" not in usb and "smk_clip_lcd_draw" not in engine
print("PASS clip grid on the LCD: main loop only, rate limited, current pad view framed")
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
