/* SPDX-License-Identifier: GPL-3.0-only */
#include "program_envelope.h"
#include "state_codec_private.h"
/* No program descriptor arrays: these arrays/revision are contiguous on both
 * qualified ABIs, asserted here. The wire layout is independent of C padding. */
typedef char miami_program_layout[(sizeof(original_miami_program)==43u*4u&&offsetof(original_miami_program,target)==18u*4u&&offsetof(original_miami_program,revision)==42u*4u)?1:-1];
static const original_state_schema miami_schema={NULL,43,sizeof(original_miami_program),1,{'L','X','E','P'}};
int original_miami_program_encode(const original_miami_program *p,void *bytes,size_t n){return original_state_encode(&miami_schema,p,bytes,n);}
int original_miami_program_decode(original_miami_program *p,const void *bytes,size_t n){if(!p)return 0;original_miami_program stage;if(!original_state_decode(&miami_schema,&stage,bytes,n))return 0;*p=stage;return 1;}
typedef char chicago_program_layout[(sizeof(original_chicago_program)==52u*4u&&offsetof(original_chicago_program,target)==22u*4u&&offsetof(original_chicago_program,revision)==51u*4u)?1:-1];
static const original_state_schema chicago_schema={NULL,52,sizeof(original_chicago_program),2,{'L','X','E','P'}};
int original_chicago_program_encode(const original_chicago_program *p,void *bytes,size_t n){return original_state_encode(&chicago_schema,p,bytes,n);}
int original_chicago_program_decode(original_chicago_program *p,const void *bytes,size_t n){if(!p)return 0;original_chicago_program stage;if(!original_state_decode(&chicago_schema,&stage,bytes,n))return 0;*p=stage;return 1;}
