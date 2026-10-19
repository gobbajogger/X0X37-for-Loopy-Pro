#!/bin/sh
# Fixture for tests/host/test_gate_trap.sh (checks 5 and 13): a build that waits, so the test can stop the gate
# during step 1 (the builds). It never builds anything.
echo "fixture build: waiting"
sleep 60
