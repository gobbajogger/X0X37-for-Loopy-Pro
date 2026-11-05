#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Keep the original entrypoint, using the recut base's own DSP headers too.
set -eu
exec sh "$(dirname "$0")/run_drum909_struct_equivalence.sh"
