#!/usr/bin/env python3
"""
exec_time_vs_size.py  — v2
Calcula el tiempo de ejecución restando "Program Started at" y "Execution completed at"
"""

import os, re
import pandas as pd
import matplotlib.pyplot as plt
from datetime import datetime

RESULTS_DIR = "results_insee"
PLOTS_DIR   = "plots_insee"
os.makedirs(PLOTS_DIR, exist_ok=True)

N_SERVERS = {
    "megafly_4":  128,  "megafly_8":  512,  "megafly_10": 800,
    "slimfly_3":   50,  "slimfly_5":  200,  "slimfly_7":  392,
}

DT_FMT = "%a %b %d %H:%M:%S %Y"  # Wed May 13 11:23:29 2026

rows = []
for fname in sorted(os.listdir(RESULTS_DIR)):
    if not fname.endswith(".log"):
        continue
    m = re.match(
        r"((?:megafly|slimfly)_\d+)_(static|adaptive)_(.+?)_load([\d.]+)_seed(\d+)\.log$",
        fname
    )
    if not m:
        continue

    topo_name = m.group(1)
    tpattern  = m.group(3)
    load      = float(m.group(4))
    seed      = int(m.group(5))
    family    = "MegaFly" if topo_name.startswith("megafly") else "SlimFly"

    with open(os.path.join(RESULTS_DIR, fname), errors="replace") as f:
        content = f.read()

    t_start = re.search(r"Program Started at:\s*(.+)", content)
    t_end   = re.search(r"Execution completed at:\s*(.+)", content)
    if not t_start or not t_end:
        continue

    try:
        dt_start = datetime.strptime(t_start.group(1).strip(), DT_FMT)
        dt_end   = datetime.strptime(t_end.group(1).strip(),   DT_FMT)
        exec_time = (dt_end - dt_start).total_seconds()
    except ValueError:
        continue

    rows.append(dict(
        topo_name = topo_name,
        family    = family,
        n_servers = N_SERVERS.get(topo_name),
        tpattern  = tpattern,
        load      = load,
        seed      = seed,
        exec_time = exec_time,
    ))

df = pd.DataFrame(rows)
if df.empty:
    print("No se encontraron tiempos en los logs.")
    exit(1)

print(df.groupby(["topo_name","family","n_servers"])["exec_time"]
        .agg(["mean","std","min","max","count"]).reset_index().to_string())

df_topo = (df.groupby(["family","topo_name","n_servers"])["exec_time"]
             .agg(mean="mean", std="std")
             .reset_index().sort_values("n_servers"))

df_load = (df.groupby(["family","topo_name","n_servers","load"])["exec_time"]
             .mean().reset_index().sort_values(["topo_name","load"]))

# ── Gráfica ───────────────────────────────────────────────────────────────────
COLORS  = {"MegaFly": "#4C72B0", "SlimFly": "#DD8452"}
MARKERS = {"MegaFly": "o",       "SlimFly": "s"}

fig, axes = plt.subplots(1, 2, figsize=(13, 5))
fig.suptitle("Tiempo de Ejecución vs Tamaño de Red — MegaFly vs SlimFly",
             fontsize=13, fontweight="bold")

# Subplot 1: media ± std
ax = axes[0]
for fam, grp in df_topo.groupby("family"):
    grp = grp.sort_values("n_servers")
    ax.errorbar(grp["n_servers"], grp["mean"], yerr=grp["std"],
                marker=MARKERS[fam], color=COLORS[fam], linewidth=2,
                markersize=8, capsize=5, label=fam, zorder=3)
    for _, row in grp.iterrows():
        k = row["topo_name"].split("_")[1]
        prefix = "k=" if fam == "MegaFly" else "Q="
        ax.annotate(f"{prefix}{k} ({int(row['n_servers'])} srv)",
                    xy=(row["n_servers"], row["mean"]),
                    xytext=(8, 4), textcoords="offset points",
                    fontsize=8, color=COLORS[fam])

ax.set_xlabel("Número de servidores")
ax.set_ylabel("Tiempo de ejecución (s)")
ax.set_title("Media ± std (todos los tráficos y loads)")
ax.legend(fontsize=10)
ax.grid(True, ls="--", alpha=0.4)

# Subplot 2: por load
ax = axes[1]
linestyles = {0.2: "-", 0.4: "--", 0.6: "-.", 0.8: ":"}
for fam, grp_f in df_load.groupby("family"):
    for load_val, grp_l in grp_f.groupby("load"):
        grp_l = grp_l.sort_values("n_servers")
        ax.plot(grp_l["n_servers"], grp_l["exec_time"],
                marker=MARKERS[fam], color=COLORS[fam],
                ls=linestyles.get(load_val, "-"), lw=1.5, ms=6, alpha=0.85,
                label=f"{fam} load={load_val}")

ax.set_xlabel("Número de servidores")
ax.set_ylabel("Tiempo de ejecución (s)")
ax.set_title("Desglosado por nivel de carga")
ax.legend(fontsize=7.5, ncol=2)
ax.grid(True, ls="--", alpha=0.4)

plt.tight_layout()
out = os.path.join(PLOTS_DIR, "exec_time_vs_size.png")
plt.savefig(out, dpi=150)
plt.close()
print(f"\n✓ Gráfica guardada en: {os.path.abspath(out)}")

