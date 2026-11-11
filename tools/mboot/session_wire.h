/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef MBOOT_SESSION_WIRE_H
#define MBOOT_SESSION_WIRE_H
#include "capture_core.h"
#define MB_WIRE_VERSION 1u
#define MB_WIRE_REQUEST_MAX 134u
#define MB_WIRE_RESPONSE_MAX 383u
/* Local protocol status, never a flash-write result. */
enum { MB_W_CAPS, MB_W_OPEN, MB_W_READ, MB_W_FINISH, MB_W_REVALIDATE,
       MB_W_CURRENT, MB_W_CLOSE };
enum { MB_W_OK, MB_W_UNSUPPORTED, MB_W_BAD_PACKET, MB_W_BUSY,
       MB_W_FOREIGN, MB_W_STALE, MB_W_FAILED, MB_W_RELEASE_PENDING };
/* Request: version, op, sequence LE32, payload. Response: same six bytes,
 * status, payload. No native structs appear on wire. No write operation exists. */
static inline uint32_t mb_wire_u32(const uint8_t *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static inline void mb_wire_put32(uint8_t *p,uint32_t n)
{ unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i)); }
/* Exact production path: unqualified, zero capabilities, no session allocation
 * or hardware callbacks. Safe with disjoint request/output, output >= 21 bytes. */
static inline size_t mb_wire_disabled(const uint8_t *q,size_t n,uint8_t *r,size_t cap)
{
    unsigned i;
    if(!q||!r||n<6||cap<21)return 0;
    for(i=0;i<6;i++)r[i]=q[i];
    r[6]=MB_W_UNSUPPORTED;
    if(q[0]!=MB_WIRE_VERSION||q[1]>MB_W_CLOSE){r[6]=MB_W_BAD_PACKET;return 7;}
    if(q[1]!=MB_W_CAPS)return 7;
    if(n!=6){r[6]=MB_W_BAD_PACKET;return 7;}
    r[6]=MB_W_OK;mb_wire_put32(r+7,0);mb_wire_put32(r+11,MB_CAPTURE_SIZE);
    mb_wire_put32(r+15,MB_CAPTURE_HEAD);r[19]=(uint8_t)MB_CAPTURE_CHUNK;r[20]=0;
    return 21;
}
/* Providers are injected test/source interfaces, NOT qualified runtime support.
 * Serialized main-loop caller only. acquire must clean provisional resources on
 * failure; successful acquire owns a held lease until release returns zero.
 * current reports fresh binding/time; read must return exact raw bytes or fail.
 * release is idempotent and owner-bound. No callback may retain buffer pointers.
 * Identity/session generation, exclusion, timeout clock, chip reads and transport
 * authentication require separate platform qualification. NULL is production. */
struct mb_wire_provider {
    void *user;
    int (*acquire)(void *,const uint8_t[32],const uint8_t[32],struct mb_capture_binding *,uint32_t *,uint64_t *,uint64_t *);
    int (*current)(void *,struct mb_capture_binding *,uint64_t *);
    int (*read)(void *,uint32_t,uint8_t *,uint32_t);
    int (*release)(void *,const struct mb_capture_binding *);
};
struct mb_wire_session {
    struct mb_capture core;
    struct mb_capture_binding binding;
    const struct mb_wire_provider *provider;
    uint32_t sequence, next;
    uint8_t active, quarantined, finished;
};
/* Zero-initialize once. Do not reset an active context: external release first.
 * Input/output must be disjoint from each other and session; output >= MAX.
 * CAPS can be probed without a context/provider; non-NULL provider is explicit
 * injection, never a runtime capability claim. Tick is required during silence. */
size_t mb_wire_dispatch(struct mb_wire_session *,const struct mb_wire_provider *,
                        const uint8_t *,size_t,uint8_t *,size_t);
int mb_wire_tick(struct mb_wire_session *);
#endif
