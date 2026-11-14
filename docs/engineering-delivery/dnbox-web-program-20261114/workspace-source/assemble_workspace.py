#!/usr/bin/env python3
"""Assemble the owner's original DnBOX downloads locally; Python 3.8+ stdlib only."""
import argparse
import functools
import hashlib
import http.server
import json
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import stat
import sys
import tarfile
import tempfile
import unicodedata
import zipfile

INPUTS = {
    "website": ("DnBOX-web-learning-reviewed-20261114.tar.xz", "75374889aab7fb631fe3293e5e52c9fe1f547852c683f2d67983a9de6dd61a6f"),
    "preparation": ("DnBOX-local-music-preparation-integrated-20261114.tar.gz", "f0f3e8fb0a1106a3edeaae27568b05b76de755bf65f832c9868c1d781eb18912"),
    "companion": ("DnBOX-performance-companion-R4-20261114.zip", "a295c44f8c6c1a4c53a3559505713abc575bdcace9157f90809f9e74ddc1b11d"),
}
OVERRIDES = (
    "index.html", "app.js", "journeys.js", "private-workspace.js", "workspace/index.html",
    "workspace/workspace.css", "workspace/workspace.js", "offline-sequencer/index.html",
    "provenance/integration-20261114.md",
)
LAUNCHER_FILES = {"assemble_workspace.py", "README.md", "expected-host-MANIFEST.sha256", "tests/test_assembler.py"} | {"host-overrides/" + n for n in OVERRIDES}
PUBLIC_MANIFEST_SHA = "ed518381e86d16fa2a104d156912b5da11d4c180b9f02764e45d4c89554ee00a"
PREPARATION_MANIFEST_SHA = "10858f70071b9fc4e5c9f79c410dc28f34f1fe135d0b3fd9dd5f2f921d317237"
R4_MANIFEST_SHA = "aeae6c83eb8cba584ff7e35d6d729a53a4e90e7e5fb8e4749f922a54718f03a7"
WEBSITE_PREFIX = "integrated-web-learning/site/"
PREPARATION_ROOT = "dnbox-local-music-packet-20261114"
COMPANION_ROOT = "performance-companion"
MAX_MEMBERS = 20000
MAX_MEMBER_BYTES = 256 * 1024 * 1024
MAX_EXPANDED_BYTES = 512 * 1024 * 1024
RECEIPT_NAME = "ASSEMBLY-RECEIPT.json"


class AssemblyError(Exception):
    pass


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def safe_name(name):
    """Reject paths that escape, alias or fail on common Windows/macOS filesystems."""
    if not isinstance(name, str) or not name or "\\" in name or ":" in name or any(ord(c) < 32 for c in name):
        raise AssemblyError("Unsafe archive or manifest path: " + repr(name))
    value = name.rstrip("/")
    path = PurePosixPath(value)
    if path.is_absolute() or not path.parts or any(p in (".", "..", "") for p in value.split("/")):
        raise AssemblyError("Unsafe archive or manifest path: " + repr(name))
    reserved = {"con", "prn", "aux", "nul"} | {"com" + str(i) for i in range(1, 10)} | {"lpt" + str(i) for i in range(1, 10)}
    if any(p.endswith((" ", ".")) or p.split(".")[0].casefold() in reserved for p in path.parts):
        raise AssemblyError("Non-portable archive or manifest path: " + repr(name))
    return path.as_posix()


def collision_key(name):
    return unicodedata.normalize("NFC", name).casefold()


def validate_members(members):
    if len(members) > MAX_MEMBERS:
        raise AssemblyError("Archive has too many entries")
    seen = {}
    total = 0
    for name, directory, size in members:
        clean = safe_name(name)
        key = collision_key(clean)
        if key in seen:
            raise AssemblyError("Duplicate or case-colliding archive entry: " + clean)
        if size < 0 or size > MAX_MEMBER_BYTES:
            raise AssemblyError("Archive member exceeds size limit: " + clean)
        total += size
        seen[key] = directory
    if total > MAX_EXPANDED_BYTES:
        raise AssemblyError("Archive exceeds expanded-size limit")
    for name, unused_directory, unused_size in members:
        parts = safe_name(name).split("/")
        for i in range(1, len(parts)):
            if seen.get(collision_key("/".join(parts[:i]))) is False:
                raise AssemblyError("Archive file conflicts with a directory: " + name)


