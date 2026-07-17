#!/bin/bash

# Cantidad y tamaño de los archivos de prueba
NUM_ARCHIVOS=20
MB_POR_ARCHIVO=10

DIRECTORIO_PRUEBA="prueba_rendimiento"
PREFIJO="bench_"

TOTAL_MB=$((NUM_ARCHIVOS * MB_POR_ARCHIVO))
TOTAL_BYTES=$((TOTAL_MB * 1024 * 1024))

echo "**************************************"
echo "  Prueba de Rendimiento MINIFILESYNC  "
echo "**************************************"

# Limpia pruebas anteriores
rm -rf "$DIRECTORIO_PRUEBA"
mkdir -p "$DIRECTORIO_PRUEBA"
mkdir -p backup
mkdir -p logs

rm -f backup/${PREFIJO}*.bin

# Inicia el Monitor en segundo plano
./main monitor "$DIRECTORIO_PRUEBA" \
    > /tmp/minisync_benchmark.log 2>&1 &

PID_MONITOR=$!

echo "PID del Monitor: $PID_MONITOR"
echo "Esperando el escaneo inicial..."

# Espera a que el Monitor realice su primer escaneo
sleep 6

echo "Creando $NUM_ARCHIVOS archivos..."
echo "Tamaño por archivo: $MB_POR_ARCHIVO MB"
echo "Tamaño total: $TOTAL_MB MB"

INICIO=$(date +%s.%N)

# Crea los archivos después del escaneo inicial
for i in $(seq 1 "$NUM_ARCHIVOS")
do
    dd \
        if=/dev/zero \
        of="$DIRECTORIO_PRUEBA/${PREFIJO}${i}.bin" \
        bs=1M \
        count="$MB_POR_ARCHIVO" \
        status=none
done

echo "Esperando que los Workers terminen las copias..."

while true
do
    CANTIDAD_COPIADA=$(
        find backup \
            -maxdepth 1 \
            -type f \
            -name "${PREFIJO}*.bin" \
            | wc -l
    )

    BYTES_COPIADOS=$(
        find backup \
            -maxdepth 1 \
            -type f \
            -name "${PREFIJO}*.bin" \
            -printf '%s\n' \
            | awk '{total += $1} END {print total + 0}'
    )

    if [ "$CANTIDAD_COPIADA" -eq "$NUM_ARCHIVOS" ] \
       && [ "$BYTES_COPIADOS" -ge "$TOTAL_BYTES" ]
    then
        break
    fi

    sleep 0.2
done

FIN=$(date +%s.%N)

TIEMPO=$(
    awk \
        -v inicio="$INICIO" \
        -v fin="$FIN" \
        'BEGIN {printf "%.3f", fin - inicio}'
)

ARCHIVOS_SEGUNDO=$(
    awk \
        -v archivos="$NUM_ARCHIVOS" \
        -v tiempo="$TIEMPO" \
        'BEGIN {printf "%.2f", archivos / tiempo}'
)

MB_SEGUNDO=$(
    awk \
        -v megabytes="$TOTAL_MB" \
        -v tiempo="$TIEMPO" \
        'BEGIN {printf "%.2f", megabytes / tiempo}'
)

echo ""
echo "                                          "
echo "||RESULTADOS"
echo ""
echo "|||Archivos copiados: $NUM_ARCHIVOS"
echo "|||Tamaño total: $TOTAL_MB MB"
echo "|||Tiempo total: $TIEMPO segundos"
echo "|||Archivos por segundo: $ARCHIVOS_SEGUNDO"
echo "|||Megabytes por segundo: $MB_SEGUNDO"
echo "**************************************"

# Finaliza correctamente el Monitor
kill -INT "$PID_MONITOR" 2>/dev/null

wait "$PID_MONITOR" 2>/dev/null

echo ""
echo "Monitor finalizado."
echo "Salida guardada en:"
echo "/tmp/minisync_benchmark.log"