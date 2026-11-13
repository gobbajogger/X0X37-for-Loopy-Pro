#!/bin/sh
# A good replay test: it fails on the code with the bug (app.txt "bug") and passes after the fix.
grep -qx fixed app.txt
