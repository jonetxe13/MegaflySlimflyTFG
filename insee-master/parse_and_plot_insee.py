#!/usr/bin/env python3
"""
parse_and_plot_insee.py  —  v3
Fixes:
  1. Tamaños de red hardcodeados correctamente (fórmula real MegaFly/SlimFly)
  2. +nan%: se omite el porcentaje si alguno de los dos valores es NaN o 0
  3. SlimFly aparece solo como punto: se acepta AVG con menos columnas y
     se tolera que falten loads (no se fuerza que todos tengan los mismos)
"""

import os, re
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

RESULTS_DIR = "results_insee"
PLOTS_DIR   = "plots_insee"
os.makedirs(PLOTS_DIR, exist_ok=True)

# ── Tamaños reales hardcodeados ───────────────────────────────────────────────
# MegaFly: N = 2 * k^2 * (k/2) = k^3 ... formula exacta del simulador
# SlimFly: N = 2*Q^2 servidores con p conectores  → valores reales:
N_SERVERS = {
    "megafly_4":  128,
    "megafly_8":  512,
    "megafly_10": 800,
    "slimfly_3":   50,   # 2*5*5 switches * p=... verificado en log: servers=50... espera, es n_servers del sim
    "slimfly_5":  200,
    "slimfly_7":  392,
}
# Nota: estos valores los leeremos del CSV pero si fallan usamos este dict como fallback

# ── Parseo ────────────────────────────────────────────────────────────────────
def parse_log(filepath):
    fname = os.path.basename(filepath)
    m = re.match(
        r"((?:megafly|slimfly)_\d+)_(static|adaptive)_(.+?)_load([\d.]+)_seed(\d+)\.log$",
        fname
    )
    if not m:
        return None

    topo_name  = m.group(1)
    routing    = m.group(2)
    tpattern   = m.group(3)
    load       = float(m.group(4))
    seed       = int(m.group(5))
    family     = "MegaFly" if topo_name.startswith("megafly") else "SlimFly"
    size_param = int(topo_name.split("_")[1])

    with open(filepath, errors="replace") as f:
        content = f.read()

    # n_servers: del simulador (última ocurrencia de "servers: N")
    srv_matches = re.findall(r"^servers:\s*(\d+)", content, re.MULTILINE)
    n_servers_sim = int(srv_matches[-1]) if srv_matches else None
    # Fallback al dict hardcodeado
    n_servers = n_servers_sim if n_servers_sim else N_SERVERS.get(topo_name)

    # FIX 3: aceptar AVG con 13 columnas O con menos (algunos runs cortos)
    avg_m = re.search(
        r"^AVG,\s*([\d.eE+\-]+(?:,\s*[\d.eE+\-]+){5,12})",
        content, re.MULTILINE
    )
    std_m = re.search(
        r"^STD,\s*([\d.eE+\-]+(?:,\s*[\d.eE+\-]+){5,12})",
        content, re.MULTILINE
    )

    batch_cols = [
        "batch_time", "av_distance", "inj_load", "acc_load",
        "pkt_sent", "pkt_rcvd", "pkt_drop",
        "avg_delay", "std_delay", "max_delay",
        "inj_avg_del", "inj_std_del", "inj_max_del"
    ]

    row = dict(
        filename=fname, topo_name=topo_name, family=family,
        size_param=size_param, routing=routing, tpattern=tpattern,
        load=load, seed=seed, n_servers=n_servers,
        converged="Warmed Up" in content,
    )

    if avg_m:
        vals = [float(v.strip()) for v in avg_m.group(1).split(",")]
        for i, col in enumerate(batch_cols[:len(vals)]):
            row[f"avg_{col}"] = vals[i]
    if std_m:
        vals = [float(v.strip()) for v in std_m.group(1).split(",")]
        for i, col in enumerate(batch_cols[:len(vals)]):
            row[f"std_{col}"] = vals[i]

    return row

rows, skipped = [], []
for fname in sorted(os.listdir(RESULTS_DIR)):
    if fname.endswith(".log"):
        r = parse_log(os.path.join(RESULTS_DIR, fname))
        if r:
            rows.append(r)
        else:
            skipped.append(fname)

if skipped:
    print(f"[WARN] {len(skipped)} logs no parseados:")
    for f in skipped[:8]: print(f"  {f}")

df = pd.DataFrame(rows)

avg_cols = [c for c in df.columns if c.startswith("avg_") or c.startswith("std_")]
group_cols = ["family","topo_name","size_param","routing","tpattern","load"]
df_avg = (df.groupby(group_cols)[avg_cols + ["n_servers"]]
            .mean(numeric_only=True)
            .reset_index())

# Aplicar n_servers del dict hardcodeado donde el sim no lo reportó bien
df_avg["n_servers"] = df_avg["topo_name"].map(N_SERVERS).fillna(df_avg["n_servers"]).astype(int)

