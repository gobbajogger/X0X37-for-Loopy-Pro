# DnBOX unit readouts (before = raw pot shown at pot 0/1/63/71/127)

| control | before | after | source of scaling |
|---|---|---|---|
| 909 BD Tune | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 BD Attack | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 BD Decay | 0 / 1 / 63 / 71 / 127 | 100ms / 103ms / 623ms / 786ms / 4.0s | EXP/LIN time table in drum909_param_si |
| 909 BD Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 BD P.Dpth | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 BD Pitch | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 BD Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 BD Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 BD Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 BD Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 SD Tune | 0 / 1 / 63 / 71 / 127 | 130Hz / 131Hz / 203Hz / 215Hz / 320Hz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| 909 SD Tone | 0 / 1 / 63 / 71 / 127 | 300Hz / 306Hz / 1.1kHz / 1.3kHz / 4.0kHz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| 909 SD Snappy | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 SD Decay | 0 / 1 / 63 / 71 / 127 | 120ms / 125ms / 457ms / 500ms / 800ms | EXP/LIN time table in drum909_param_si |
| 909 SD Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 SD Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 SD Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 SD Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 SD Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 LT Tune | 0 / 1 / 63 / 71 / 127 | 50Hz / 50Hz / 67Hz / 69Hz / 90Hz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| 909 LT Decay | 0 / 1 / 63 / 71 / 127 | 80ms / 82ms / 450ms / 560ms / 2.6s | EXP/LIN time table in drum909_param_si |
| 909 LT Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 LT Attack | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 LT Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 LT Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 LT Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 LT Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 MT Tune | 0 / 1 / 63 / 71 / 127 | 80Hz / 80Hz / 100Hz / 103Hz / 125Hz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| 909 MT Decay | 0 / 1 / 63 / 71 / 127 | 70ms / 72ms / 387ms / 481ms / 2.2s | EXP/LIN time table in drum909_param_si |
| 909 MT Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 MT Attack | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 MT Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 MT Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 MT Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 MT Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 HT Tune | 0 / 1 / 63 / 71 / 127 | 110Hz / 110Hz / 137Hz / 140Hz / 170Hz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| 909 HT Decay | 0 / 1 / 63 / 71 / 127 | 60ms / 62ms / 342ms / 426ms / 2.0s | EXP/LIN time table in drum909_param_si |
| 909 HT Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 HT Attack | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 HT Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 HT Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 HT Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 HT Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 RS Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 RS Tune | 0 / 1 / 63 / 71 / 127 | 150Hz / 151Hz / 212Hz / 221Hz / 300Hz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| 909 RS Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 RS Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 RS Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 RS Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 CP Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 CP Tune | 0 / 1 / 63 / 71 / 127 | 650Hz / 654Hz / 951Hz / 998Hz / 1.4kHz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| 909 CP Tail | 0 / 1 / 63 / 71 / 127 | 120ms / 122ms / 344ms / 393ms / 1000ms | EXP/LIN time table in drum909_param_si |
| 909 CP Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 CP Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 CP Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 CP Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 CH Decay | 0 / 1 / 63 / 71 / 127 | 15ms / 15ms / 66ms / 80ms / 300ms | EXP/LIN time table in drum909_param_si |
| 909 CH Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 CH Tune | 0 / 1 / 63 / 71 / 127 | -24st / -24st / -0.2st / +2.8st / +24st | 12·log2(ratio) / TU12 semitone law in drum909_param_si |
| 909 CH Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 CH Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 CH Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 CH Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 OH Decay | 0 / 1 / 63 / 71 / 127 | 20ms / 21ms / 152ms / 197ms / 1.2s | EXP/LIN time table in drum909_param_si |
| 909 OH Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 OH Tune | 0 / 1 / 63 / 71 / 127 | -24st / -24st / -0.2st / +2.8st / +24st | 12·log2(ratio) / TU12 semitone law in drum909_param_si |
| 909 OH Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 OH Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 OH Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 OH Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 CR Tune | 0 / 1 / 63 / 71 / 127 | -24st / -24st / -0.2st / +2.8st / +24st | 12·log2(ratio) / TU12 semitone law in drum909_param_si |
| 909 CR Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 CR Decay | 0 / 1 / 63 / 71 / 127 | 100ms / 103ms / 540ms / 670ms / 3.0s | EXP/LIN time table in drum909_param_si |
| 909 CR Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 CR Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 CR Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 CR Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 RD Tune | 0 / 1 / 63 / 71 / 127 | -24st / -24st / -0.2st / +2.8st / +24st | 12·log2(ratio) / TU12 semitone law in drum909_param_si |
| 909 RD Level | 0 / 1 / 63 / 71 / 127 | OFF / -40dB / -3.5dB / -2.4dB / +2.6dB | 20·log10(gain), gain from drum909_param_si (LIN/EXP pot table) |
| 909 RD Decay | 0 / 1 / 63 / 71 / 127 | 100ms / 103ms / 540ms / 670ms / 3.0s | EXP/LIN time table in drum909_param_si |
| 909 RD Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 RD Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 RD Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum909_param_si |
| 909 RD Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 909 KIT Accent | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 909 KIT Veloc | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 BD Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 BD Tone | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 BD Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 BD Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 BD Attack | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 BD Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 BD Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 BD Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 BD Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 SD Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 SD Tone | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 49% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 SD Snappy | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 SD Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 SD Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 SD Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 SD Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 SD Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 SD Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 LT Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 LT Tune | 0 / 1 / 63 / 71 / 127 | -2.0st / -2.0st / 0.0st / +0.2st / +2.0st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 LT Decay | 0 / 1 / 63 / 71 / 127 | 60ms / 62ms / 342ms / 426ms / 2.0s | EXP/LIN time table in drum808_param_si |
| 808 LT Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 LT Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 LT Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 LT Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 MT Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 MT Tune | 0 / 1 / 63 / 71 / 127 | -2.0st / -2.0st / 0.0st / +0.2st / +2.0st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 MT Decay | 0 / 1 / 63 / 71 / 127 | 60ms / 62ms / 342ms / 426ms / 2.0s | EXP/LIN time table in drum808_param_si |
| 808 MT Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 MT Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 MT Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 MT Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 HT Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 HT Tune | 0 / 1 / 63 / 71 / 127 | -2.0st / -2.0st / 0.0st / +0.2st / +2.0st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 HT Decay | 0 / 1 / 63 / 71 / 127 | 60ms / 62ms / 342ms / 426ms / 2.0s | EXP/LIN time table in drum808_param_si |
| 808 HT Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 HT Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 HT Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 HT Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 RS Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 RS Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 RS Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 RS Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 RS Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 RS Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 RS Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 CP Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 CP Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 CP Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CP Attack | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CP Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CP Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CP Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CP Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 CB Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 CB Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 CB Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CB Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CB Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CB Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CB Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 CY Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 CY Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CY Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 CY Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CY Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CY Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CY Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 OH Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 OH Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 OH Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 OH Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 OH Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 OH Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 OH Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 CH Level | 0 / 1 / 63 / 71 / 127 | OFF / -36dB / -0.1dB / +1.0dB / +6.0dB | 20·log10(gain), gain from drum808_param_si (LIN/EXP pot table) |
| 808 CH Tune | 0 / 1 / 63 / 71 / 127 | -12st / -12st / -0.1st / +1.4st / +12st | 12·log2(ratio) / TU12 semitone law in drum808_param_si |
| 808 CH Decay | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CH Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 CH Rev | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CH Dly | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in drum808_param_si |
| 808 CH Pan | L64 / L63 / L1 / R7 / R63 | L64 / L63 / L1 / R7 / R63 | unchanged (no confirmed unit) |
| 808 KIT Level | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 808 KIT Accent | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| 303  CrWave | TRI / TRI / SAW / SAW / SQR | TRI / TRI / SAW / SAW / SQR | unchanged (no confirmed unit) |
| 303  CrLvl | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Sub | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  CrPit | 0 / 1 / 63 / 71 / 127 | -12st / -12st / 0st / +1st / +12st | 12·log2(ratio) / TU12 semitone law in ui.c_param_si |
| 303  Glide | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  XmLvl | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  XmWave | TRI / TRI / SAW / SQR / PUL | TRI / TRI / SAW / SQR / PUL | unchanged (no confirmed unit) |
| 303  XmTune | -5.00 / -4.92 / -0.08 / +0.56 / +5.00 | -5.00 / -4.92 / -0.08 / +0.56 / +5.00 | unchanged (no confirmed unit) |
| 303  XmDep | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Cutoff | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Crush | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  WobShp | TRI / TRI / DOWN / UP / SQR | TRI / TRI / DOWN / UP / SQR | unchanged (no confirmed unit) |
| 303  WobDep | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  WobRat | 1/2 / 1/2 / 1/16 / 1/3 / 1/24 | 1/2 / 1/2 / 1/16 / 1/3 / 1/24 | unchanged (no confirmed unit) |
| 303  Output | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Stereo | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  PBend | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Damper | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Wave | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Glide | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Cutoff | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Peak | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  EnvAmt | 0 / 1 / 63 / 71 / 127 | -100% / -98% / -2% / 11% / 100% | pot/127 depth in ui.c_param_si |
| 303  Bite | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Gnaw | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Attack | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Decay | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Sust | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Rel | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  AmpMod | ENV / ENV / ENV / GATE / GATE | ENV / ENV / ENV / GATE / GATE | unchanged (no confirmed unit) |
| 303  Level | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  ArpOn | OFF / OFF / OFF / ON / ON | OFF / OFF / OFF / ON / ON | unchanged (no confirmed unit) |
| 303  ArpMod | DOWN / DOWN / UP / UP / UP/DN | DOWN / DOWN / UP / UP / UP/DN | unchanged (no confirmed unit) |
| 303  ArpRng | 1 / OCT / 1 / OCT / 2 / OCT / 2 / OCT / FULL | 1 / OCT / 1 / OCT / 2 / OCT / 2 / OCT / FULL | unchanged (no confirmed unit) |
| 303  ArpSpd | 1/2 / 1/2 / 1/8D / 1/4T / 1/32 | 1/2 / 1/2 / 1/8D / 1/4T / 1/32 | unchanged (no confirmed unit) |
| 303  FxEd1 | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  FxEd2 | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  PBend | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| 303  Damper | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in ui.c_param_si |
| BRK BREAK Level | 0 / 1 / 63 / 71 / 127 | OFF / -42dB / -6.1dB / -5.1dB / 0.0dB | 20·log10(gain), gain from engine_param_si (LIN/EXP pot table) |
| BRK BREAK Pitch | 0 / 1 / 63 / 71 / 127 | -12st / -11st / 0st / +1st / +12st | 12·log2(ratio) / TU12 semitone law in engine_param_si |
| BRK BREAK Rev | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| BRK BREAK Dly | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| FX FX Volume | 0 / 1 / 63 / 71 / 127 | OFF / -42dB / -6.1dB / -5.1dB / 0.0dB | 20·log10(gain), gain from fxbus_param_si (LIN/EXP pot table) |
| FX FX Drive | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| FX FX Comp | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in fxbus_param_si |
| FX FX RvDec | 0 / 1 / 63 / 71 / 127 | 0 / 1 / 63 / 71 / 127 | unchanged (no confirmed unit) |
| FX FX RvTone | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in fxbus_param_si |
| FX FX RvHPF | 0 / 1 / 63 / 71 / 127 | 30Hz / 31Hz / 153Hz / 188Hz / 800Hz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| FX FX RvLvl | 0 / 1 / 63 / 71 / 127 | OFF / -41dB / -4.5dB / -3.5dB / +1.6dB | 20·log10(gain), gain from fxbus_param_si (LIN/EXP pot table) |
| FX FX DlFdbk | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 42% / 48% / 85% | pot/127 depth in fxbus_param_si |
| FX FX DlTone | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in fxbus_param_si |
| FX FX DlLvl | 0 / 1 / 63 / 71 / 127 | OFF / -41dB / -4.5dB / -3.5dB / +1.6dB | 20·log10(gain), gain from fxbus_param_si (LIN/EXP pot table) |
| FX FX DlHPF | 0 / 1 / 63 / 71 / 127 | 30Hz / 31Hz / 153Hz / 188Hz / 800Hz | EXP pot table min·(max/min)^(pot/127) (build/gen/x0x_drum_tables.h) |
| FX FX Wear | 0 / 1 / 63 / 71 / 127 | 0% / 1% / 50% / 56% / 100% | pot/127 depth in fxbus_param_si |
| MIAMI PITCH | 0 | 0st | carrier semitone offset (Miami id 3 CrPit) |
| BRK Pitch | 0 | 0st | break pitch semitones (T_BRK c=1) |
| CHICAGO * | % | % (unchanged) | already had % |
