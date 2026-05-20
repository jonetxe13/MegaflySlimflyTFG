#!/bin/bash
# ============================================================
# Comparación MegaFly vs SlimFly con routing Valiant en INRFlow
# Modo DINÁMICO — barre injmode para simular distintas cargas
# Topologías de tamaño similar: megafly_10 y slimfly_7
# ============================================================

INRFLOW="./build/bin/inrflow"
OUTDIR="results_dynamic"
WORKLOAD_DIR="workloads"
mkdir -p "$OUTDIR"
mkdir -p "$WORKLOAD_DIR"

# ============================================================
# PARÁMETROS
# ============================================================

MEGAFLY_K=10
SLIMFLY_Q=7

# injmode: 0=todos los flujos a la vez (carga máxima), >0=menos carga
INJMODES=(0 1 2 4 8)

# Capacidad: c1=enlaces servidor, c2=enlaces switch (en bps)
CAPACITY="10000000_10000000_10000000_10000000"

SCHEDULING="fcfs"

SEEDS=(13)

declare -A TOPO_SIZES
TOPO_SIZES["megafly_10"]=650
TOPO_SIZES["slimfly_7"]=588   # ajusta si el valor real es distinto

TRAFFIC_PATTERNS=(
    "random_10000"
    "hotregion_10"
    "shift_1"
)

# ============================================================
# ÍNDICE DE RESULTADOS
# ============================================================
echo "topology,routing,traffic,injmode,seed,outdir" > "$OUTDIR/index.csv"

# ============================================================
# FUNCIÓN: generar fichero de workload
# ============================================================
make_workload() {
    local PATTERN="$1"
    local SIZE="$2"
    local FILE="$3"
    echo "0 ${PATTERN} ${SIZE} sequential" > "$FILE"
}

# ============================================================
# FUNCIÓN: lanzar una simulación dinámica
# ============================================================
run_sim() {
    local TOPO="$1"
    local INJMODE="$2"
    local SEED="$3"
    local PATTERN="$4"
    local WORKLOAD_FILE="$5"
    local OUTFILE="$6"
    local TOPO_BASE=$(echo "$TOPO" | sed 's/_[0-9]*$//')
    local ROUTING="${TOPO_BASE}-valiant"

    echo ">>> topo=$TOPO | routing=$ROUTING | pattern=$PATTERN | injmode=$INJMODE | seed=$SEED"

    "$INRFLOW" \
        topo="$TOPO" \
        routing="$ROUTING" \
        mode=dynamic \
        injmode="$INJMODE" \
        capacity="$CAPACITY" \
        scheduling="$SCHEDULING" \
        workload="file_$(pwd)/${WORKLOAD_DIR}/workload_${TOPO}_${PATTERN}.txt" \
        rseed="$SEED" \
        verbose=1 \
        > "${OUTFILE}.out" 2>&1

    # Renombrar ficheros generados por INRFlow (el 2>/dev/null va en el subshell, no en el for)
    for DAT in *.dat *.scheduling *.execution *.applications *.list_applications; do
        [ -f "$DAT" ] && mv "$DAT" "${OUTDIR}/${TOPO}_valiant_${PATTERN}_inj${INJMODE}_seed${SEED}_${DAT}"
    done

    if grep -qi "error\|failed\|unknown" "${OUTFILE}.out"; then
        echo "    [!] Posible error - revisa ${OUTFILE}.out"
    else
        echo "    [OK]"
    fi
}

# ============================================================
# BUCLE PRINCIPAL
# ============================================================
for TOPO in "megafly_${MEGAFLY_K}" "slimfly_${SLIMFLY_Q}"; do
    SIZE=${TOPO_SIZES[$TOPO]}
    ROUTING="${TOPO}-valiant"
    echo ""
    echo "=== Iniciando simulaciones: $TOPO (routing=$ROUTING, size=$SIZE) ==="

    for PATTERN in "${TRAFFIC_PATTERNS[@]}"; do
        # WORKLOAD_FILE="${WORKLOAD_DIR}/workload_${TOPO}_${PATTERN}.txt"
        WORKLOAD_FILE="$(pwd)/${WORKLOAD_DIR}/workload_${TOPO}_${PATTERN}.txt"
        make_workload "$PATTERN" "$SIZE" "$WORKLOAD_FILE"

        for INJMODE in "${INJMODES[@]}"; do
            for SEED in "${SEEDS[@]}"; do
                OUTFILE="${OUTDIR}/${TOPO}_valiant_${PATTERN}_inj${INJMODE}_seed${SEED}"
                run_sim "$TOPO" "$INJMODE" "$SEED" "$PATTERN" "$WORKLOAD_FILE" "$OUTFILE"
                echo "${TOPO},${ROUTING},${PATTERN},${INJMODE},${SEED},${OUTFILE}" >> "$OUTDIR/index.csv"
            done
        done
    done
done

echo ""
echo "=== Completado. Índice en: $OUTDIR/index.csv ==="

