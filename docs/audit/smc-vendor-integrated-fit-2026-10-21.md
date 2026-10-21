# Integrated vendor TEST source: real app partition now fits

This supersedes the frozen checkpoint's **app size deficit only**, not its missing
runtime/update admission. No firmware package, release or hardware test occurred.
The compile-time study-only activation interlock is unchanged.

Reviewed app commits25b5650/b5a2903 were integrated as e61a515/4e03005; clean export
recipe8ae22d1 adds the changed drum808 source. A fresh baseline816 archive with the
three study preparation scripts rebuilt the real bridge at **521108 B**, leaving
**2892 B** below524000. Its exact SHA256 is
`d608dd0ea8d70962812f5c0a15582ebf0a04fe9c83377d66653861f256cbd0b7`, matching the
independently reviewed optimizer artifact. Main RAM98100, BLE RAM2936 and pool
335484 are unchanged; the8192-byte pool guard remains. Original pot tables are
retained. Hot render/shaper/hat bodies are unchanged in target assembly, but the
outlined coefficient transition path adds12 B stack: this is not total WCET or
stack-high-water proof. Sustained native DSP tests cover66×131072 samples plus
shorter input/state comparisons; target execution remains pending.

The optional reviewed Classic-only config probe reduces the vendor controller to
**103976 B** in its fixed primary/data/load ranges; low RAM26352 and highCPU6444
are unchanged. This removes no LE SM/encryption or calibrated RF path. It needs no
shared erase sector or compressed code to meet its own storage bounds. Source
runner configuration is explicit, not a change to the default controller build.

Actual ELF fixtures replace the128-byte placeholder with the real segmented
manifest without adding bytes. Production-validator native and sanitizer tests
cover model, every header byte, truncation and payload corruption. These are
private validation fixtures, not installation files. The generator does not seal
appdata or create a complete bundle.

Remaining gates include actual indivisible updater/paired rollback and staged
record ownership, independent appdata binding, never-started recovery admission
or source-proven RF/DMA shutdown, live heap/stack/deadline bounds and hardware
transport/control verification. A normal-session scan/disconnect command return
is NOT proof that RF/DMA is quiescent. No low-heap allocation semantics were
silently changed merely to make memory arithmetic pass.

Private ELF/map/bin hashes are in the companion JSON. These artifacts remain
outside Git. Reproduce with the README's baseline archive + prepare/admission/
bridge scripts and exact pinned toolchain. The ordinary build ceiling printed by
the diagnostic export is not a substitute for comparing against524000 here.
