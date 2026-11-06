LX-REQ-016 / LX-REQ-023: isolated NIGHTLY version integration proposal.
Base: PR698 48e92b76c2f476cf40771e00a04e1838136bb55d.
Donor: c32c54ac8ec68aaca53adccf7af66f155590f2dc, tree ac490f7fae9198748aa54bd914d42f99d45444f9.
Plan: authenticate inputs; port only build version flags, decrypted ABOUT package validation, and relevant regression tests; preserve security checks, allocation semantics and shared ownership; test positive/negative behavior; deliver an unselected patch and exact proposed tree to parent/sole integrator.
No shared ref, ledger allocation/reset, merge, publication or device work. This lane is the bounded patch producer; existing independent QA and sole integrator retain review/selection ownership.
Acceptance: existing package and allocator tests plus donor version regressions and 0.0.25 positive/negative controls pass. Native/package outputs and final release qualification remain downstream.