def scan_tar(path):
    with tarfile.open(path, "r:*") as archive:
        info = archive.getmembers()
        for member in info:
            if not (member.isfile() or member.isdir()) or member.issym() or member.islnk() or getattr(member, "sparse", None):
                raise AssemblyError("Unsupported archive entry type: " + member.name)
        validate_members([(m.name, m.isdir(), m.size) for m in info])
        return info


def scan_zip(path):
    with zipfile.ZipFile(path) as archive:
        info = archive.infolist()
        for member in info:
            mode = stat.S_IFMT(member.external_attr >> 16)
            if member.flag_bits & 1 or mode not in (0, stat.S_IFREG, stat.S_IFDIR):
                raise AssemblyError("Encrypted or unsupported ZIP entry: " + member.filename)
        validate_members([(m.filename, m.is_dir(), m.file_size) for m in info])
        return info


def write_member(source, destination, expected_size):
    destination.parent.mkdir(parents=True, exist_ok=True)
    count = 0
    with destination.open("xb") as output:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            count += len(block)
            if count > expected_size or count > MAX_MEMBER_BYTES:
                raise AssemblyError("Archive data exceeds declared size: " + str(destination))
            output.write(block)
    if count != expected_size:
        raise AssemblyError("Archive data is truncated: " + str(destination))


def extract_tar(path, destination, selected_prefix=None):
    members = scan_tar(path)
    count = 0
    with tarfile.open(path, "r:*") as archive:
        for member in members:
            name = safe_name(member.name)
            if selected_prefix is not None:
                if not name.startswith(selected_prefix):
                    continue
                name = name[len(selected_prefix):]
                if not name:
                    continue
            target = destination.joinpath(*safe_name(name).split("/"))
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                with archive.extractfile(member) as source:
                    write_member(source, target, member.size)
                count += 1
    return count


def extract_zip(path, destination):
    members = scan_zip(path)
    count = 0
    with zipfile.ZipFile(path) as archive:
        for member in members:
            target = destination.joinpath(*safe_name(member.filename).split("/"))
            if member.is_dir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                with archive.open(member) as source:
                    write_member(source, target, member.file_size)
                count += 1
    return count


def parse_manifest(text):
    result = {}
    seen = set()
    for line in text.splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  (.+)", line)
        if not match:
            raise AssemblyError("Malformed SHA-256 manifest line")
        value, name = match.groups()
        name = safe_name(name)
        if collision_key(name) in seen:
            raise AssemblyError("Duplicate manifest entry: " + name)
        seen.add(collision_key(name))
        result[name] = value
    if not result:
        raise AssemblyError("Empty manifest")
    return result


def files_under(root):
    result = {}
    for path in sorted(root.rglob("*")):
        if path.is_symlink():
            raise AssemblyError("Unexpected symlink: " + str(path))
        if path.is_file():
            result[path.relative_to(root).as_posix()] = digest(path)
    return result


def verify_files(root, expected, count=None):
    if count is not None and len(expected) != count:
        raise AssemblyError("Unexpected manifest entry count")
    for name, value in expected.items():
        path = root.joinpath(*safe_name(name).split("/"))
        checksum = value if isinstance(value, str) else value["sha256"]
        if not path.is_file() or path.is_symlink() or digest(path) != checksum:
            raise AssemblyError("File hash mismatch: " + name)
        if isinstance(value, dict) and path.stat().st_size != value["bytes"]:
            raise AssemblyError("File size mismatch: " + name)


def verify_launcher(root):
    expected = parse_manifest((root / "LAUNCHER-MANIFEST.sha256").read_text(encoding="utf-8"))
    actual = set(files_under(root)) - {"LAUNCHER-MANIFEST.sha256"}
    if set(expected) != LAUNCHER_FILES or actual != LAUNCHER_FILES:
        raise AssemblyError("Launcher contents differ from the code-only whitelist")
    verify_files(root, expected)
    own = parse_manifest((root / "expected-host-MANIFEST.sha256").read_text(encoding="utf-8"))
    if len(own) != 75 or any(n.startswith(("tools/", "librarian/", "performance/")) for n in own):
        raise AssemblyError("Unexpected own-host manifest scope")
    for name in OVERRIDES:
        if digest(root / "host-overrides" / name) != own.get(name):
            raise AssemblyError("Host override differs from the frozen manifest: " + name)
    return own


def ensure_fresh_output(output):
    if output.is_symlink() or (output.exists() and (not output.is_dir() or any(output.iterdir()))):
        raise AssemblyError("Output must be a new or empty directory; nothing was overwritten: " + str(output))


