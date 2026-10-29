# SPDX-License-Identifier: GPL-3.0-only
"""Reject pi32v2 ROM references, with a bounded proof for scalar immediates.

The disassembler annotates integers as addresses even when they are fixed-point
constants. An annotation alone cannot distinguish them. Exempt a register
immediate only if every resolved path consumes it as a scalar, kills it, or
exports a value whose unsigned interval excludes ROM. Address uses, calls,
opaque consumers, parallel hazards and live cycles fail closed. This is a
deliberately small instruction model, not a general pointer analysis.
"""
from dataclasses import dataclass
import re

ROM_LO, ROM_HI, U32 = 0xFFC00000, 0xFFD00000, 0xFFFFFFFF
INSN = re.compile(r"^\s*([0-9a-fA-F]+):\s+(?:[0-9a-fA-F]{2}\s+)+(.*?)\s*$")
LABEL = re.compile(r"^[.$A-Za-z_][\w.$]*:$")
ANNOTATION = re.compile(r"\s+<[^<>]*: ([0-9a-fA-F]+) >")
REGISTER = r"(?:r(?:1[0-5]|[0-9])|sp)"
PAIR = rf"{REGISTER}(?:_{REGISTER})?"
REGS = re.compile(rf"(?<![A-Za-z0-9]){REGISTER}(?![A-Za-z0-9])")
NUMBER = r"(?:-?0[xX][0-9a-fA-F]+|-?\d+)"
NUMBERS = re.compile(rf"(?<![\w.]){NUMBER}(?![\w.])")
ASSIGN = re.compile(rf"^({PAIR})\s*(=|\+=|-=|\*=|/=|&=|\|=|\^=|<<=|>>=|>>>=)\s*(.+)$")
IMMEDIATE = re.compile(rf"^({REGISTER}) = ({NUMBER})$")
MEMORY = r"(?:[bhd])?\[[^\[\]]+\]"
LOAD = re.compile(rf"^({PAIR}) = ({MEMORY})(?: \([us]\))?$")
STORE = re.compile(rf"^({MEMORY}) = ({PAIR})$")
BRANCH = re.compile(r"^(?:ifs? \(.+\) )?goto -?\d+\s+<[^<>]*: ([0-9a-fA-F]+) >$")
CONDITION = re.compile(r"^ifs? \(.+\)(?: goto .+| \{)$")
ATOM = rf"(?:{REGISTER}|{NUMBER})"
SCALAR_CONDITION = re.compile(rf"^ifs? \({ATOM} (?:==|!=|>=|<=|>|<) {ATOM}\)(?: goto .+| \{{)$")
BINARY = re.compile(rf"^({ATOM}) (\+|-|\*|/|&|\||\^|<<|>>|>>>) ({ATOM})( \(u\))?$")


def _number(s):
    return int(s, 16 if "x" in s.lower() else 10) & U32


def _regs(s):
    return set(REGS.findall(s))


def _plain(s):
    return ANNOTATION.sub("", s).replace("#", "").strip()


@dataclass(frozen=True)
class Value:
    lo: int = 0
    hi: int = U32
    tainted: bool = False

    def may_be_rom(self):
        return self.lo < ROM_HI and self.hi >= ROM_LO


@dataclass
class Node:
    address: int | None
    body: str
    parallel: bool = False


def _functions(dis):
    """Keep brace markers: they encode control flow omitted from opcode rows."""
    functions, nodes, labelled = [], [], False
    for line in dis.splitlines():
        if LABEL.fullmatch(line.strip()):
            if nodes:
                functions.append(nodes)
            nodes = []
            labelled = True
        elif match := INSN.match(line):
            if not labelled and not nodes:
                nodes.append(Node(None, "<missing function label>"))
            node = Node(int(match[1], 16), match[2])
            if nodes and "#" in nodes[-1].body:
                node.parallel = nodes[-1].parallel = True
            nodes.append(node)
        elif line.strip() in ("}", "} else {"):
            nodes.append(Node(None, line.strip()))
        elif line.strip() and not line.startswith(("Disassembly", "Contents", "\n")):
            # Do not silently skip an unrecognised row inside a function.
            if nodes and not line.endswith("file format elf32-pi32v2"):
                nodes.append(Node(None, "<unparsed disassembly>"))
    if nodes:
        functions.append(nodes)
    return functions


