#!/usr/bin/env python3
"""
parse_and_plot.py
Parsea los logs de INRFlow generados por la comparación MegaFly vs SlimFly
y genera gráficas comparativas en la carpeta plots/
"""

import os, re, json
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np


def megafly_size(k):
    if k % 4 != 0: return None, None
    p = k // 4
    g = k * (k // 4) + 1
    n_sw = k * g
    n_srv = p * n_sw
    return n_sw, n_srv

def slimfly_size(Q):
    n_sw  = 2 * Q * Q
    p     = Q - 1
    n_srv = n_sw * p
    return n_sw, n_srv


RESULTS_DIR = "results"
PLOTS_DIR   = "plots"
os.makedirs(PLOTS_DIR, exist_ok=True)

# ============================================================
# 1. PARSEO
# ============================================================

SCALAR_FIELDS = {
    "n.servers":                                    "n_servers",
    "n.switches":                                   "n_switches",
    "switch.radix":                                 "switch_radix",
    "n.links":                                      "n_links",
    "pattern":                                      "pattern",
    "n.flows":                                      "n_flows",
    "n.flows.delivered":                            "n_flows_delivered",
    "p.flows.connected":                            "p_flows_connected",
    "r.seed":                                       "seed",
    "min.link.path.length":                         "path_min",
    "max.link.path.length":                         "path_max",
    "mean.link.path.length":                        "path_mean",
    "std.link.path.length":                         "path_std",
    "mean.server.hop.length":                       "hop_mean",
    "std.server.hop.length":                        "hop_std",
    "min.flows.per.link":                           "flows_link_min",
    "max.flows.per.link":                           "flows_link_max",
    "mean.flows.per.link":                          "flows_link_mean",
    "std.flows.per.link":                           "flows_link_std",
    "connected.nonzero.flows.over.bottleneck.flow": "flows_bottleneck",
    "connected.flows.over.mean.flow":               "flows_over_mean",
}

def parse_log(filepath):
    fname  = os.path.basename(filepath)
    m = re.match(r"(megafly_\d+|slimfly_\d+)_dragonfly_min_(.+)_seed(\d+)\.log", fname)
    if not m:
        return None

    topo_name, tpattern, seed = m.group(1), m.group(2), int(m.group(3))
    family     = "MegaFly" if topo_name.startswith("megafly") else "SlimFly"
    size_param = int(topo_name.split("_")[1])

    row = dict(filename=fname, topo_name=topo_name, family=family,
               size_param=size_param, tpattern=tpattern, seed=seed)

    with open(filepath) as f:
        content = f.read()

    for log_key, col in SCALAR_FIELDS.items():
        pat   = rf"^{re.escape(log_key)}\s+(\S+)\s*$"
        match = re.search(pat, content, re.MULTILINE)
        if match:
            val = match.group(1)
            try:
                if val in ("inf", "Inf", "INF", "infinity", "nan", "NaN"):
                    val = float("nan")
                else:
                    val = float(val) if "." in val else int(val)
            except ValueError:
                pass
            row[col] = val

    # Histogramas en JSON (por si los necesitas después)
    path_hist = {int(a): int(b) for a, b in re.findall(r"^p\s+(\d+)\s+(\d+)", content, re.MULTILINE)}
    flow_hist = {int(a): int(b) for a, b in re.findall(r"^f\s+(\d+)\s+(\d+)", content, re.MULTILINE)}
    row["path_hist"] = json.dumps(path_hist)
    row["flow_hist"] = json.dumps(flow_hist)

    return row

rows = []
for fname in sorted(os.listdir(RESULTS_DIR)):
    if fname.endswith(".log"):
        r = parse_log(os.path.join(RESULTS_DIR, fname))
        if r:
            rows.append(r)

df = pd.DataFrame(rows)
# ratio de congestión: max/mean (cuanto mayor, peor)

# Promediar sobre semillas
numeric_cols = ["n_servers", "n_switches", "switch_radix", "n_links",
                "path_mean", "path_std", "path_max",
                "hop_mean", "hop_std",
                "flows_link_mean", "flows_link_std", "flows_link_max",
                "flows_bottleneck", "flows_over_mean", "p_flows_connected"]


df_avg = (df.groupby(["family", "topo_name", "size_param", "tpattern"])[numeric_cols]
            .mean(numeric_only=True)
            .reset_index())

# Guardar CSV limpio
df.to_csv(os.path.join(RESULTS_DIR, "parsed_all.csv"), index=False)
df_avg.to_csv(os.path.join(RESULTS_DIR, "parsed_avg.csv"), index=False)
print(f"Parseados {len(df)} logs → {len(df_avg)} combinaciones (promediadas sobre semillas)")
print(df_avg[["family","topo_name","tpattern","path_mean","hop_mean","flows_link_mean"]].to_string())

# ============================================================
# 2. GRAFICAS
# ============================================================

TRAFFIC_LABELS = {
    "bisection":    "Bisection",
    "alltoall":     "All-to-all",
    "random_5000":  "Random 5k",
    "hotregion_1000": "Hot region",
    "shift_5":      "Shift-5",
}
COLORS = {"MegaFly": "#4C72B0", "SlimFly": "#DD8452"}
MARKERS = {"MegaFly": "o", "SlimFly": "s"}

traffic_types = sorted(df_avg["tpattern"].unique())

# ----------------------------------------------------------
# Fig 1: Longitud media de camino (path_mean) por tamaño de red
#         Una subfigura por patrón de tráfico
# ----------------------------------------------------------
fig, axes = plt.subplots(1, len(traffic_types), figsize=(4*len(traffic_types), 4), sharey=False)
fig.suptitle("Mean Path Length — MegaFly vs SlimFly", fontsize=14, fontweight="bold")

for ax, tp in zip(axes, traffic_types):
    sub = df_avg[df_avg["tpattern"] == tp].sort_values("n_servers")
    for family, grp in sub.groupby("family"):
        ax.plot(grp["n_servers"], grp["path_mean"],
                marker=MARKERS[family], color=COLORS[family],
                linewidth=2, markersize=7, label=family)
        ax.fill_between(grp["n_servers"],
                        grp["path_mean"] - grp["path_std"],
                        grp["path_mean"] + grp["path_std"],
                        alpha=0.15, color=COLORS[family])
    ax.set_title(TRAFFIC_LABELS.get(tp, tp), fontsize=10)
    ax.set_xlabel("N servers")
    ax.set_ylabel("Mean path length (hops)")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(fontsize=8)

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, "01_path_mean.png"), dpi=150)
plt.close()
print("Guardada: 01_path_mean.png")

