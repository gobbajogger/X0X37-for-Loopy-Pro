#!/usr/bin/env python3
"""Run the production corner decoder against independent 4x4 circle coverage."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path(os.environ.get('CORNER_SOURCE_FILE', ROOT / 'firmware/src/app/ui.c'))
text = SOURCE.read_text()


def extract(pattern):
    match = re.search(pattern, text, re.S | re.M)
    if not match:
        raise SystemExit('production corner table/decoder definition missing')
    return match.group(0)


definitions = '\n'.join([
    extract(r'^static const uint8_t CORNER_OFF\[[^\n]+;'),
    extract(r'^static const uint8_t CORNER_PACKED\[\d+\] = \{.*?^};'),
    extract(r'^static __attribute__\(\(noinline\)\) uint32_t corner_coverage\(uint32_t index\)\n\{.*?^}'),
    extract(r'^#define corner_fill_at[^\n]+'),
    extract(r'^#define corner_ring_at[^\n]+'),
])

# Count original geometric samples, independently of the nibble codec and table.
expected = []
for ring in (False, True):
    for radius in range(1, 9):
        for y in range(radius):
            for x in range(radius):
                coverage = 0
                for sy in range(4):
                    for sx in range(4):
                        dx = 8 * radius - 8 * x - 2 * sx - 1
                        dy = 8 * radius - 8 * y - 2 * sy - 1
                        distance = dx * dx + dy * dy
                        coverage += distance < 64 * radius * radius and (
                            not ring or distance >= 64 * (radius - 1) ** 2)
                expected.append(coverage)
assert len(expected) == 408
assert 8 not in expected and set(expected) == set(range(17)) - {8}

program = '''#include <stdint.h>
#include <stdio.h>
#define RMAX 8
''' + definitions + '\nstatic const uint8_t expected[408] = {' + ','.join(map(str, expected)) + '''};
int main(void)
{
    unsigned checks = 0;
    if (sizeof CORNER_PACKED != 204u) return 2;
    for (unsigned n = 0; n < 408; n++) {
        if (corner_coverage(n) != expected[n]) {
            fprintf(stderr, "corner %u: got %u expected %u\\n", n,
                    (unsigned)corner_coverage(n), (unsigned)expected[n]);
            return 1;
        }
        checks++;
    }
    unsigned n = 0;
    for (unsigned r = 1; r <= RMAX; r++)
        for (unsigned j = 0; j < r; j++)
            for (unsigned i = 0; i < r; i++) {
                if (corner_fill_at(r, j, i) != expected[n] ||
                    corner_ring_at(r, j, i) != expected[204u + n]) return 1;
                n++;
                checks += 2;
            }
    printf("corner coverage: %u production-decoder and geometry checks passed\\n", checks);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='x0x-corner-') as work:
    source = Path(work) / 'test.c'
    executable = Path(work) / 'test'
    source.write_text(program)
    subprocess.run([os.environ.get('CC', 'cc'), '-std=c99', '-O1', '-Wall', '-Wextra',
                    '-Werror', *shlex.split(os.environ.get('SAN_FLAGS', '')),
                    str(source), '-o', str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
