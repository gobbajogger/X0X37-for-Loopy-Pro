# Credits

DnBOX is a fork of [Felucca](https://github.com/hugelton/Felucca) and keeps that project's licence. The names below are the third-party work it is built from. Full terms, file paths and notices are in [LICENSING.md](LICENSING.md) and [LICENSE](LICENSE).

| Work | Authors / origin | Licence |
| --- | --- | --- |
| [Felucca](https://github.com/hugelton/Felucca) (platform: hardware layer, USB, update loader, web installer, storage) | Leo Kuroshita ([@kurogedelic](https://github.com/kurogedelic)), [Hügelton Instruments](https://github.com/hugelton) | GPL-3.0-only |
| [9W9](https://github.com/athousanddetails/schwung-9W9) (909 voices) | athousanddetails, after [ER-99](https://github.com/matthewcieplak/er-99) | GPL-3.0 |
| [ER-99](https://github.com/matthewcieplak/er-99) (909 hi-hat, ride and crash samples in `assets/909/`) | Matthew Cieplak | GPL-3.0 |
| [8W8](https://github.com/athousanddetails/schwung-8W8) (808 voices) | athousanddetails; circuit models after the TR-808 service notes and Werner / Abel / Smith; rim shot after sc808 (Yoshinosuke Horiuchi / Sam Aaron) | GPL-3.0 |
| [Open303](https://www.rs-met.com/) (303 engine core) | Robin Schmidt. The [schwung-303 NOTICE](https://github.com/charlesvestal/schwung-303/blob/master/NOTICE) states that the Open303 engine core is separately licensed under the MIT license by Robin Schmidt. The MIT notice is in [docs/notices/open303-MIT.txt](docs/notices/open303-MIT.txt). | MIT |
| [schwung-303](https://github.com/charlesvestal/schwung-303) (303 packaging) | charlesvestal | GPL-3.0 |
| [jc303](https://github.com/midilab/jc303) / midilab (Devilfish extensions, as used via schwung-303) | midilab / jc303 contributors, named in the schwung-303 NOTICE and [docs/MANUAL.md](docs/MANUAL.md) | See the schwung-303 NOTICE |
| [dm-Rat](https://github.com/davemollen/dm-Rat) (RAT drive model, as used via schwung-303) | Dave Mollen, named in the schwung-303 NOTICE and [docs/MANUAL.md](docs/MANUAL.md) | GPL-3.0 |
| [TB-3PO](https://github.com/charlesvestal/schwung-tb3po) (acid line generator) | Ported from schwung-tb3po, itself a port of the Phazerville Hemisphere Suite `TB_3PO` applet (djphazer and contributors) | GPL-3.0 |
| [BB Gen](https://github.com/mestela/schwung-breakbeat) (break generator) | mestela. Upstream declares MIT in `src/module.json`. The inherited port is recorded as GPL-3.0-only, with the MIT notice retained in [docs/notices/schwung-breakbeat-MIT.txt](docs/notices/schwung-breakbeat-MIT.txt). | Upstream MIT; port GPL-3.0-only (see LICENSING.md) |
| [Polly](https://github.com/charlesvestal/fm1-polly/tree/62cd07ea7c95c30021c31e957cea8be448415fa7) (chord detection) | charlesvestal; adapted in `app/smk_chord_detect.h` as recorded in [LICENSING.md](LICENSING.md) | GPL-3.0 |
| Fukiai icon font (web pages only) | Hügelton Instruments; `web/fukiai.ttf` and `web/FUKIAI-LICENSE.txt` | MIT |
| JieLi AC79 SDK files (`uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin`) | [JieLi AC79 SDK](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK); read from the SDK checkout at package time and placed in each `.fwsc` package. No SDK files are in this tree. See [LICENSING.md](LICENSING.md). | Apache-2.0 |

Fonts (Barlow Semi Condensed and Terminus, SIL OFL 1.1) and the BLE-MIDI host from felucca-bsp (MPL-2.0) are also described in [LICENSING.md](LICENSING.md). Felucca's non-GPL assets (icon atlas, panel photo, Hügelton drum pack) are not part of this fork.

## Support the original creators

DnBOX does not take donations. It builds on other people's work. If you want to support it, support the original creators directly:

- Leo Kuroshita / Hügelton Instruments (Felucca): [GitHub Sponsors](https://github.com/sponsors/hugelton) or [Felucca on itch.io](https://hugelton.itch.io/felucca)
- athousanddetails (9W9, 8W8): [GitHub](https://github.com/athousanddetails)
- Matthew Cieplak (ER-99): [ER-99](https://github.com/matthewcieplak/er-99)
- Robin Schmidt (Open303): [Open303](https://github.com/RobinSchmidt/Open303)
- charlesvestal (schwung-303, TB-3PO, Polly): [GitHub](https://github.com/charlesvestal)
- mestela (BB Gen): [schwung-breakbeat](https://github.com/mestela/schwung-breakbeat)
- midilab (jc303): [Support midilab](https://midilab.co/#support)
- Dave Mollen (dm-Rat): [GitHub Sponsors](https://github.com/sponsors/davemollen)
- djphazer (Hemisphere Suite TB-3PO): [GitHub](https://github.com/djphazer)

