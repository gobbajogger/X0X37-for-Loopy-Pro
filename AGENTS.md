# x0x37

For firmware or other code changes, follow [the full-cycle workflow](docs/rules/workflow.md). The process keeps running: a valid nightly build goes to the nightly installer page, and that publish starts the next audit in a new thread. A stage hands off only after that new thread's id is pushed.

A docs-only edit stays in the current thread. It does not open an audit, implementation, test, validation, merge, install, or nightly-publish thread.

That workflow is the current stage procedure. It replaces one-chat stage routing. Leave the removed skill pack, ChatGPT project routing, and Codex `create_thread` instructions unrestored.
