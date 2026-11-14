# Run your DnBOX music workspace locally

This launcher assembles the three original owner-delivered archives on your computer. It contains only the assembler, its tests and the website integration code. It includes no component payload, recording, screenshot, firmware or SDK. It fetches nothing and publishes nothing.

Use Python 3.8 or later. No Node, package installation, build step or symlink support is needed.

1. Create a parent folder for your local workspace. Unpack this launcher into a child folder named `launcher`.
2. Beside `launcher`, create `original-downloads` and download the three **original files** below from your existing Library deliveries into it. Keep those archives intact. Open a terminal inside `launcher`.
3. Run:

```sh
python3 assemble_workspace.py --inputs-dir ../original-downloads --output ../DnBOX-local --serve 8000
```

On Windows, use `py -3` in place of `python3`; generated Windows restart commands are labelled for PowerShell and use literal arguments. If the downloads are elsewhere, give their full folder path in quotes. Open **http://127.0.0.1:8000/workspace/#editor** after the command prints the verified host hash. Keep the terminal open while using the workspace; Ctrl+C stops the server and leaves your assembled files intact. Use `--serve 0` to select a free local port automatically and open the exact address printed by the command. If assembly completed but the chosen port was occupied, use the restart command below with a free port.

The folders must be siblings, as shown below. Keep downloads and assembled output **outside** the code-only `launcher` folder so its whitelist stays verifiable.

```text
your-workspace/
  launcher/              assemble_workspace.py, README.md, manifests, host-overrides/, tests/
  original-downloads/    the three intact original archives
  DnBOX-local/           created by the command
```

| Original archive | Exact SHA-256 |
| --- | --- |
| `DnBOX-web-learning-reviewed-20261114.tar.xz` | `75374889aab7fb631fe3293e5e52c9fe1f547852c683f2d67983a9de6dd61a6f` |
| `DnBOX-local-music-preparation-integrated-20261114.tar.gz` | `f0f3e8fb0a1106a3edeaae27568b05b76de755bf65f832c9868c1d781eb18912` |
| `DnBOX-performance-companion-R4-20261114.zip` | `a295c44f8c6c1a4c53a3559505713abc575bdcace9157f90809f9e74ddc1b11d` |

Renamed original downloads are recognized by their exact hash. Explicit paths also work:

```sh
python3 assemble_workspace.py --website ../original-downloads/DnBOX-web-learning-reviewed-20261114.tar.xz --preparation ../original-downloads/DnBOX-local-music-preparation-integrated-20261114.tar.gz --companion ../original-downloads/DnBOX-performance-companion-R4-20261114.zip --output ../DnBOX-local --serve 8000
```

Choose a **new or empty** output folder. The assembler rejects a wrong archive, unsafe archive entry or nonempty output; it never replaces an existing workspace. Original archives remain unchanged. The result contains `host/`, private extracted original preparation/companion packets, a full served-file manifest and `ASSEMBLY-RECEIPT.json`. Its server binds only to `127.0.0.1`; there is no public-host option or CORS change. Keep the assembled output local: it contains the original private component deliveries.

To restart an existing assembly later:

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory ../DnBOX-local/host
```

The website base is the 70-file integrated learning site. The assembler retains its manifest as `provenance/public-site-MANIFEST-ed518381.sha256`, applies only nine owned website integration/guide/provenance files and installs the supplied 75-entry `expected-host-MANIFEST.sha256`. It verifies all 69 preparation source entries, all 62 companion packet entries and all 16 R4 candidate entries. Component directories are copied byte-for-byte into `/tools/`, `/librarian/` and `/performance/`; no component source is rewritten.

The preparation version is source `6d98989c716d8067280f1a1c4934da57fd05cfd6`, with the versioned preset source-bank profiles. The companion is R4 on its recorded `83fc6d6` source contract. The exact private host manifest is `99e51da2442eac9e081f8698018985159d157cde1d5bdb0dcaf029638ad1c8d6`. Assembly reproduces those reviewed served bytes, including the final version/help guide; it does not claim another firmware or device qualification.

The editor is silent and saves local JSON drafts. Its raw revision-13 project subset is narrower than portable `.x0xproject` backup; unsupported formats, sound exports and device sync remain unavailable. Preset files hold parameters and sample references, not sample audio. The companion demo is silent and simulated; physical MIDI remains untested and requires separate explicit browser actions and a matching profile. Assembly performs no device operation, firmware installation or public deployment, and does not clear any existing export/publication refusal.
