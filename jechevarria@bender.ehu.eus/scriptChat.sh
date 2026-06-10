#!/bin/bash
# Script de comparación multi-tamaño para routing Dragonfly
# Compara varios algoritmos en diferentes tamaños de red

# Configuración
BINARY="./build/bin/inrflow"
OUTPUT_DIR="results_multi_size_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

# Múltiples tamaños de red a probar
TOPOLOGIES=(
    "megafly_4"    # Pequeña: 42 servidores
    # "dragonfly_4_4_4"    # Mediana: 272 servidores
    # "dragonfly_6_6_6"    # Grande: 1512 servidores
)

# Algoritmos a comparar
ROUTINGS=(
    # "dragonfly-min"
    "dragonfly-valiant"
    # "dragonfly-quick-valiant-private"
    # "dragonfly-quick-valiant-local"
)

# Patrón de tráfico
PATTERN="random_500"

# Seed fijo para comparación justa
SEED=42

echo "================================================================"
echo "COMPARACIÓN MULTI-TAMAÑO - ROUTING DRAGONFLY"
echo "================================================================"
echo "Directorio: $OUTPUT_DIR"
echo "Topologías: ${#TOPOLOGIES[@]}"
echo "Algoritmos: ${#ROUTINGS[@]}"
echo "Total experimentos: $((${#TOPOLOGIES[@]} * ${#ROUTINGS[@]}))"
echo ""