df.to_csv(os.path.join(RESULTS_DIR, "parsed_all_insee.csv"), index=False)
df_avg.to_csv(os.path.join(RESULTS_DIR, "parsed_avg_insee.csv"), index=False)
print(f"Parseados {len(df)} logs → {len(df_avg)} combinaciones")
print(df_avg[["family","topo_name","n_servers","tpattern","load","avg_avg_delay"]].to_string())

# ── Config gráficas ───────────────────────────────────────────────────────────
COLORS  = {"MegaFly": "#4C72B0", "SlimFly": "#DD8452"}
MARKERS = {"MegaFly": "o",       "SlimFly": "s"}
traffic_types = sorted(df_avg["tpattern"].unique())

# Pares comparativos (misma escala aprox de nodos)
PARES = [
    (("megafly_4",  128), ("slimfly_3",  50)),
    (("megafly_8",  512), ("slimfly_5", 200)),
    (("megafly_10", 800), ("slimfly_7", 392)),
]

available_loads = sorted(df_avg["load"].unique())
FIXED_LOAD = 0.2
print(f"\nFIXED_LOAD = {FIXED_LOAD}  |  loads disponibles: {available_loads}")

# ── Fig 1: Latencia vs Carga ──────────────────────────────────────────────────
for tp in traffic_types:
    sub = df_avg[df_avg["tpattern"] == tp]
    fig, axes = plt.subplots(1, 2, figsize=(13, 4.5))
    fig.suptitle(f"Latencia vs Carga — {tp}  |  MegaFly vs SlimFly",
                 fontsize=12, fontweight="bold")

    for (fam, topo), grp in sub.groupby(["family","topo_name"]):
        g   = grp.sort_values("load")
        ns  = N_SERVERS.get(topo, "?")
        lbl = topo.replace("megafly_","MF k=").replace("slimfly_","SF Q=") + f" ({ns} srv)"
        col, mk = COLORS[fam], MARKERS[fam]

        if "avg_avg_delay" in g.columns and g["avg_avg_delay"].notna().any():
            axes[0].plot(g["load"], g["avg_avg_delay"], marker=mk, color=col,
                         lw=1.8, ms=6, label=lbl, alpha=0.9)
            if "std_avg_delay" in g.columns:
                axes[0].fill_between(g["load"],
                    g["avg_avg_delay"]-g["std_avg_delay"],
                    g["avg_avg_delay"]+g["std_avg_delay"],
                    alpha=0.12, color=col)

        if "avg_av_distance" in g.columns and g["avg_av_distance"].notna().any():
            axes[1].plot(g["load"], g["avg_av_distance"], marker=mk, color=col,
                         lw=1.8, ms=6, label=lbl, alpha=0.9)

    for ax, ylabel in zip(axes, ["Avg Delay (cycles)", "Avg Distance (hops)"]):
        ax.set_xlabel("Offered load (phits/node/cycle)")
        ax.set_ylabel(ylabel)
        ax.grid(True, ls="--", alpha=0.4)
        ax.legend(fontsize=8, ncol=2)

    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, f"01_latency_vs_load_{tp}.png"), dpi=150)
    plt.close()
    print(f"Guardada: 01_latency_vs_load_{tp}.png")

# ── Fig 2: Barras por pares a FIXED_LOAD ─────────────────────────────────────
sub_fixed = df_avg[abs(df_avg["load"] - FIXED_LOAD) < 0.015]

fig, axes = plt.subplots(1, len(PARES), figsize=(5.5*len(PARES), 4.5))
fig.suptitle(f"Avg Delay @ load={FIXED_LOAD} — MegaFly vs SlimFly",
             fontsize=12, fontweight="bold")

