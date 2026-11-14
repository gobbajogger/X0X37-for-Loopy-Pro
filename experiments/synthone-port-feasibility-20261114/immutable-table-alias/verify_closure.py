"""Reject substituted graph/DSP dependencies before compiling this experiment."""
import hashlib
import json
from pathlib import Path

GRAPH_COMMIT = "a0c8698f724e28ec6275307525933c195817b3c5"
GRAPH_TREE = "e58592f678c804798aa2250da84f815520731ca5"
PINS = {
    "shared_graph.cpp": "970a5aadafe8eb17043e6acf7afe02821e95dc6b6e554182f099ae1da0f910f3",
    "shared_graph.hpp": "137576968b44d5508f919ad54a49a717a3bcec0482e25cde64bb3c7a26231bfc",
    "DEPENDENCIES.json": "00aa7392ffc13b59deef1a05315cc86cedd751522c54817744319676effc4500",
}


def verify(graph_root):
    root = Path(graph_root).resolve(strict=True)
    for name, expected in PINS.items():
        if hashlib.sha256((root / name).read_bytes()).hexdigest() != expected:
            raise ValueError("graph pin mismatch: " + name)
    manifest = json.loads((root / "DEPENDENCIES.json").read_text())
    for row in manifest["dependencies"]:
        path = (root / row["path"]).resolve(strict=True)
        if root.parent not in path.parents:
            raise ValueError("dependency escapes experiment: " + row["path"])
        data = path.read_bytes()
        blob = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        if (len(data), hashlib.sha256(data).hexdigest(), blob) != (
                row["bytes"], row["sha256"], row["git_blob_sha"]):
            raise ValueError("dependency pin mismatch: " + row["path"])
    return root


def stage(graph_root, destination):
    """Copy only authenticated compiler inputs into a newly created tree.

    Verifying intended files in-place is insufficient: an unlisted header in
    an earlier include directory can change both comparison graphs identically.
    Preserve the verified relative layout, exclude every unlisted file, then
    reverify the copied closure before the compiler can consume it.
    """
    root = verify(graph_root)
    destination = Path(destination)
    destination.mkdir(parents=False, exist_ok=False)
    staged_graph = destination / "shared-graph"
    staged_graph.mkdir()
    paths = list(PINS)
    paths += [row["path"] for row in json.loads((root / "DEPENDENCIES.json").read_text())["dependencies"]]
    for relative in paths:
        output = staged_graph / relative
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes((root / relative).read_bytes())
    verify(staged_graph)
    return staged_graph
