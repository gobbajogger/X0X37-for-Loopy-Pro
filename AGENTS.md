# x0x37

For firmware or other code changes, follow [the full-cycle workflow](docs/rules/workflow.md). The process is a loop: a valid nightly build goes to the nightly installer page, and that publish starts the next audit in a new thread. A stage with no new defect returns to audit the same way. A stage hands off only by starting a Cloud Agent the owner can open, then pushing that agent's URL. An internal subagent is not that handoff.

A docs-only edit stays in the current thread. It does not open an audit, implementation, test, validation, merge, install, or nightly-publish thread.

That workflow is the current stage procedure. It replaces one-chat stage routing. Leave the removed skill pack, ChatGPT project routing, and Codex `create_thread` instructions unrestored.
