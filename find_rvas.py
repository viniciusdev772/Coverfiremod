#!/usr/bin/env python3
"""Find all needed RVAs. Format: RVA is on the line BEFORE the method."""
INPUT = "/tmp/dump.cs"
OUTPUT = "/mnt/c/Users/Usuario/Documents/Coverfiremod/all_rvas.txt"

SEARCHES = {
    # PlayerControl
    "PC.NeedReloading": ("PlayerControl : MonoBehaviour", "NeedReloading()"),
    "PC.ActionReload": ("PlayerControl : MonoBehaviour", "ActionReload()"),
    "PC.Modify_Dispersion": ("PlayerControl : MonoBehaviour", "Modify_Dispersion("),
    "PC.Modify_VelocidadDisparo": ("PlayerControl : MonoBehaviour", "Modify_VelocidadDisparo("),
    "PC.Modify_Capacidad": ("PlayerControl : MonoBehaviour", "Modify_Capacidad("),
    "PC.Modify_Life": ("PlayerControl : MonoBehaviour", "Modify_Life("),
    "PC.CanShoot": ("PlayerControl : MonoBehaviour", "CanShoot()"),
    "PC.disparar": ("PlayerControl : MonoBehaviour", "void disparar()"),
    "PC.chargeWeapon": ("PlayerControl : MonoBehaviour", "void chargeWeapon()"),
    "PC.setWaitToFire": ("PlayerControl : MonoBehaviour", "void setWaitToFire()"),
    "PC.end_disparar": ("PlayerControl : MonoBehaviour", "void end_disparar()"),
    "PC.accion_recarga": ("PlayerControl : MonoBehaviour", "void accion_recarga()"),
    "PC.ChangeWeapon": ("PlayerControl : MonoBehaviour", "ChangeWeapon("),
    # EnemyController
    "EC.SetInitialHealth": ("EnemyController : MonoBehaviour", "SetInitialHealth("),
    "EC.SetCurrentHealth": ("EnemyController : MonoBehaviour", "SetCurrentHealth("),
    "EC.GetCurrentHealth": ("EnemyController : MonoBehaviour", "GetCurrentHealth()"),
    "EC.SetDamage": ("EnemyController : MonoBehaviour", "void SetDamage("),
    "EC.SetDamageProbability": ("EnemyController : MonoBehaviour", "SetDamageProbability("),
    "EC.GetDamage": ("EnemyController : MonoBehaviour", "float GetDamage()"),
    "EC.KillEnemy": ("EnemyController : MonoBehaviour", "KillEnemy()"),
    "EC.ApplyDamage": ("EnemyController : MonoBehaviour", "ApplyDamage(float damageShoot"),
    # SceneController
    "SC.ForzarSiguienteOleada": ("SceneController : MonoBehaviour", "ForzarSiguienteOleada"),
}

out = open(OUTPUT, "w")
in_class = None
prev_line = ""
found = {}

with open(INPUT, "r", errors="replace") as f:
    for line in f:
        s = line.strip()
        # Track class
        if "class " in s and "TypeDefIndex" in s:
            in_class = s
        if not in_class:
            prev_line = s
            continue
        # Check searches
        for key, (cls_match, method_match) in SEARCHES.items():
            if key in found:
                continue
            if cls_match not in in_class:
                continue
            if method_match in s:
                rva = "???"
                if "RVA: 0x" in prev_line:
                    start = prev_line.index("RVA: 0x") + 5
                    end = prev_line.index(" ", start)
                    rva = prev_line[start:end]
                found[key] = rva
                out.write(f"{key:40s} = {rva:12s}  // {s[:120]}\n")
        prev_line = s

out.write(f"\n\nTotal: {len(found)}/{len(SEARCHES)}\n")
missing = set(SEARCHES.keys()) - set(found.keys())
if missing:
    out.write(f"\nNao encontrados:\n")
    for m in sorted(missing):
        out.write(f"  {m}\n")
out.close()
print("OK")