# Contador
TOTAL=$((${#TOPOLOGIES[@]} * ${#ROUTINGS[@]}))
CURRENT=0

# Crear archivo de resumen
SUMMARY="${OUTPUT_DIR}/summary.txt"
echo "Resumen de Comparación Multi-Tamaño" > "$SUMMARY"
echo "Generado: $(date)" >> "$SUMMARY"
echo "================================================================" >> "$SUMMARY"
echo "" >> "$SUMMARY"

# Loop principal
for topo in "${TOPOLOGIES[@]}"; do
    echo ""
    echo "──────────────────────────────────────────────────────────────"
    echo "Procesando topología: $topo"
    echo "──────────────────────────────────────────────────────────────"

    # Crear directorio para esta topología
    TOPO_DIR="${OUTPUT_DIR}/${topo}"
    mkdir -p "$TOPO_DIR"

    for routing in "${ROUTINGS[@]}"; do
        CURRENT=$((CURRENT + 1))
        echo -ne "  [$CURRENT/$TOTAL] Ejecutando: $routing...\r"

        # Archivo de salida
        OUTPUT_FILE="${TOPO_DIR}/${routing}_seed${SEED}.txt"

        # Ejecutar simulación
        $BINARY \
            topo=$topo \
            routing=$routing \
            mode=static \
            tpattern=$PATTERN \
            placement=sequential \
            rseed=$SEED \
            verbose=0 \
            > "$OUTPUT_FILE" 2>&1

        # Verificar éxito
        if [ $? -eq 0 ]; then
            echo "  [$CURRENT/$TOTAL] ✓ $routing completado      "
        else
            echo "  [$CURRENT/$TOTAL] ✗ ERROR: $routing          "
            echo "ERROR: $topo - $routing" >> "${OUTPUT_DIR}/errors.log"
        fi
    done
done

echo ""
echo "================================================================"
echo "EXTRAYENDO MÉTRICAS"
echo "================================================================"

# Función para extraer métrica específica
extract_metric() {
    local file=$1
    local metric=$2
    grep "$metric" "$file" 2>/dev/null | awk '{print $2}'
}

# Crear tabla comparativa
COMPARISON="${OUTPUT_DIR}/comparison_table.txt"
echo "Tabla Comparativa - Routing por Tamaño de Red" > "$COMPARISON"
echo "================================================================" >> "$COMPARISON"
printf "%-20s %-25s %-15s %-15s %-15s\n" \
    "Topología" "Routing" "PathLength" "Conectiv%" "MaxFlows" >> "$COMPARISON"
echo "================================================================" >> "$COMPARISON"

# Extraer datos de todos los experimentos
for topo in "${TOPOLOGIES[@]}"; do
    echo "" >> "$COMPARISON"

    for routing in "${ROUTINGS[@]}"; do
        FILE="${OUTPUT_DIR}/${topo}/${routing}_seed${SEED}.txt"

        if [ -f "$FILE" ]; then
            PATH_LEN=$(extract_metric "$FILE" "mean.link.path.length")
            CONNECTIVITY=$(extract_metric "$FILE" "p.flows.connected")
            MAX_FLOWS=$(extract_metric "$FILE" "max.flows.per.link")

            # Formatear valores
            PATH_LEN=${PATH_LEN:-"N/A"}
            CONNECTIVITY=${CONNECTIVITY:-"N/A"}
            MAX_FLOWS=${MAX_FLOWS:-"N/A"}

            printf "%-20s %-25s %-15s %-15s %-15s\n" \
                "$topo" "$routing" "$PATH_LEN" "$CONNECTIVITY" "$MAX_FLOWS" >> "$COMPARISON"
        else
            printf "%-20s %-25s %-15s %-15s %-15s\n" \
                "$topo" "$routing" "ERROR" "ERROR" "ERROR" >> "$COMPARISON"
        fi
    done
done

echo ""
echo "================================================================"
echo "ANÁLISIS POR TOPOLOGÍA"
echo "================================================================"

# Análisis por cada topología
for topo in "${TOPOLOGIES[@]}"; do
    ANALYSIS="${OUTPUT_DIR}/analysis_${topo}.txt"
    echo "Análisis para: $topo" > "$ANALYSIS"
    echo "================================================================" >> "$ANALYSIS"
    echo "" >> "$ANALYSIS"

    echo "Comparación de Algoritmos de Routing:" >> "$ANALYSIS"
    echo "---" >> "$ANALYSIS"

    # Comparar MIN vs otros
    MIN_FILE="${OUTPUT_DIR}/${topo}/dragonfly-min_seed${SEED}.txt"
    if [ -f "$MIN_FILE" ]; then
        MIN_PATH=$(extract_metric "$MIN_FILE" "mean.link.path.length")
        echo "Baseline (MIN): Path Length = $MIN_PATH" >> "$ANALYSIS"
        echo "" >> "$ANALYSIS"
    fi

    # Comparar todos los routings
    for routing in "${ROUTINGS[@]}"; do
        FILE="${OUTPUT_DIR}/${topo}/${routing}_seed${SEED}.txt"

        if [ -f "$FILE" ]; then
            PATH_LEN=$(extract_metric "$FILE" "mean.link.path.length")
            CONNECTIVITY=$(extract_metric "$FILE" "p.flows.connected")
            MAX_FLOWS=$(extract_metric "$FILE" "max.flows.per.link")
            AVG_FLOWS=$(extract_metric "$FILE" "mean.flows.per.link")

            echo "Routing: $routing" >> "$ANALYSIS"
            echo "  - Path Length: $PATH_LEN" >> "$ANALYSIS"
            echo "  - Conectividad: ${CONNECTIVITY}%" >> "$ANALYSIS"
            echo "  - Max Flows/Link: $MAX_FLOWS" >> "$ANALYSIS"
            echo "  - Avg Flows/Link: $AVG_FLOWS" >> "$ANALYSIS"

            # Calcular diferencia con MIN si disponible
            if [ -n "$MIN_PATH" ] && [ "$routing" != "dragonfly-min" ]; then
                DIFF=$(echo "$PATH_LEN - $MIN_PATH" | bc -l 2>/dev/null)
                if [ -n "$DIFF" ]; then
                    echo "  - Diferencia vs MIN: +$DIFF links" >> "$ANALYSIS"
                fi
            fi
            echo "" >> "$ANALYSIS"
        fi
    done
done

echo ""
echo "================================================================"
echo "ANÁLISIS DE ESCALABILIDAD"
echo "================================================================"

SCALABILITY="${OUTPUT_DIR}/scalability_analysis.txt"
echo "Análisis de Escalabilidad por Routing" > "$SCALABILITY"
echo "================================================================" >> "$SCALABILITY"
echo "" >> "$SCALABILITY"

# Analizar cada routing a través de tamaños
for routing in "${ROUTINGS[@]}"; do
    echo "Routing: $routing" >> "$SCALABILITY"
    echo "---" >> "$SCALABILITY"

    for topo in "${TOPOLOGIES[@]}"; do
        FILE="${OUTPUT_DIR}/${topo}/${routing}_seed${SEED}.txt"

        if [ -f "$FILE" ]; then
            PATH_LEN=$(extract_metric "$FILE" "mean.link.path.length")
            SERVERS=$(extract_metric "$FILE" "n.servers")

            echo "  $topo (${SERVERS} servidores): Path Length = $PATH_LEN" >> "$SCALABILITY"
        fi
    done
    echo "" >> "$SCALABILITY"
done

echo ""
echo "================================================================"
echo "COMPARACIÓN MIN vs VALIANT vs QUICKVALIANT"
echo "================================================================"

VERSUS="${OUTPUT_DIR}/min_vs_valiant_vs_quick.txt"
echo "Comparación Directa: MIN vs VALIANT vs QuickValiant" > "$VERSUS"
echo "================================================================" >> "$VERSUS"
echo "" >> "$VERSUS"

for topo in "${TOPOLOGIES[@]}"; do
    echo "Topología: $topo" >> "$VERSUS"
    echo "---" >> "$VERSUS"

    MIN_FILE="${OUTPUT_DIR}/${topo}/dragonfly-min_seed${SEED}.txt"
    VAL_FILE="${OUTPUT_DIR}/${topo}/dragonfly-valiant_seed${SEED}.txt"
    QPRIV_FILE="${OUTPUT_DIR}/${topo}/quick-valiant-private_seed${SEED}.txt"
    QLOC_FILE="${OUTPUT_DIR}/${topo}/quick-valiant-local_seed${SEED}.txt"

    if [ -f "$MIN_FILE" ]; then
        MIN_PATH=$(extract_metric "$MIN_FILE" "mean.link.path.length")
        echo "  MIN:                      $MIN_PATH" >> "$VERSUS"
    fi

    if [ -f "$VAL_FILE" ]; then
        VAL_PATH=$(extract_metric "$VAL_FILE" "mean.link.path.length")
        echo "  VALIANT:                  $VAL_PATH" >> "$VERSUS"
    fi

    if [ -f "$QPRIV_FILE" ]; then
        QPRIV_PATH=$(extract_metric "$QPRIV_FILE" "mean.link.path.length")
        echo "  QuickValiant-Private:     $QPRIV_PATH" >> "$VERSUS"
    fi

    if [ -f "$QLOC_FILE" ]; then
        QLOC_PATH=$(extract_metric "$QLOC_FILE" "mean.link.path.length")
        echo "  QuickValiant-Local:       $QLOC_PATH" >> "$VERSUS"
    fi

    # Calcular diferencias
    if [ -n "$MIN_PATH" ] && [ -n "$VAL_PATH" ]; then
        DIFF_VAL=$(echo "$VAL_PATH - $MIN_PATH" | bc -l 2>/dev/null)
        echo "" >> "$VERSUS"
        echo "  Diferencia VALIANT vs MIN: +$DIFF_VAL links" >> "$VERSUS"
    fi

    if [ -n "$MIN_PATH" ] && [ -n "$QPRIV_PATH" ]; then
        DIFF_QPRIV=$(echo "$QPRIV_PATH - $MIN_PATH" | bc -l 2>/dev/null)
        echo "  Diferencia QuickVal-Priv vs MIN: +$DIFF_QPRIV links" >> "$VERSUS"
    fi

    echo "" >> "$VERSUS"
done

echo ""
echo "================================================================"
echo "RESULTADOS COMPLETOS"
echo "================================================================"
echo ""
echo "✓ Experimentos completados: $TOTAL"
echo ""
echo "Archivos generados:"
echo "  - Tabla comparativa:       $COMPARISON"
echo "  - Análisis escalabilidad:  $SCALABILITY"
echo "  - MIN vs VALIANT vs Quick: $VERSUS"
echo "  - Análisis por topología:  ${OUTPUT_DIR}/analysis_*.txt"
echo ""
echo "Para ver resultados:"
echo "  cat $COMPARISON"
echo "  cat $VERSUS"
echo ""

# Mostrar tabla en consola
echo "════════════════════════════════════════════════════════════════"
echo "TABLA COMPARATIVA RESUMIDA"
echo "════════════════════════════════════════════════════════════════"
cat "$COMPARISON"
echo ""

echo "════════════════════════════════════════════════════════════════"
echo "MIN vs VALIANT vs QuickValiant"
echo "════════════════════════════════════════════════════════════════"
cat "$VERSUS"
echo ""

echo "✓ Script completado exitosamente"
echo ""