# ----------------------------------------------------------
# Fig 2: Distribución de flujos por enlace (flows_link_mean ± std)
#         barras agrupadas por topología y tráfico
# ----------------------------------------------------------
fig, axes = plt.subplots(1, len(traffic_types), figsize=(4*len(traffic_types), 4), sharey=False)
fig.suptitle("Mean Flows per Link — MegaFly vs SlimFly", fontsize=14, fontweight="bold")

for ax, tp in zip(axes, traffic_types):
    sub = df_avg[df_avg["tpattern"] == tp].sort_values("n_servers")
    for family, grp in sub.groupby("family"):
        ax.errorbar(grp["n_servers"], grp["flows_link_mean"],
                    yerr=grp["flows_link_std"],
                    marker=MARKERS[family], color=COLORS[family],
                    linewidth=2, markersize=7, capsize=4, label=family)
    ax.set_title(TRAFFIC_LABELS.get(tp, tp), fontsize=10)
    ax.set_xlabel("N servers")
    ax.set_ylabel("Flows per link")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(fontsize=8)

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, "02_flows_per_link.png"), dpi=150)
plt.close()
print("Guardada: 02_flows_per_link.png")

# ----------------------------------------------------------
# Fig 3: Flows over bottleneck — métrica de congestión
# ----------------------------------------------------------
fig, axes = plt.subplots(1, len(traffic_types), figsize=(4*len(traffic_types), 4), sharey=False)
fig.suptitle("Flows over Bottleneck Link — MegaFly vs SlimFly", fontsize=14, fontweight="bold")

