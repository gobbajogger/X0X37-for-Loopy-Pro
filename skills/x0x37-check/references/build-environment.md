# Build and test environment

Host tests can run on supported macOS or Linux. JieLi target tools require Linux x86-64. You can use a working local container.
Use actual mounted paths, including linked-worktree Git metadata. Never change `HOME` to imitate another machine.

Set `JIELI_TOOLCHAIN` or `X0X_TOOLCHAIN`, `AC79_SDK`, and reference variables as required. Keep exact reference pins.
Set `CC` and `CXX` to single executable names, such as `CC=cc CXX=c++`. Do not put flags in them.

A missing optional reference blocks only its dependent check.
