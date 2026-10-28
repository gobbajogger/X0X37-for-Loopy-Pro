#!/usr/bin/env python3
"""Render existing-role bootstrap text. No scheduler or network operations."""
import argparse
import json
from pathlib import Path


def render_all(directory=None):
    directory = Path(directory) if directory else Path(__file__).resolve().parent
    registry = json.loads((directory / "roles.json").read_text(encoding="utf-8"))
    template = (directory / "bootstrap.txt").read_text(encoding="utf-8")
    output = {}
    role_ids = set()
    for role in registry["roles"]:
        role_id, automation_id = role["role_id"], role["automation_id"]
        if role_id in role_ids or automation_id in output:
            raise ValueError("Duplicate role or automation identity")
        role_ids.add(role_id)
        text = template.replace("{{role_id}}", role_id).replace("{{automation_id}}", automation_id)
        if "{{" in text or "}}" in text:
            raise ValueError("Unresolved bootstrap placeholder")
        output[automation_id] = {"role_id": role_id, "prompt": text}
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--role", help="Print one role's prompt; omit for JSON keyed by automation ID")
    args = parser.parse_args()
    rendered = render_all()
    if args.role:
        matches = [entry["prompt"] for entry in rendered.values() if entry["role_id"] == args.role]
        if len(matches) != 1:
            parser.error("Unknown role ID")
        print(matches[0], end="")
    else:
        print(json.dumps(rendered, ensure_ascii=False, indent=2))