def _cfg(nodes):
    addresses = {n.address: i for i, n in enumerate(nodes) if n.address is not None}
    if len(addresses) != sum(n.address is not None for n in nodes):
        raise ValueError("duplicate instruction address")
    edges = [[i + 1] for i in range(len(nodes))]
    stack = []
    for i, node in enumerate(nodes):
        body = node.body.strip()
        plain = _plain(body)
        if "unknown instruction" in plain or "<unparsed" in plain or "<missing" in plain:
            raise ValueError("incomplete instruction context")
        if plain == "rts" or re.match(r"^\{pc[,}]", plain):
            edges[i] = []
        elif re.search(r"^(?:pc|rets)\b|\b(?:reti|rti)\b", plain):
            raise ValueError("unmodelled control register or return")
        elif match := BRANCH.fullmatch(body):
            target = addresses.get(int(match[1], 16), -1)
            edges[i] = [target] + ([i + 1] if body.startswith("if") else [])
        elif CONDITION.fullmatch(plain) and plain.endswith("{"):
            stack.append([i, None])
        elif plain == "} else {":
            if not stack or stack[-1][1] is not None:
                raise ValueError("unmatched else")
            stack[-1][1] = i
        elif plain == "}":
            if not stack:
                raise ValueError("unmatched close")
            start, alternate = stack.pop()
            edges[start] = [start + 1, (alternate + 1 if alternate is not None else i + 1)]
            if alternate is not None:
                edges[alternate] = [i + 1]
        elif plain.startswith(("goto", "if", "jump", "rti")):
            # The forward proof rejects a reached unresolved edge.
            edges[i] = [-1]
    if stack:
        raise ValueError("unclosed condition")
    if any(not 0 <= target < len(nodes) for targets in edges for target in targets):
        raise ValueError("unresolved edge or truncated function")
    predecessors = [[] for _ in nodes]
    for i, targets in enumerate(edges):
        for target in targets:
            if 0 <= target < len(nodes):
                predecessors[target].append(i)
    return edges, predecessors


def _constant_before(nodes, predecessors, origin, register):
    """A reaching constant must agree on every incoming path, including joins."""
    budget = [256]

    def before(index, visiting):
        budget[0] -= 1
        if budget[0] < 0 or index in visiting or not predecessors[index]:
            return None
        values = []
        for prior in predecessors[index]:
            node = nodes[prior]
            body = _plain(node.body)
            assignment = ASSIGN.fullmatch(body)
            if (node.parallel or "call" in body or "<unparsed" in body or "unknown instruction" in body
                    or "++" in body or "--" in body):
                return None
            if assignment:
                _, operator, rhs = assignment.groups()
                if not (LOAD.fullmatch(body) or re.fullmatch(ATOM, rhs)
                        or (operator == "=" and BINARY.fullmatch(rhs))):
                    return None
                if register in _regs(assignment[1]):
                    immediate = IMMEDIATE.fullmatch(body)
                    values.append(_number(immediate[2]) if immediate else None)
                    continue
            elif not (STORE.fullmatch(body) or SCALAR_CONDITION.fullmatch(body)
                      or body.startswith("goto ") or body in ("}", "} else {")):
                return None
            values.append(before(prior, visiting | {index}))
        return values[0] if values and all(v == values[0] for v in values) else None

    return before(origin, set())


def _atom(s, values):
    return values[s] if s in values else Value(_number(s), _number(s))


def _arithmetic(a, op, b, unsigned=False):
    tainted = a.tainted or b.tainted
    lo, hi = 0, U32
    if op == "+":
        lo, hi = a.lo + b.lo, a.hi + b.hi
    elif op == "-":
        lo, hi = a.lo - b.hi, a.hi - b.lo
    elif op == "*":
        lo, hi = a.lo * b.lo, a.hi * b.hi
    elif op == "/" and unsigned and b.lo > 0:
        lo, hi = a.lo // b.hi, a.hi // b.lo
    elif op == "&":
        lo, hi = 0, min(a.hi, b.hi)
    elif op in ("|", "^") and a.lo == a.hi and b.lo == b.hi:
        lo = hi = (a.lo | b.lo) if op == "|" else (a.lo ^ b.lo)
    elif op == "<<" and b.lo == b.hi and b.hi < 32:
        lo, hi = a.lo << b.lo, a.hi << b.lo
    if lo == hi:
        lo = hi = lo & U32
    elif lo < 0 or hi > U32:
        lo, hi = 0, U32  # A wrapping interval is not a scalar escape proof.
    return Value(lo, hi, tainted)