def resolve_inputs(args):
    found = {}
    directory = Path(args.inputs_dir).expanduser() if args.inputs_dir else None
    for kind, (filename, checksum) in INPUTS.items():
        explicit = getattr(args, kind)
        if explicit:
            path = Path(explicit).expanduser()
        elif directory is not None:
            path = directory / filename
            if not path.is_file():
                matches = [p for p in directory.iterdir() if p.is_file() and digest(p) == checksum] if directory.is_dir() else []
                if len(matches) != 1:
                    raise AssemblyError("Place the original " + filename + " in --inputs-dir, or give --" + kind)
                path = matches[0]
        else:
            raise AssemblyError("Use --inputs-dir or provide all three explicit archive paths")
        if not path.is_file() or digest(path) != checksum:
            raise AssemblyError("Wrong SHA-256 for " + kind + "; output was not created: " + str(path))
        found[kind] = path.resolve()
    return found


def assemble(args, launcher_root=None):
    launcher_root = launcher_root or Path(__file__).resolve().parent
    output = Path(args.output).expanduser().absolute()
    ensure_fresh_output(output)
    own = verify_launcher(launcher_root)
    inputs = resolve_inputs(args)  # Check all archive bytes before creating output/staging directories.
    for kind in ("website", "preparation"):
        scan_tar(inputs[kind])
    scan_zip(inputs["companion"])
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".dnbox-assembly-", dir=str(output.parent)) as temporary:
        stage = Path(temporary) / "workspace"
        host = stage / "host"
        host.mkdir(parents=True)
        if extract_tar(inputs["website"], host, WEBSITE_PREFIX) != 70:
            raise AssemblyError("Website input must provide exactly 70 site files")
        public_manifest = host / "MANIFEST.sha256"
        if digest(public_manifest) != PUBLIC_MANIFEST_SHA:
            raise AssemblyError("Unexpected website base manifest")
        public = parse_manifest(public_manifest.read_text(encoding="utf-8"))
        verify_files(host, public, 69)
        if set(files_under(host)) != set(public) | {"MANIFEST.sha256"}:
            raise AssemblyError("Unexpected website files")
        renamed = host / "provenance/public-site-MANIFEST-ed518381.sha256"
        if renamed.exists():
            raise AssemblyError("Website provenance destination already exists")
        public_manifest.rename(renamed)
        private_inputs = stage / "original-inputs"
        prep_dir = private_inputs / "preparation"
        companion_dir = private_inputs / "companion"
        extract_tar(inputs["preparation"], prep_dir)
        extract_zip(inputs["companion"], companion_dir)
        prep = prep_dir / PREPARATION_ROOT
        companion = companion_dir / COMPANION_ROOT
        source_path = prep / "SOURCE.json"
        if digest(source_path) != PREPARATION_MANIFEST_SHA:
            raise AssemblyError("Unexpected preparation source manifest")
        source = json.loads(source_path.read_text(encoding="utf-8"))
        verify_files(prep, source["files"], 69)
        expected_web = {name[4:]: value for name, value in source["files"].items() if name.startswith("web/")}
        if files_under(prep / "web") != expected_web:
            raise AssemblyError("Preparation web closure differs from its source manifest")
        packet = json.loads((companion / "PACKET-MANIFEST.json").read_text(encoding="utf-8"))
        verify_files(companion, packet["files"], 62)
        if set(files_under(companion)) != set(packet["files"]) | {"PACKET-MANIFEST.json"}:
            raise AssemblyError("Unexpected companion packet files")
        r4_path = companion / "evidence/candidate-r4.json"
        if digest(r4_path) != R4_MANIFEST_SHA:
            raise AssemblyError("Unexpected companion candidate")
        r4 = json.loads(r4_path.read_text(encoding="utf-8"))
        if r4.get("candidate") != "R4":
            raise AssemblyError("Companion candidate is not R4")
        verify_files(companion, r4["files"], 16)
        for name in OVERRIDES:
            target = host / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(launcher_root / "host-overrides" / name, target)
        shutil.copyfile(launcher_root / "expected-host-MANIFEST.sha256", host / "MANIFEST.sha256")
        verify_files(host, own, 75)
        if set(files_under(host)) != set(own) | {"MANIFEST.sha256"}:
            raise AssemblyError("Assembled own-host scope differs from the frozen manifest")
        components = {
            "tools": prep / "web", "librarian": prep / "web/presets", "performance": companion / "dist",
        }
        component_counts = {}
        expected_served = dict(files_under(host))
        for namespace, source_root in components.items():
            expected = files_under(source_root)
            shutil.copytree(str(source_root), str(host / namespace))
            if files_under(host / namespace) != expected:
                raise AssemblyError("Copied component bytes differ: " + namespace)
            component_counts[namespace] = len(expected)
            expected_served.update({namespace + "/" + name: value for name, value in expected.items()})
        if files_under(host) != expected_served:
            raise AssemblyError("Unexpected assembled served file")
        served_manifest = "".join(value + "  " + name + "\n" for name, value in sorted(expected_served.items()))
        (stage / "SERVED-MANIFEST.sha256").write_bytes(served_manifest.encode("utf-8"))
        receipt = {
            "private_only": True, "result": "PASS", "own_manifest_sha256": digest(host / "MANIFEST.sha256"),
            "own_manifest_entries": 75, "website_site_files": 70, "preparation_source_entries": 69,
            "companion_packet_entries": 62, "companion_r4_entries": 16, "component_copies": component_counts,
            "served_entries": len(expected_served), "served_manifest_sha256": digest(stage / "SERVED-MANIFEST.sha256"),
            "inputs": {kind: {"filename": path.name, "sha256": INPUTS[kind][1]} for kind, path in inputs.items()},
            "preparation_commit": source["source_commit"], "companion_candidate": "R4",
            "companion_source_base": r4["base"],
            "components_byte_unchanged": True, "symlinks_required": False,
            "network_fetches": 0, "publication": False, "device_operations": 0,
        }
        (stage / RECEIPT_NAME).write_bytes((json.dumps(receipt, indent=2) + "\n").encode("utf-8"))
        if any(digest(path) != INPUTS[kind][1] for kind, path in inputs.items()):
            raise AssemblyError("An original input changed during assembly; output was not installed")
        ensure_fresh_output(output)
        if output.exists():
            output.rmdir()  # Atomic emptiness check; never removes a nonempty owner directory.
        stage.rename(output)
    return output, receipt


