# SPDX-License-Identifier: GPL-3.0-only
"""Original TouchOSC layout source and deterministic XML/.tosc writer.

Authoring does not run this tool. The Orchestrator builds and validates its
outputs on the box, then opens the layout in TouchOSC for runtime acceptance.
"""
import argparse
from pathlib import Path
import uuid
from xml.dom import minidom
import zlib


def make_xml(directory):
    directory = Path(directory)
    scripts = {name: (directory / name).read_text(encoding="utf-8") for name in ("controller.lua", "button.lua")}
    document = minidom.Document()
    lexml = document.createElement("lexml")
    lexml.setAttribute("version", "5")
    document.appendChild(lexml)

    def element(parent, name, text=None):
        result = document.createElement(name)
        parent.appendChild(result)
        if text is not None:
            result.appendChild(document.createTextNode(str(text)))
        return result

    def cdata(parent, text):
        # Lua source is trusted repository input; reject rather than corrupt
        # a future script that cannot be represented by one CDATA section.
        if "]]>" in text:
            raise ValueError("Script/text contains a CDATA terminator")
        parent.appendChild(document.createCDATASection(text))

    def property_node(properties, name, kind, value):
        prop = element(properties, "property")
        prop.setAttribute("type", kind)
        cdata(element(prop, "key"), name)
        target = element(prop, "value")
        if kind == "s":
            cdata(target, value)
        elif kind in ("r", "c"):
            for key, part in zip("xywh" if kind == "r" else "rgba", value):
                element(target, key, part)
        else:
            target.appendChild(document.createTextNode(str(value)))

    def value_node(values, name, default):
        val = element(values, "value")
        cdata(element(val, "key"), name)
        element(val, "locked", 0)
        element(val, "lockedDefaultCurrent", 0)
        cdata(element(val, "default"), default)
        element(val, "defaultPull", 0)

    def control(parent, name, kind, frame, text="", script=None):
        node = element(parent, "node")
        node.setAttribute("ID", str(uuid.uuid5(uuid.NAMESPACE_URL, "https://github.com/gobbajogger/X0X37-for-Loopy-Pro/touchosc/" + name)))
        node.setAttribute("type", kind)
        props = {
            "background": ("b", 0 if kind == "LABEL" else 1),
            "color": ("c", (0.16, 0.25, 0.30, 1)),
            "cornerRadius": ("f", 1),
            "frame": ("r", frame),
            "grabFocus": ("b", 1 if kind == "BUTTON" else 0),
            "interactive": ("b", 1 if kind == "BUTTON" else 0),
            "locked": ("b", 0),
            "name": ("s", name),
            "orientation": ("i", 0),
            "outline": ("b", 0 if kind == "LABEL" else 1),
            "outlineStyle": ("i", 1),
            "pointerPriority": ("i", 0),
            "shape": ("i", 1),
            "visible": ("b", 1),
        }
        if kind == "BUTTON":
            props.update({"buttonType": ("i", 0), "press": ("b", 1), "release": ("b", 1), "valuePosition": ("b", 0)})
        if kind == "LABEL":
            props.update({"font": ("i", 0), "textAlignH": ("i", 2), "textAlignV": ("i", 2), "textClip": ("b", 1), "textColor": ("c", (0.93, 0.97, 0.96, 1)), "textLength": ("i", 0), "textSize": ("i", 15)})
        if script:
            props["script"] = ("s", scripts[script])
        properties = element(node, "properties")
        for key in sorted(props):
            property_node(properties, key, *props[key])
        values = element(node, "values")
        if kind == "BUTTON":
            value_node(values, "x", "0")
        if kind == "LABEL":
            value_node(values, "text", text)
        value_node(values, "touch", "false")
        return node

    root = control(lexml, "dnbox_companion", "GROUP", (0, 0, 480, 800), script="controller.lua")
    children = element(root, "children")
    control(children, "title", "LABEL", (16, 12, 448, 38), "DNBOX / MIDI PERFORMANCE")
    control(children, "route", "LABEL", (16, 54, 448, 38), "DISARMED / MIDI CH 2 / connection 1")
    control(children, "feedback", "LABEL", (16, 96, 448, 38), "No incoming MIDI observed")
    control(children, "instructions", "LABEL", (16, 138, 448, 38), "Confirm route and GLO MIDI channel, then ARM")

    def button(name, label, frame):
        control(children, name, "BUTTON", frame, script="button.lua")
        # Labels do not receive pointer input and do not have MIDI bindings.
        control(children, name + "_label", "LABEL", frame, label)

    button("channel_down", "CH -", (16, 188, 104, 52))
    button("channel_up", "CH +", (128, 188, 104, 52))
    button("arm", "ARM", (240, 188, 104, 52))
    button("disarm", "RELEASE", (352, 188, 112, 52))
    labels = ("C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4")
    for index, label in enumerate(labels):
        button("note_" + str(60 + index), label, (16 + index % 4 * 112, 268 + index // 4 * 100, 104, 88))
    control(children, "drums_title", "LABEL", (16, 576, 448, 36), "GM hits / choose the kit's MIDI channel first")
    for index, (note, label) in enumerate(((36, "Kick 36"), (38, "Snare 38"), (42, "CHat 42"), (46, "OHat 46"))):
        button("note_" + str(note), label, (16 + index * 112, 624, 104, 88))
    control(children, "limits", "LABEL", (16, 728, 448, 52), "10-second hold limit / no pattern or sound sync")
    return document.toxml(encoding="UTF-8")


def make_tosc(directory):
    return zlib.compress(make_xml(directory), level=9)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True, help="New .tosc or .xml output; existing files are never replaced")
    args = parser.parse_args()
    if args.output.suffix not in (".tosc", ".xml"):
        parser.error("output must end in .tosc or .xml")
    here = Path(__file__).resolve().parent
    data = make_xml(here) if args.output.suffix == ".xml" else make_tosc(here)
    with args.output.open("xb") as stream:
        stream.write(data)


if __name__ == "__main__":
    main()
