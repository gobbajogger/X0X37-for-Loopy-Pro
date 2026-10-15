// SPDX-License-Identifier: MPL-2.0
/* Copyright (C) 2026 M. Geisler (genkigenki/makkuro)
 *
 * What btstack.a needs from the SDK's system and app layers, for a BLE-only, one-link host.
 * Our own code, from the public header declarations (Apache-2.0). */
#include "ctl_port.h"

/* ---- the SDK's config knobs (apps/.../btstack_config, consts the library reads) ---- */
const int config_le_hci_connection_num = 1;
const int config_le_gatt_server_num = 1;
const int config_le_gatt_client_num = 0;
const int config_le_sm_support_enable = 1;
const int config_le_sm_sub_sc_enable = 0;         /* legacy Just Works: iOS accepts it */
const int config_delete_link_key = 0;
const int btstack_emitter_support = 0;
const int config_stack_modules = 1 << 2;          /* BT_BTSTACK_LE */

/* ---- cbuf (include_lib/btctrler/adapter/include/common/cbuf.h layout) ---- */
typedef struct {
    u8 *begin, *end, *read_ptr, *write_ptr, *tmp_ptr;
    u32 tmp_len, data_len, total_len;
} cbuffer_t;

void cbuf_init(cbuffer_t *c, void *buf, u32 size)
{
    c->begin = c->read_ptr = c->write_ptr = c->tmp_ptr = buf;
    c->end = (u8 *)buf + size;
    c->tmp_len = c->data_len = 0;
    c->total_len = size;
}
void cbuf_clear(cbuffer_t *c)
{
    u32 f = ctl_port_irq_save();
    c->read_ptr = c->write_ptr = c->tmp_ptr = c->begin;
    c->tmp_len = c->data_len = 0;
    ctl_port_irq_restore(f);
}
u32 cbuf_is_write_able(cbuffer_t *c, u32 len)
{
    u32 free_n = c->total_len - c->data_len;
    return len <= free_n ? free_n : 0;
}
u32 cbuf_write(cbuffer_t *c, void *buf, u32 len)
{
    u32 f, k;
    const u8 *s = buf;
    f = ctl_port_irq_save();
    if (len > c->total_len - c->data_len) { ctl_port_irq_restore(f); return 0; }
    for (k = 0; k < len; k++) {
        *c->write_ptr++ = s[k];
        if (c->write_ptr == c->end) c->write_ptr = c->begin;
    }
    c->data_len += len;
    ctl_port_irq_restore(f);
    return len;
}
u32 cbuf_read(cbuffer_t *c, void *buf, u32 len)
{
    u32 f, k;
    u8 *d = buf;
    f = ctl_port_irq_save();
    if (len > c->data_len) { ctl_port_irq_restore(f); return 0; }
    for (k = 0; k < len; k++) {
        d[k] = *c->read_ptr++;
        if (c->read_ptr == c->end) c->read_ptr = c->begin;
    }
    c->data_len -= len;
    ctl_port_irq_restore(f);
    return len;
}
/* contiguous alloc / update: the part up to the buffer end */
void *cbuf_write_alloc(cbuffer_t *c, u32 *len)
{
    u32 free_n = c->total_len - c->data_len, run = (u32)(c->end - c->write_ptr);
    *len = free_n < run ? free_n : run;
    return c->write_ptr;
}
void cbuf_write_updata(cbuffer_t *c, u32 len)
{
    u32 f = ctl_port_irq_save();
    c->write_ptr += len;
    if (c->write_ptr >= c->end) c->write_ptr -= c->total_len;
    c->data_len += len;
    ctl_port_irq_restore(f);
}
void *cbuf_read_alloc(cbuffer_t *c, u32 *len)
{
    u32 run = (u32)(c->end - c->read_ptr);
    *len = c->data_len < run ? c->data_len : run;
    return c->read_ptr;
}
void cbuf_read_updata(cbuffer_t *c, u32 len)
{
    u32 f = ctl_port_irq_save();
    c->read_ptr += len;
    if (c->read_ptr >= c->end) c->read_ptr -= c->total_len;
    c->data_len -= len;
    ctl_port_irq_restore(f);
}

/* ---- lbuf_realloc: keep the block when it fits (ctl_heap.c stores the size before it) ---- */
int lbuf_real_size(void *lbuf);
void *lbuf_realloc(void *lbuf, int size)
{
    return (lbuf && size <= lbuf_real_size(lbuf)) ? lbuf : (void *)0;
}

/* ---- CRC-16 (CCITT, poly 0x1021, init 0) as the SDK's chip_crc16 ---- */
u16 chip_crc16(const void *ptr, u32 len)
{
    const u8 *p = ptr;
    u16 crc = 0;
    while (len--) {
        crc ^= (u16)(*p++) << 8;
        for (int i = 0; i < 8; i++) crc = (crc & 0x8000) ? (u16)((crc << 1) ^ 0x1021) : (u16)(crc << 1);
    }
    return crc;
}

/* ---- the app's event bus: not used (host_app.c takes the events from the callbacks) ---- */
int bt_event_notify(int from, void *event) { (void)from; (void)event; return 0; }

/* the library built cbuf_get_data_size as a function */
u32 cbuf_get_data_size(cbuffer_t *c) { return c->data_len; }

/* classic profile knobs and hooks the shared stack code references; no classic here */
const u8 a2dp_mutual_support = 0;
const u8 more_avctp_cmd_support = 0;
const int sniff_support_reset_anchor_point = 0;
int a2dp_sink_media_codec_begin(void) { return 0; }
int a2dp_sink_media_codec_end(void) { return 0; }
