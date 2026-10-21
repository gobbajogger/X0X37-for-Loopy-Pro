# Bound sample comparison after installation

`tools/fm1_install.py --verify-slots` makes a verified archive of every logical
sample slot before writing, including when the package keeps the current slot
map. It then performs the normal firmware install, any necessary migration,
and selected controller verification. Finally it uses the existing updater's
bound, verified target for a read-only comparison with the saved images.

For each occupied slot, the comparison covers its complete 512-byte header and
all used ADPCM payload bytes. It logs both the actual and expected image CRC32
and compares the bytes, including header bytes 480 through 511. Empty-slot
metadata is checked through `SMP_INFO`. Unused flash padding is not compared.
The post-install comparison sends only `SMP_INFO` and `SMP_READ`.

The package's expected slot map must match the running firmware. Preserved
images are matched by flash offset; migrated images retain the original backup
slot number in the log. A missing destination, a changed image, an unexpected
sample in an empty slot, a changed identity, or a failed read stops verification.
The saved archive is retained. Verification never triggers another erase or
restore. The option is incompatible with `--no-slots` and refuses an already
running loader or a package without X0X sample slots before writing.

The initial layout check conservatively uses rounded KiB usage from `SMP_INFO`.
After all full images and the saved archive are verified, the migration decision
uses each image's exact length, including its 512-byte header, and the selected
package's destination capacities. A valid image that fills a retained slot must
stay there; rounding must not cause an unnecessary copy into another slot. If
the backup is unavailable, installation still stops before firmware writes.

The same updater remains responsible for every stage. Its verified MIDI port
name is frozen after reboot. A new bystander or a replacement port is refused
before another unit is opened. As documented in the earlier installer audit,
the protocol has no stable per-unit serial, so multiple update-compatible
keyboards must be disconnected for the update.

## Offline evidence

Tested on this addendum's source, based on installer repair commit
`c01629eeba48c5fd05aabf3af5eb6511083ef84e`:

- `python3 -B tests/install_slot_verify_test.py`: 9 tests passed, with Pro and
  Elite subcases. Covers normal port renaming, every logical sample image,
  empty-slot metadata, a reserved-header byte mismatch, unexpected sample data,
  slot migration, an unexpected map, post-verification bystanders and replacement
  ports, implicit backup, and refusal of unverifiable targets. Boundary subcases
  fill each of the three dynamic slots exactly on Pro and Elite and prove that
  no sample erase or write occurs. A moved destination with smaller capacity
  still triggers a necessary migration into a sufficiently large free slot.
- `python3 -B tests/install_slots_test.py`: passed, including backup, migration,
  partial restoration, stock firmware and explicit opt-out behavior.
- `python3 -B tests/install_ctl_test.py`: passed, including complete controller
  block and erased-padding comparison and controller read failure handling.
- `python3 -B tests/installer_safety_test.py`: 11 tests passed, preserving the
  previous wrong-device, failed-layout, upload-capacity and full-header fixes.

The integration owner registers `install-slot-verify` in `tests/run_tests.sh`
and includes it in the final development validation plan. The personal Elite
launcher enables `--verify-slots` and retains per-slot CRC output in its native
installation log. No firmware build, installation or physical device readback
was performed for this helper addendum. Hardware CRC results remain pending the
actual guarded installation.
