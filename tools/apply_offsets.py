"""Rewrite numeric literals in our offsets header from an authority dump.

Only the literal on the defining line changes: comments, ordering and the
hand-written tail of the file are left exactly as they are.  A key is matched by
its full `A::B` path, which is what offsets_diff.py compares, so a namespace that
appears in two blocks cannot pick up the wrong line.

Zeroes in the authority are skipped (the dumper's "unresolved" answer), and so
are the keys in SKIP below, with the reason written next to each.

Usage: python apply_offsets.py <authority.h> <target.h> [--dry]
"""

import re
import sys

NAMESPACE = re.compile(r"^\s*namespace\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\{")
VALUE = re.compile(
    r"^(?P<head>\s*inline\s+constexpr\s+(?:std::)?u?int(?:ptr|32|64)_t\s+(?P<name>\w+)\s*=\s*)(?P<value>(?:0[xX][0-9a-fA-F]+)|(?:\d+))(?P<tail>\s*;.*)$"
)

# Keys the dump answers with something that cannot be an offset in this object,
# or that no code here reads.  Fixing them to the dump value would be noise at
# best: 0xCCD2 is outside the object, and PlatformStatePointer looks like the
# dumper reading an enum as a pointer.
SKIP = {
    "Offsets::StatsItem::Value",
    "Offsets::Humanoid::PlatformStatePointer",
    "Offsets::BasePart::Color3",
    # The two dumpers disagree (0x309 in file (1).hpp, 0x1BE here).  Noclip and
    # the hitbox expander *write* this byte, so the wrong answer corrupts a
    # primitive; with no third source to break the tie it stays where it was.
    "Offsets::Primitive::Flags",
}


def parse_authority(path):
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
            try:
                value = int(m.group("value"), 0)
            except ValueError:
                continue
            out["::".join(stack + [m.group("name")])] = value
    return out


def main():
    authority = parse_authority(sys.argv[1])
    target_path = sys.argv[2]
    dry = "--dry" in sys.argv

    with open(target_path, "r", encoding="utf-8-sig", errors="replace", newline="") as fh:
        lines = fh.readlines()

    stack = []
    changed = []
    for index, line in enumerate(lines):
        code = line.split("//", 1)[0]
        m = NAMESPACE.match(code)
        if m:
            stack.append(m.group(1))
            continue
        if code.strip().startswith("}"):
            if stack:
                stack.pop()
            continue

        m = VALUE.match(code)
        if not m:
            continue

        key = "::".join(stack + [m.group("name")])
        want = authority.get(key)
        if not want or key in SKIP:
            continue
        have = int(m.group("value"), 0)
        if have == want:
            continue

        start = line.index(m.group("value"), 0)
        end = start + len(m.group("value"))
        lines[index] = line[:start] + f"0x{want:X}" + line[end:]
        changed.append((key, have, want))

    print(f"rewritten: {len(changed)}")
    for key, have, want in changed:
        print(f"  {key}: 0x{have:X} -> 0x{want:X}")

    if dry:
        print("dry run - nothing written")
        return

    with open(target_path, "w", encoding="utf-8-sig", newline="") as fh:
        fh.writelines(lines)
    print("written")


if __name__ == "__main__":
    main()