for ax, tp in zip(axes, traffic_types):
    sub = df_avg[df_avg["tpattern"] == tp].sort_values("n_servers")
    for family, grp in sub.groupby("family"):
        ax.plot(grp["n_servers"], grp["flows_bottleneck"],
                marker=MARKERS[family], color=COLORS[family],
                linewidth=2, markersize=7, label=family)
    ax.set_title(TRAFFIC_LABELS.get(tp, tp), fontsize=10)
    ax.set_xlabel("N servers")
    ax.set_ylabel("Flows / bottleneck link")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(fontsize=8)

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, "03_bottleneck.png"), dpi=150)
plt.close()
print("Guardada: 03_bottleneck.png")

# ----------------------------------------------------------
# Fig 4: Heatmap — path_mean por (topología x patrón de tráfico)
# ----------------------------------------------------------
pivot = df_avg.pivot_table(index="topo_name", columns="tpattern",
                           values="path_mean", aggfunc="mean")
pivot.index = [t.replace("megafly_", "MF k=").replace("slimfly_", "SF Q=") for t in pivot.index]
pivot.columns = [TRAFFIC_LABELS.get(c, c) for c in pivot.columns]

fig, ax = plt.subplots(figsize=(len(pivot.columns)*1.8 + 1, len(pivot)*0.8 + 1.5))
im = ax.imshow(pivot.values, aspect="auto", cmap="YlOrRd")
ax.set_xticks(range(len(pivot.columns))); ax.set_xticklabels(pivot.columns, rotation=30, ha="right")
ax.set_yticks(range(len(pivot.index)));   ax.set_yticklabels(pivot.index)
for i in range(pivot.shape[0]):
    for j in range(pivot.shape[1]):
        val = pivot.values[i, j]
        if not np.isnan(val):
            ax.text(j, i, f"{val:.2f}", ha="center", va="center", fontsize=8,
                    color="black" if val < pivot.values.max()*0.7 else "white")
plt.colorbar(im, ax=ax, label="Mean path length")
ax.set_title("Mean Path Length Heatmap", fontsize=13, fontweight="bold", pad=12)
plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, "04_heatmap_path.png"), dpi=150)
plt.close()
print("Guardada: 04_heatmap_path.png")

# ----------------------------------------------------------
# Fig 5: Topología — tamaño de red (n_servers, n_links) por familia
# ----------------------------------------------------------
topo_info = df_avg.drop_duplicates("topo_name")[["family","topo_name","size_param","n_servers","n_switches","n_links"]].sort_values(["family","size_param"])

fig, axes = plt.subplots(1, 2, figsize=(10, 4))
fig.suptitle("Network Size Scaling — MegaFly vs SlimFly", fontsize=13, fontweight="bold")

for family, grp in topo_info.groupby("family"):
    axes[0].plot(grp["size_param"], grp["n_servers"], marker=MARKERS[family],
                 color=COLORS[family], linewidth=2, markersize=8, label=family)
    axes[1].plot(grp["size_param"], grp["n_links"],   marker=MARKERS[family],
                 color=COLORS[family], linewidth=2, markersize=8, label=family)

for ax, ylabel, title in zip(axes,
    ["N servers", "N links"],
    ["Servers vs size param", "Links vs size param"]):
    ax.set_xlabel("Size param (k / Q)")
    ax.set_ylabel(ylabel)
    ax.set_title(title, fontsize=10)
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend()