def serve(output, port):
    handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=str(output / "host"))
    server = http.server.ThreadingHTTPServer(("127.0.0.1", port), handler)
    url = "http://127.0.0.1:" + str(server.server_port) + "/workspace/#editor"
    print("Private local workspace: " + url, flush=True)
    print("Loopback only. Press Ctrl+C to stop; assembled files stay on disk.", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


def format_command(parts, windows=None):
    if windows is None:
        windows = os.name == "nt"
    if windows:
        # PowerShell: invoke a possibly quoted executable and keep every argument literal.
        return "& " + " ".join("'" + part.replace("'", "''") + "'" for part in parts)
    return " ".join(shlex.quote(part) for part in parts)


def restart_command(output, windows=None):
    return format_command([sys.executable, "-m", "http.server", "8000", "--bind", "127.0.0.1", "--directory", str(output / "host")], windows=windows)


def restart_shell():
    return "PowerShell" if os.name == "nt" else "your shell"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inputs-dir", help="Folder containing the three original archives")
    for kind in INPUTS:
        parser.add_argument("--" + kind, help="Explicit original " + kind + " archive path")
    parser.add_argument("--output", required=True, help="New or empty destination directory")
    parser.add_argument("--serve", nargs="?", const=8000, type=int, metavar="PORT", help="Serve on 127.0.0.1 only (default 8000; 0 selects a free port)")
    args = parser.parse_args(argv)
    try:
        if args.serve is not None and not 0 <= args.serve <= 65535:
            raise AssemblyError("Port must be between 0 and 65535")
        output, receipt = assemble(args)
        print("Verified private assembly: " + str(output), flush=True)
        print("Host manifest: " + receipt["own_manifest_sha256"], flush=True)
        if args.serve is not None:
            try:
                serve(output, args.serve)
            except OSError as error:
                print("Assembly is verified and saved, but the local server could not start: " + str(error), file=sys.stderr)
                print("Restart in " + restart_shell() + " with a free port: " + restart_command(output), file=sys.stderr)
                return 2
        else:
            print("Serve later in " + restart_shell() + ": " + restart_command(output), flush=True)
        return 0
    except (AssemblyError, OSError, ValueError, KeyError, tarfile.TarError, zipfile.BadZipFile) as error:
        print("Assembly stopped: " + str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
