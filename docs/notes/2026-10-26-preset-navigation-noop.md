# Preserve edits at clamped preset-navigation endpoints

Independent actual-source reproduction found that one extra outward K4/K6
detent on the first or last preset republished the same factory request. Its
next audio boundary discarded the edited sound even though the displayed
preset index had not changed. No-input controls retained edits; explicit API
reselection correctly reloaded factory values.

The UI selector now refuses an unchanged navigation target after clamping,
before publishing a preset request or marking the project dirty. The same
rule applies to the ENGINE selector so an unchanged bank does not silently
select preset zero. It compares the existing requested target, not merely the
active sound: a waiting request remains intact. USER (-1) can still enter the
first preset. This is a no-op guard, not preset wrapping or a new wait policy.
The general engine selection API still permits explicit same-preset reload.

`preset_navigation_noop_test.sh` uses production input and audio paths, checks
both voices, both engines, both endpoints and physical K4/K6/K5 routes, and
asserts no new publication, no dirty/save mutation, no changed sound/tag bytes
and no unintended index change. Positive controls cover the next real adjacent
request, deferred preset/bank targets, USER entry and intentional API reload.
The unchanged parent compiles but fails the no-publication assertion at runtime.

This is separate from the approved caption/wrap stage a55eb8a and the full Elite
DEV package at 8b4221d. Its own focused proof and review do not relabel historical
packages or claim hardware qualification.
