# Mapped knob captions and wrapping synth rows

The owner noticed that K5-8 claimed controls which were no longer mapped, asked
for KNOBS instead of K, chose ROW with an up/down glyph, and requested wrapping
where sensible. This bounded child follows those requests without renumbering
or adding any parameter mappings.

The active matrix row now lists only its nonempty parameter references. SOUND
shows KNOBS 5-6, full rows KNOBS 5-8, Chicago sustain KNOB 6, and Miami PITCH / FX
KNOBS 5,7. Contiguous runs are compressed; gaps are never advertised as controls.
The logical simulator legends use the corresponding 1-4 numbering.

The footer retains physical < > and uses the existing primitive up/down glyph
next to ROW. PATCH cancellation/back and PARA next remain contextual actions.
No unsupported font characters or new image assets are required.

SELECT (the physical arrows' existing route) now wraps only the Chicago/Miami
sound matrix: last to first and first to last. It previously clamped; the old
boundary assertions are intentionally updated for the owner's new behavior.
Other views, lists, held-step chance and HOME-held tempo retain their routes.
This is not a global navigation or parameter-value wrap change.

Focused tests cover actual framebuffer headers and footers, both engines and
voices, physical/logical captions, every sparse mask, real preset-name widths,
pending actions, nonmutation, both wrap directions across full/partial banks,
and unchanged clamps outside this matrix. The prior clamp is a runtime RED;
compilation failure is not accepted as a negative. Existing affected matrix and
status consumers are updated without weakening their other assertions.

Frozen predecessor 8b4221d retains its separate Sol/native receipts. This child
requires its own scoped review and affected checks; no old package is relabeled
as the child, and no hardware or rollout approval is implied.
