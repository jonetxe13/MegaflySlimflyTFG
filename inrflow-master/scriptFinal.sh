#!/bin/bash
# ============================================================
# Comparación MegaFly vs SlimFly en INRFlow
# Parámetros pasados directamente como argumentos al binario
# ============================================================

INRFLOW="./build/bin/inrflow"
OUTDIR="results"
mkdir -p "$OUTDIR"

# ============================================================
# PARÁMETROS DE BARRIDO
# ============================================================
MEGAFLY_K_LIST=(4 8 16 20)
SLIMFLY_Q_LIST=(3 5 13 17)

# Cargas (se mapean al workload auto_1_instantaneous_ptp_1_1_1_LOAD_LOAD_sequential_0_0_consecutive)
# Aquí usamos el parámetro 'load' directamente si tu versión lo soporta,
# si no, ajusta el workload manualmente (ver nota abajo)
LOADS=(10 30 50 70 90)   # en Mbps o unidades relativas según tu config

# Patrones de tráfico soportados por INRFlow
TRAFFIC_TYPES=(
    "random_10000"
    "random_50000"
    "random_100000"
    "hotregion_10"
    "hotregion_100"
    "hotregion_500"
    "shift_1"
    "shift_10"
    "shift_100"
    "alltoone"
    "onetoall"
)

# Modo de ejecución
MODE="static"

# Capacidad de enlaces (ajusta según tu red)
CAPACITY="10000000_10000000_10000000_80000000"

# Repeticiones (cambia rseed para cada una)
SEEDS=(13 42 99)

# ============================================================
# ÍNDICE DE RESULTADOS
# ============================================================
echo "topology,routing,traffic,mode,seed,logfile" > "$OUTDIR/index.csv"

# ============================================================
# FUNCIÓN: lanzar una simulación
# ============================================================
run_sim() {
    local TOPO="$1"
    local TRAFFIC="$2"
    local SEED="$3"
    local LOGFILE="$4"

    echo ">>> topo=$TOPO | routing=dragonfly_min | tpattern=$TRAFFIC | mode=$MODE | rseed=$SEED"

    "$INRFLOW" \
        topo="$TOPO" \
        routing=dragonfly_min \
        tpattern="$TRAFFIC" \
        mode="$MODE" \
        capacity="$CAPACITY" \
        rseed="$SEED" \
        placement=sequential \
        verbose=1 \
        > "${LOGFILE}.log" 2>&1

    if grep -q "ERROR\|error\|failed" "${LOGFILE}.log"; then
        echo "    [!] Posible error - revisa ${LOGFILE}.log"
    else
        echo "    [OK]"
    fi
}

# ============================================================
# BUCLE: MegaFly
# ============================================================
for K in "${MEGAFLY_K_LIST[@]}"; do
    TOPO="megafly_${K}"
    for TRAFFIC in "${TRAFFIC_TYPES[@]}"; do
        for SEED in "${SEEDS[@]}"; do
            LOGFILE="${OUTDIR}/${TOPO}_dragonfly_min_${TRAFFIC}_seed${SEED}"
            run_sim "$TOPO" "$TRAFFIC" "$SEED" "$LOGFILE"
            echo "${TOPO},dragonfly_min,${TRAFFIC},${MODE},${SEED},${LOGFILE}.log" >> "$OUTDIR/index.csv"
        done
    done
done

# ============================================================
# BUCLE: SlimFly
# ============================================================
for Q in "${SLIMFLY_Q_LIST[@]}"; do
    TOPO="slimfly_${Q}"
    for TRAFFIC in "${TRAFFIC_TYPES[@]}"; do
        for SEED in "${SEEDS[@]}"; do
            LOGFILE="${OUTDIR}/${TOPO}_dragonfly_min_${TRAFFIC}_seed${SEED}"
            run_sim "$TOPO" "$TRAFFIC" "$SEED" "$LOGFILE"
            echo "${TOPO},dragonfly_min,${TRAFFIC},${MODE},${SEED},${LOGFILE}.log" >> "$OUTDIR/index.csv"
        done
    done
done

echo ""
echo "=== Completado. Índice en: $OUTDIR/index.csv ==="
