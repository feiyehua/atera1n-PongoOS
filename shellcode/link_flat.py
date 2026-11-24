#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2026 feiyehua

import struct, sys

SRC = sys.argv[1] if len(sys.argv) > 1 else "sep.o"
OUT = sys.argv[2] if len(sys.argv) > 2 else "sep.bin"

SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_REL = 9
SHF_ALLOC = 0x2

d = bytearray(open(SRC, "rb").read())

e_shoff = struct.unpack_from("<I", d, 0x20)[0]
shnum, shstrndx = struct.unpack_from("<HH", d, 0x30)
raw = [struct.unpack_from("<IIIIIIIIII", d, e_shoff + i * 40) for i in range(shnum)]
shstr = raw[shstrndx][4]


def sname(off):
    end = d.index(b"\0", shstr + off)
    return d[shstr + off:end].decode()


secs = {}
for i, s in enumerate(raw):
    secs[i] = dict(
        name=sname(s[0]), type=s[1], flags=s[2], addr=s[3],
        off=s[4], size=s[5], link=s[6], info=s[7], align=s[8], ent=s[9],
    )

symtab_i = next(i for i, s in secs.items() if s["type"] == SHT_SYMTAB)
st = secs[symtab_i]
strtab = secs[st["link"]]

syms = []
for i in range(0, st["size"], 16):
    name, value, size, info, other, shndx = struct.unpack_from("<IIIBBH", d, st["off"] + i)
    end = d.index(b"\0", strtab["off"] + name)
    syms.append(dict(
        name=d[strtab["off"] + name:end].decode(), value=value, size=size,
        info=info, shndx=shndx,
    ))

# Mapping symbols ($a/$t/$d) tell us ARM vs Thumb state per offset.
maps = {}
for s in syms:
    if s["name"] in ("$a", "$t", "$d") and s["shndx"] != 0:
        maps.setdefault(s["shndx"], []).append((s["value"], s["name"][1]))
for v in maps.values():
    v.sort()


def is_thumb(sym):
    if sym["value"] & 1:
        return True
    if sym["shndx"] == 0:
        return False
    prev = [state for off, state in maps.get(sym["shndx"], []) if off <= (sym["value"] & ~1)]
    return bool(prev) and prev[-1] == "t"


# Code = .text plus any other PROGBITS section defining a function symbol.
# (Some objects smuggle code into oddly named sections like .note.GNU-stack.)
func_secs = {s["shndx"] for s in syms if (s["info"] & 0xF) == 2 and s["shndx"] not in (0, 0xFFFF)}
text_i = next((i for i, s in secs.items() if s["name"] == ".text"), None)
code = ([text_i] if text_i is not None else []) + [
    i for i, s in secs.items()
    if s["type"] == SHT_PROGBITS and s["size"] and i != text_i
    and i in func_secs and not (s["flags"] & SHF_ALLOC)
]
if not code:
    sys.exit("no code sections found")

# Lay code sections out from base 0, 4-byte aligned.
vma, blob = 0, bytearray()
for i in code:
    s = secs[i]
    align = max(1, s["align"])
    vma = (vma + align - 1) & ~(align - 1)
    s["vma"], s["blob_off"] = vma, len(blob)
    chunk = bytearray(d[s["off"]:s["off"] + s["size"]])
    blob += chunk
    vma += len(chunk)
    pad = (-len(blob)) & 3
    blob += b"\0" * pad
    vma += pad


def sym_addr(sym):
    if sym["shndx"] in (0, 0xFFFF):
        return sym["value"] & ~1
    return secs[sym["shndx"]]["vma"] + (sym["value"] & ~1)


for _, s in secs.items():
    if s["type"] != SHT_REL:
        continue
    tgt = secs[s["info"]]
    if "blob_off" not in tgt:
        continue
    for off in range(0, s["size"], 8):
        r_offset, r_info = struct.unpack_from("<II", d, s["off"] + off)
        r_type = r_info & 0xFF
        sym = syms[r_info >> 8]
        P = tgt["vma"] + r_offset
        S = sym_addr(sym)
        pos = tgt["blob_off"] + r_offset
        insn = struct.unpack_from("<I", blob, pos)[0]

        if r_type == 28:  # R_ARM_CALL
            if (insn >> 28) == 0xF:
                sys.exit("thumb-caller BL not supported (%s)" % sym["name"])
            imm = S - (P + 8)
            if is_thumb(sym):
                new = 0xFA000000 | (((imm >> 1) & 1) << 24) | ((imm >> 2) & 0xFFFFFF)
                kind = "BLX(ARM->Thumb)"
            else:
                if imm & 3:
                    sys.exit("misaligned ARM branch to %s" % sym["name"])
                new = (insn & 0xFF000000) | ((imm >> 2) & 0xFFFFFF)
                kind = "BL(ARM->ARM)"
        else:
            sys.exit("unsupported reloc type %d for %s @%#x" % (r_type, sym["name"], r_offset))

        struct.pack_into("<I", blob, pos, new)
        print("%-24s @%#06x -> %#010x %s  0x%08x" % (sym["name"], P, S, kind, new))

open(OUT, "wb").write(blob)
print("wrote %s (%d bytes, base 0x0)" % (OUT, len(blob)))
