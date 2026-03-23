#!/usr/bin/env python3
"""Verify all offsets and RVAs in Main.cpp against dump.cs."""
DUMP = "/tmp/dump.cs"
OUT = "/mnt/c/Users/Usuario/Documents/Coverfiremod/verify.txt"

# What Main.cpp uses — offset -> (class, field_name)
OFFSETS_TO_CHECK = {
    # EnemyController offsets
    ("EnemyController", "isActivo", 0x104, "bool"),
    ("EnemyController", "isDead", 0x106, "bool"),
    ("EnemyController", "isAware", 0x108, "bool"),
    ("EnemyController", "isShooting", 0x114, "bool"),
    ("EnemyController", "isLaunchGrenade", 0x116, "bool"),
    ("EnemyController", "Speed", 0x78, "float"),
    ("EnemyController", "speed_interpoints", 0x80, "float"),
    ("EnemyController", "speed_onalarm", 0x84, "float"),
    ("EnemyController", "SpeedBetweenCovers", 0xAC, "float"),
    ("EnemyController", "potencia_damageToPlayer", 0x220, "float"),
    ("EnemyController", "probabilidad_damageToPlayer", 0x224, "float"),
    ("EnemyController", "damage", 0x228, "float"),
    ("EnemyController", "isBoss", 0x46, "bool"),
    ("EnemyController", "IsSniper", 0x11E, "bool"),
    ("EnemyController", "IsBazooka", 0x11F, "bool"),
    ("EnemyController", "IsShotgun", 0x120, "bool"),
    ("EnemyController", "isZombie", 0xCC, "bool"),
    ("EnemyController", "isFlying", 0x122, "bool"),
    ("EnemyController", "HasShield", 0x324, "bool"),
    ("EnemyController", "myplayercontrol", 0x1C, "PlayerControl"),
    ("EnemyController", "tr", 0x12C, "Transform"),
    ("EnemyController", "Head", 0x238, "Transform"),
    ("EnemyController", "Hips", 0x230, "Transform"),
    ("EnemyController", "Spine2", 0x234, "Transform"),
    # PlayerControl offsets
    ("PlayerControl", "initial_life", 0x6C, "float"),
    ("PlayerControl", "actual_life", 0x70, "float"),
    ("PlayerControl", "multiplier_speed_transform", 0x154, "float"),
    ("PlayerControl", "myscenecontroller", 0x24, "SceneController"),
}

# RVAs to check - rva_hex -> (class, method_substr)
RVAS_TO_CHECK = {
    0xBEEE58: ("PlayerControl", "ApplyDamage"),
    0xBF2B6C: ("PlayerControl", "ApplyDirectDamage"),
    0xBD2B20: ("PlayerControl", "SendShootCommand"),
    0xBD0ECC: ("PlayerControl", "ActionShoot"),
    0xB115A8: ("EnemyController", "ApplyDamage"),
    0xB14590: ("EnemyController", "KillEnemy"),
    0xBD2A98: ("PlayerControl", "NeedReloading"),
    0xAFE838: ("EnemyController", "SetCurrentHealth"),
    0xAFE840: ("EnemyController", "SetDamage"),
    0xAFE848: ("EnemyController", "SetDamageProbability"),
}

out = open(OUT, "w")

# Pass 1: Check offsets
out.write("=== OFFSET VERIFICATION ===\n\n")
found_offsets = {}
with open(DUMP, "r", errors="replace") as f:
    cur_class = None
    for line in f:
        s = line.strip()
        if "class " in s and "TypeDefIndex" in s:
            if "class EnemyController " in s:
                cur_class = "EnemyController"
            elif "class PlayerControl : " in s:
                cur_class = "PlayerControl"
            else:
                cur_class = None
        if not cur_class:
            continue
        for cls, field, offset, typ in OFFSETS_TO_CHECK:
            if cls != cur_class:
                continue
            hex_off = f"0x{offset:X}"
            if field in s and hex_off in s:
                key = (cls, field, offset)
                if key not in found_offsets:
                    found_offsets[key] = s.strip()

for cls, field, offset, typ in sorted(OFFSETS_TO_CHECK):
    key = (cls, field, offset)
    hex_off = f"0x{offset:X}"
    if key in found_offsets:
        out.write(f"  OK   {cls}.{field} @ {hex_off} => {found_offsets[key][:100]}\n")
    else:
        out.write(f"  FAIL {cls}.{field} @ {hex_off} NOT FOUND!\n")

# Pass 2: Check RVAs
out.write("\n\n=== RVA VERIFICATION ===\n\n")
found_rvas = {}
with open(DUMP, "r", errors="replace") as f:
    prev = ""
    cur_class = None
    for line in f:
        s = line.strip()
        if "class " in s and "TypeDefIndex" in s:
            if "class EnemyController " in s:
                cur_class = "EnemyController"
            elif "class PlayerControl : " in s:
                cur_class = "PlayerControl"
            else:
                cur_class = None
        for rva, (cls, method) in RVAS_TO_CHECK.items():
            hex_rva = f"0x{rva:X}"
            if hex_rva in prev and method in s:
                found_rvas[rva] = (prev.strip(), s.strip())
        prev = s

for rva, (cls, method) in sorted(RVAS_TO_CHECK.items()):
    hex_rva = f"0x{rva:X}"
    if rva in found_rvas:
        out.write(f"  OK   {cls}.{method} @ {hex_rva} => {found_rvas[rva][1][:100]}\n")
    else:
        out.write(f"  FAIL {cls}.{method} @ {hex_rva} NOT FOUND!\n")

ok_offsets = sum(1 for k in OFFSETS_TO_CHECK if (k[0],k[1],k[2]) in found_offsets)
ok_rvas = sum(1 for r in RVAS_TO_CHECK if r in found_rvas)
out.write(f"\n\nOffsets: {ok_offsets}/{len(OFFSETS_TO_CHECK)}  RVAs: {ok_rvas}/{len(RVAS_TO_CHECK)}\n")
out.close()
print("OK")
