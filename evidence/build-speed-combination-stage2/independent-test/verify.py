#!/usr/bin/env python3
"""Verify final affected candidate. Initial full identity probe is retained as history."""
from pathlib import Path
import runpy
runpy.run_path(str(Path(__file__).with_name('verify-affected.py')), run_name='__main__')