def _scalar_proof(nodes, origin):
    """Explore every path until the candidate dies; bounded ambiguity rejects."""
    try:
        edges, predecessors = _cfg(nodes)
    except ValueError:
        return False
    values = {f"r{i}": Value() for i in range(16)} | {"sp": Value()}
    for register in values:
        constant = _constant_before(nodes, predecessors, origin, register)
        if constant is not None:
            values[register] = Value(constant, constant)
    pending = [(origin, values, False, frozenset())]
    budget = 1024
    while pending:
        i, values, carry, visited = pending.pop()
        budget -= 1
        if budget < 0 or i in visited or not 0 <= i < len(nodes):
            return False
        node, values = nodes[i], values.copy()
        body = _plain(node.body)
        tainted = {r for r, v in values.items() if v.tainted}
        touched = _regs(body)
        if node.parallel and (i == origin or touched & tainted):
            return False
        # Even a bounded scalar must not become an effective address. Keeping
        # this restriction also rejects pointer spills and indirect calls.
        if any(_regs(address) & tainted for address in re.findall(r"\[([^]]+)\]", body)):
            return False
        if re.search(r"\bcall\b", body):
            return False
        if carry and re.search(r"(?<![\w])!?c(?![\w])", body):
            return False
        if body == "rts":
            if any(v.tainted and v.may_be_rom() for v in values.values()):
                return False
            continue
        if SCALAR_CONDITION.fullmatch(body):
            # Explicit compare-and-branch consumes register scalars. It does
            # not export a pointer; both successors are checked below.
            carry = carry or bool(touched & tainted)
        elif body in ("}", "} else {") or (body.startswith("goto ") and BRANCH.fullmatch(node.body.strip())):
            pass
        elif match := STORE.fullmatch(body):
            if any(values[r].tainted and values[r].may_be_rom() for r in _regs(match[2])):
                return False
            if "++" in match[1] or "--" in match[1]:
                return False
        elif match := LOAD.fullmatch(body):
            if "++" in match[2] or "--" in match[2]:
                return False
            for register in _regs(match[1]):
                values[register] = Value()
        elif match := ASSIGN.fullmatch(body):
            dest, op, rhs = match.groups()
            if "_" in dest:
                if touched & tainted:
                    return False
                for register in _regs(dest):
                    values[register] = Value()
            else:
                binary = BINARY.fullmatch(rhs) if op == "=" else None
                if op == "=" and re.fullmatch(ATOM, rhs):
                    result = _atom(rhs, values)
                elif binary:
                    left, operation, right, unsigned = binary.groups()
                    result = _arithmetic(_atom(left, values), operation, _atom(right, values), bool(unsigned))
                    # The explicit carry chain in pi32v2 uses +c / -!c. A
                    # later untainted add/subtract overwrites that dependency.
                    carry = result.tainted if operation in ("+", "-") and not node.parallel else carry or result.tainted
                elif op != "=" and re.fullmatch(ATOM, rhs):
                    result = _arithmetic(values[dest], op[:-1], _atom(rhs, values))
                    carry = result.tainted if op in ("+=", "-=") and not node.parallel else carry or result.tainted
                else:
                    return False
                if i == origin:
                    result = Value(result.lo, result.hi, True)
                if dest == "sp" and result.tainted:
                    return False
                values[dest] = result
        else:
            return False
        if not any(v.tainted for v in values.values()) and not carry:
            continue
        if not edges[i]:  # Only the explicitly checked rts exit is accepted.
            return False
        for target in edges[i]:
            pending.append((target, values, carry, visited | {i}))
    return True


def references(dis):
    """Return errors in the build guard's existing message format."""
    errors = []
    for nodes in _functions(dis):
        for i, node in enumerate(nodes):
            if node.address is None:
                continue
            body = _plain(node.body)
            if "[" in body:
                for match in ANNOTATION.finditer(node.body):
                    target = int(match[1], 16) & U32
                    if ROM_LO <= target < ROM_HI:
                        errors.append(f"reference to ROM address {target:#010x}")
            # Branch annotations are actual targets; annotations on numeric
            # constants can differ in bit zero and are not the operand value.
            if re.search(r"\b(?:call|goto)\b", body):
                targets = [int(m[1], 16) & U32 for m in ANNOTATION.finditer(node.body)]
                targets += [_number(m[0]) for m in NUMBERS.finditer(body)]
                for value in targets:
                    if ROM_LO <= value < ROM_HI:
                        errors.append(f"reference to ROM address {value:#010x}")
                continue
            immediates = [_number(m[0]) for m in NUMBERS.finditer(body)]
            for value in immediates:
                if not ROM_LO <= value < ROM_HI:
                    continue
                assignment = IMMEDIATE.fullmatch(body)
                if not assignment or not _scalar_proof(nodes, i):
                    errors.append(f"reference to ROM address {value:#010x}")
    return errors
