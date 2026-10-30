# Tape reel activity

Replaces26ee decorative visible-TAPE rotation per owner refinement. Existing delay input and computed wet values contribute one boolean in the existing sample loop, held for less than80ms by the UI telemetry. No new scan/RMS/output maths. All render_sub segments contribute; silent later segments cannot clear a recent signal. Delay mute/zero return, DIGI, init and project hold stop motion. Live input and actual wet tails work with transport stopped; silent running transport does not drive reels. Idle freezes the current phase; resume does not catch up idle time. Final master/device gain is separate.

Rejected ring-occupancy proxy produced about1.72s false post-output motion; it is not used. R4 design approved exact revised mock; local timestamp is read before clock to avoid IRQ read-order underflow. Previous DRIVE/LOOP/FILTER/Settings approvals remain unchanged.

Affected actual host gates/render and native BUILD_ONLY must be pinned to final successor; no hardware or public delivery claimed. Public Git operation denial remains binding.
