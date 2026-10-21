# Targeted vendor size continuation

The owner reopened optimization after the frozen checkpoint. These are actual
strict, fully resolved target links, not an installable profile. App and updater
work are separate; no musical feature, sample capacity, security capability or
guard was removed.

| Variant | Stored bytes | Saving from frozen |
| --- | ---: | ---: |
| Frozen `ca6c21b` | 104144 | 0 |
| Inline-hint threshold zero | 104144 | 0 |
| Classic DUT/FCC test flags zero | 103988 | 156 |
| Above + Classic key-delete/auto-sniff flags zero | 103976 | 168 |
| Above + Classic LMP name/passkey/eSCO flags zero | 103976 | 168 |
| Key/sniff variant + backend tail-merge probe | 103976 | 168 |

All links report zero unresolved imports. Low RAM26352 and high CPU RAM6444 are
unchanged. The useful variant has primary93180, constants7272 and RAM-code load3524
bytes. The zero-gain LMP and compiler probes are not proposed runtime changes.
The tail-merge option is disabled by default in this compiler; no benefit was
found, so no adoption is justified.

## Semantic boundary

The exact pinned SDK `apps/common/config/log_config/lib_btctrler_config.c` exposes the
configuration scalars (definitions55–57,79–82). Private exact-module inspection
found DUT reads only in Classic `bredr_link` transmit/close/RX test branches; FCC
reads only in `bredr_fcc` parameter/init branches. Zero is an explicit disable
branch, not an invented function stub. The retained
`link_conn_tx_bulk_avaiable` drops from210 to76 bytes. Calibration functions with
`test` in their names were NOT removed.

`config_delete_link_key` is read in Classic HCI remote-address/status handling,
not the LE SM device database. `config_btstask_auto_exit_sniff` is read in Classic
AVCTP operation control. Changing these only saves12 additional linked bytes;
`delete_link_key` itself remains164 bytes due to other retained references.

LE encryption feature1 and SM support1 remain. The linked sizes of `sm_run`,
`sm_event_packet_handler`, `gatt_client_att_packet_handler`, `ll_hci_encryption`,
`wf_rf_common_init` and `wf_phy_init` are unchanged across the useful probes.
This symbol check supports the scoped configuration analysis, but is not a
hardware regression test or proof of every reachable protocol behavior.

## Reproduction and limitations

Use `tools/studies/smc_reference/link_runtime.py` with the frozen README command
and append the corresponding optional switches:

- `--inlinehint-threshold 0`
- `--no-classic-test-modes`
- `--no-classic-test-modes --no-classic-key-sniff`
- above plus `--no-classic-lmp` or `--tail-merge-probe`

Each output must be a new outside-repository directory. The runner records its
exact link command and private input/source/artifact hashes. Companion JSON
retains each result; proprietary archives, IR and target binaries remain private.
No default build configuration is changed. The current measured168-byte saving
is insufficient alone: the prior aggregate deficit1108 becomes940 before the
remaining indivisible update/admission work. No complete fit is claimed.
