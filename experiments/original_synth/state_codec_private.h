/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_STATE_CODEC_PRIVATE_H
#define ORIGINAL_STATE_CODEC_PRIVATE_H
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#if CHAR_BIT != 8 || INT32_MIN != (-2147483647-1)
#error State envelope requires qualified byte and signed32 representations
#endif
/* Immutable compiler-owned schema, never an untrusted-file descriptor.
 * offset/kind packs into2 readonly bytes; no per-instance table or heap.
 * U/I retain exact32 bits, F finite binary32, B byte, N normalized binary32.
 * One shared cold encoder/decoder serves both programs and both voice types.
 * NULL fields is a compiler-proved contiguous normalized-word program with
 * one trailing uint32 revision. No descriptor data is needed for that case. */
enum {ORIGINAL_STATE_U,ORIGINAL_STATE_F,ORIGINAL_STATE_B,ORIGINAL_STATE_I,ORIGINAL_STATE_N};
typedef struct {const uint16_t *fields;uint32_t count,size;uint8_t backend;char magic[4];} original_state_schema;
#define ORIGINAL_STATE_FIELD(T,K,P) ((uint16_t)(((offsetof(T,P)<<3)|(K))+0u*sizeof(char[(offsetof(T,P)<8192u&&sizeof(((T *)0)->P)==((K)==ORIGINAL_STATE_B?1u:4u))?1:-1])))
/* Caller data, byte buffer and staging do not overlap. Typed public wrappers
 * use a small local staging object to support overlapping decode input/output.
 * Validate the complete header/checksum/type domain before mutating staging. */
int original_state_encode(const original_state_schema *,const void *object,void *bytes,size_t capacity);
int original_state_decode(const original_state_schema *,void *staging,const void *bytes,size_t length);
#endif
