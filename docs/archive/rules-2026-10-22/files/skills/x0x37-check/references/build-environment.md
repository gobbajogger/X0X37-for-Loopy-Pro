# Build and test environment

Use dot's cloud machine for development under [routing](../../x0x37-task/references/routing.md).
Host tests require a supported host; JieLi target tools require Linux x86-64.
Verify the actual executable and supported runtime rather than assuming a copied tool works.
Use actual mounted paths, including linked-worktree Git metadata. Never change `HOME` to imitate another machine.
Reserve the local Mac for required installation and hardware access.

Before reusing approved assets, verify their readable executor-local path, provenance,
version or commit and hashes, required libraries, and compatibility with the selected
source and configuration. Keep exact DSP reference pins and sample identities.
SDK and toolchain availability does not prove a controller rebuild is supported.
Do not commit or bundle proprietary JieLi SDK/toolchain files; keep `refs-oscuro` excluded.
Use [evidence reuse](validation.md) for retained controllers and prior checks.

Set `JIELI_TOOLCHAIN` or `X0X_TOOLCHAIN`, `AC79_SDK`, and reference variables as required.
Set `CC` and `CXX` to single executable names, such as `CC=cc CXX=c++`. Do not put flags in them.
A missing reference, tool, dependency, or device blocks only dependent work.