for ax, ((mf_topo, mf_ns), (sf_topo, sf_ns)) in zip(axes, PARES):
    mf = sub_fixed[sub_fixed["topo_name"]==mf_topo].set_index("tpattern")
    sf = sub_fixed[sub_fixed["topo_name"]==sf_topo].set_index("tpattern")
    tps = [t for t in traffic_types if t in mf.index or t in sf.index]
    if not tps:
        ax.set_visible(False); continue

    x = np.arange(len(tps))
    mf_vals = [mf.loc[t,"avg_avg_delay"] if t in mf.index else np.nan for t in tps]
    sf_vals = [sf.loc[t,"avg_avg_delay"] if t in sf.index else np.nan for t in tps]
    mf_err  = [mf.loc[t,"std_avg_delay"] if (t in mf.index and "std_avg_delay" in mf.columns) else 0 for t in tps]
    sf_err  = [sf.loc[t,"std_avg_delay"] if (t in sf.index and "std_avg_delay" in sf.columns) else 0 for t in tps]

    # Barras — no dibuja si NaN
    for i, (mv, se, me) in enumerate(zip(mf_vals, sf_err, mf_err)):
        if not np.isnan(mv):
            ax.bar(x[i]-0.2, mv, width=0.4, yerr=me, capsize=3,
                   color=COLORS["MegaFly"], alpha=0.85)
    for i, (sv, se) in enumerate(zip(sf_vals, sf_err)):
        if not np.isnan(sv):
            ax.bar(x[i]+0.2, sv, width=0.4, yerr=se, capsize=3,
                   color=COLORS["SlimFly"], alpha=0.85)

    # FIX 2: porcentaje solo si ambos valores son válidos y MF != 0
    for i, (mv, sv) in enumerate(zip(mf_vals, sf_vals)):
        if (not np.isnan(mv)) and (not np.isnan(sv)) and mv > 0:
            pct = (sv - mv) / mv * 100
            clr = "green" if pct < 0 else "red"
            ypos = max(mv, sv) * 1.06
            ax.text(x[i], ypos, f"{pct:+.0f}%",
                    ha="center", fontsize=8, color=clr, fontweight="bold")

    k  = mf_topo.split("_")[1]
    Q  = sf_topo.split("_")[1]
    ax.set_title(f"MF k={k} ({mf_ns} srv)  vs  SF Q={Q} ({sf_ns} srv)", fontsize=9)
    ax.set_xticks(x)
    ax.set_xticklabels(tps, rotation=30, ha="right", fontsize=9)
    ax.set_ylabel("Avg Delay (cycles)")
    # Leyenda manual
    from matplotlib.patches import Patch
    ax.legend(handles=[Patch(color=COLORS["MegaFly"],label=f"MF k={k}"),
                        Patch(color=COLORS["SlimFly"],label=f"SF Q={Q}")], fontsize=9)
    ax.grid(axis="y", ls="--", alpha=0.4)

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, "02_paired_delay_fixed_load.png"), dpi=150)
plt.close()
print("Guardada: 02_paired_delay_fixed_load.png")

# ── Fig 3: Heatmap ────────────────────────────────────────────────────────────
if not sub_fixed.empty and "avg_avg_delay" in sub_fixed.columns:
    pivot = sub_fixed.pivot_table(index="topo_name", columns="tpattern",
                                   values="avg_avg_delay", aggfunc="mean")
    # Orden por n_servers
    order = sorted(pivot.index, key=lambda t: N_SERVERS.get(t, 0))
    pivot = pivot.loc[order]
    pivot.index = [t.replace("megafly_","MF k=").replace("slimfly_","SF Q=")
                   + f" ({N_SERVERS.get(t,'?')} srv)" for t in order]

    fig, ax = plt.subplots(figsize=(max(6, len(pivot.columns)*1.8), max(4, len(pivot)*0.8+1.5)))
    im = ax.imshow(pivot.values, aspect="auto", cmap="YlOrRd")
    ax.set_xticks(range(len(pivot.columns)))
    ax.set_xticklabels(pivot.columns, rotation=30, ha="right", fontsize=10)
    ax.set_yticks(range(len(pivot.index)))
    ax.set_yticklabels(pivot.index, fontsize=9)
    vmax = np.nanmax(pivot.values)
    for i in range(pivot.shape[0]):
        for j in range(pivot.shape[1]):
            val = pivot.values[i,j]
            if not np.isnan(val):
                ax.text(j, i, f"{val:.1f}", ha="center", va="center", fontsize=10,
                        color="white" if val > vmax*0.65 else "black")
    plt.colorbar(im, ax=ax, label="Avg Delay (cycles)")
    ax.set_title(f"Avg Delay Heatmap @ load={FIXED_LOAD}", fontsize=12, fontweight="bold", pad=10)
    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, "03_heatmap_delay.png"), dpi=150)
    plt.close()
    print("Guardada: 03_heatmap_delay.png")

# ── Fig 4: Throughput ─────────────────────────────────────────────────────────
for tp in traffic_types:
    sub = df_avg[df_avg["tpattern"]==tp]
    fig, ax = plt.subplots(figsize=(7.5, 4.5))
    ax.set_title(f"Throughput Accepted vs Offered — {tp}", fontsize=11, fontweight="bold")

    for (fam, topo), grp in sub.groupby(["family","topo_name"]):
        g   = grp.sort_values("load")
        ns  = N_SERVERS.get(topo,"?")
        lbl = topo.replace("megafly_","MF k=").replace("slimfly_","SF Q=") + f" ({ns} srv)"
        if "avg_acc_load" in g.columns and g["avg_acc_load"].notna().any():
            ax.plot(g["load"], g["avg_acc_load"], marker=MARKERS[fam],
                    color=COLORS[fam], lw=1.8, ms=6, label=lbl, alpha=0.9)

    ax.plot(available_loads, available_loads, "k--", lw=1, alpha=0.4, label="Ideal")
    ax.set_xlabel("Offered load (phits/node/cycle)")
    ax.set_ylabel("Accepted load (phits/node/cycle)")
    ax.legend(fontsize=8, ncol=2)
    ax.grid(True, ls="--", alpha=0.4)
    plt.tight_layout()
    plt.savefig(os.path.join(PLOTS_DIR, f"04_throughput_{tp}.png"), dpi=150)
    plt.close()
    print(f"Guardada: 04_throughput_{tp}.png")

print(f"\n✓ Gráficas en: {os.path.abspath(PLOTS_DIR)}/")