plt.tight_layout()
plt.savefig(os.path.join(PLOTS_DIR, "05_network_size.png"), dpi=150)
plt.close()
print("Guardada: 05_network_size.png")


pares = [(4,3), (8,5), (16,13), (20,17)]
metricas = [
    ("path_mean",        "Mean path length (hops)", None),
    ("flows_link_mean",  "Mean flows per link",      "flows_link_std"),
    ("congestion_ratio", "Congestion ratio (max/mean)", None),
    ("flows_bottleneck", "Flows over bottleneck",    None),
]

for metric, ylabel, std_col in metricas:
    fig, axes = plt.subplots(1, 4, figsize=(18, 4), sharey=False)
    fig.suptitle(f"{ylabel} — Pares MegaFly vs SlimFly (tamaño similar)", fontsize=12, fontweight="bold")

    for ax, (k, Q) in zip(axes, pares):
        mf = df_avg[df_avg["topo_name"] == f"megafly_{k}"]
        sf = df_avg[df_avg["topo_name"] == f"slimfly_{Q}"]

        # Solo patrones con datos válidos en ambas topologías
        tps_validos = [tp for tp in traffic_types
                       if (len(mf[mf["tpattern"]==tp][metric].dropna()) > 0 and
                           len(sf[sf["tpattern"]==tp][metric].dropna()) > 0)]

        if not tps_validos:
            ax.set_visible(False)
            continue

        x      = np.arange(len(tps_validos))
        mf_vals = [mf[mf["tpattern"]==tp][metric].values[0] for tp in tps_validos]
        sf_vals = [sf[sf["tpattern"]==tp][metric].values[0] for tp in tps_validos]
        mf_err  = [mf[mf["tpattern"]==tp][std_col].values[0] for tp in tps_validos] if std_col else None
        sf_err  = [sf[sf["tpattern"]==tp][std_col].values[0] for tp in tps_validos] if std_col else None

        bars_mf = ax.bar(x - 0.2, mf_vals, width=0.4, yerr=mf_err, capsize=3,
                         label=f"MF k={k}", color=COLORS["MegaFly"], alpha=0.85)
        bars_sf = ax.bar(x + 0.2, sf_vals, width=0.4, yerr=sf_err, capsize=3,
                         label=f"SF Q={Q}", color=COLORS["SlimFly"], alpha=0.85)

        # Anotar % de diferencia encima de cada par
        for i, (mv, sv) in enumerate(zip(mf_vals, sf_vals)):
            if mv and sv and not (np.isnan(mv) or np.isnan(sv)) and mv != 0:
                diff_pct = (sv - mv) / mv * 100
                color = "green" if diff_pct < 0 else "red"
                ax.text(i, max(mv, sv) * 1.05, f"{diff_pct:+.0f}%",
                        ha="center", fontsize=6.5, color=color, fontweight="bold")

        mf_srv = int(mf["n_servers"].dropna().iloc[0]) if len(mf["n_servers"].dropna()) > 0 else "?"
        sf_srv = int(sf["n_servers"].dropna().iloc[0]) if len(sf["n_servers"].dropna()) > 0 else "?"
        ax.set_title(f"MF k={k} ({mf_srv} nodos)\nvs SF Q={Q} ({sf_srv} nodos)", fontsize=8)
        ax.set_xticks(x)
        ax.set_xticklabels([TRAFFIC_LABELS.get(t, t) for t in tps_validos],
                           rotation=40, ha="right", fontsize=7)
        ax.set_ylabel(ylabel)
        ax.legend(fontsize=8)
        ax.grid(axis="y", linestyle="--", alpha=0.4)

    plt.tight_layout()
    safe_name = metric.replace("/", "_").replace(".", "_")
    fname = f"09_paired_{safe_name}.png"
    plt.savefig(os.path.join(PLOTS_DIR, fname), dpi=150)
    plt.close()
    print(f"Guardada: {fname}")
