/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef ORIGINAL_CHICAGO_DRIVE_H
#define ORIGINAL_CHICAGO_DRIVE_H
#include <stdint.h>
#include <stddef.h>
/* Explicit original X11800Drive state. No wrapper event queue or lifecycle is
 * inferred. Caller owns activity changes and reset timing; no active backend. */
typedef struct {
 int32_t target_drive,target_gain,drive,gain,threshold;
 int32_t previous_driven[2],previous_nonlinear[2];
 uint8_t activity,edit1,edit2;
} original_chicago_drive;
void original_chicago_drive_init(original_chicago_drive *s);
/* Reset preserves targets, cached parameters and activity. */
void original_chicago_drive_reset(original_chicago_drive *s);
int original_chicago_drive_set(original_chicago_drive *s,unsigned parameter,unsigned value);
int original_chicago_drive_activity(original_chicago_drive *s,unsigned activity);
/* Separate stereo histories and once-per-frame shared smoothing. N=0 is a
 * no-op. In-place L->L/R->R is allowed; all other cross-array overlap forbidden.
 * This does not drain/synthesize original queued type4 reset events. */
int original_chicago_drive_process(original_chicago_drive *s,const int16_t *left,const int16_t *right,int16_t *out_left,int16_t *out_right,size_t count);
#endif
