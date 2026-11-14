# Source reuse and distribution scope

The design renderer uses public DnBOX UI families from `firmware/src/app/ui.c`, `ui_original_controls.h`, `ui_original_wave.h` and `ui_pictograms.h` at exact3c41/d158 pins, with explicit proposal adaptations. Those sources carry SPDX GPL-3.0-only. Original source attributions remain intact; this export does not rewrite Git identity or historical credits. The firmware UI and panel draw through Felucca's canvas; panel source credits Leo Kuroshita (@kurogedelic), Hugelton Instruments.

Barlow Semi Condensed font files retain the supplied SIL Open Font License in `Barlow-OFL.txt`.

Only final graphical WAVE drawing coordinates and metadata hashes leave the private analytical intake. No restricted numerical header, factory cell/native snapshot, proprietary SDK/toolchain, host executable, FWSC/WASM/controller payload, user secret or raw private Settings archive is included. Graphical output is not a distribution grant for those inputs. The old Library HTTP401 and file-URI denial are preserved and not retried.
