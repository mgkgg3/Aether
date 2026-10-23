"""Compare two generated offsets headers, key by key.

The headers are a flat list of `namespace A { ... }` blocks with
`inline constexpr uintptr_t Name = 0x123;` inside.  Values are compared by the
full `A::B` path, which is stable even when a file reopens a namespace in a
second block (this tree does that at the bottom).

Usage: python offsets_diff.py <authority.h> <target.h>
"""

import re
import sys

NAMESPACE = re.compile(r"^\s*namespace\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\{")
VALUE = re.compile(
    r"^\s*inline\s+constexpr\s+(?:std::)?u?int(?:ptr|32|64)_t\s+(\w+)\s*=\s*([0-9a-fA-Fx]+)\s*;"
)


def parse(path):
    out = {}
    stack = []
    with open(path, "r", encoding="utf-8-sig", errors="replace") as fh:
        for line in fh:
            line = line.split("//", 1)[0]
            m = NAMESPACE.match(line)
            if m:
                stack.append(m.group(1))
                continue
            if line.strip().startswith("}"):
                if stack:
                    stack.pop()
                continue
            m = VALUE.match(line)
            if not m:
                continue
            name, raw = m.group(1), m.group(2)
            try:
                value = int(raw, 0)
            except ValueError:
                continue
            key = "::".join(stack + [name])
            out.setdefault(key, []).append(value)
    return out


def main():
    authority = parse(sys.argv[1])
    target = parse(sys.argv[2])

    differ, missing = [], []
    for key, values in sorted(target.items()):
        if key not in authority:
            continue
        # Repeated names inside one namespace mean the dump itself is
        # ambiguous; compare against the last value the authority gives.
        want = authority[key][-1]
        have = values[-1]
        if want == 0:
            continue
        if want != have:
            differ.append((key, have, want))
    for key in sorted(authority):
        if key not in target and authority[key][-1] != 0:
            missing.append((key, authority[key][-1]))

    print(f"differing: {len(differ)}")
    for key, have, want in differ:
        print(f"  {key}: have 0x{have:X}, dump 0x{want:X}")
    print(f"only in dump: {len(missing)}")
    for key, want in missing:
        print(f"  {key}: 0x{want:X}")


if __name__ == "__main__":
    main()
