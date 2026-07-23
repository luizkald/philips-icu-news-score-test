#!/usr/bin/env bash
#
# watch_demo.sh - Reproduz uma sessão da CLI "ao vivo": alimenta a entrada do
# app icu_news linha a linha, com uma pausa entre cada linha, para acompanhar o
# dashboard e o LED sendo atualizados a cada ação (como se alguém digitasse).
#
# Uso:
#   ./watch_demo.sh [arquivo_de_entrada] [atraso_segundos]
#
# Exemplos:
#   ./watch_demo.sh                 # demo.txt, 0.6s entre linhas
#   ./watch_demo.sh demo2.txt       # demo2.txt, 0.6s
#   ./watch_demo.sh demo.txt 1.2    # mais devagar
#
# Requer o binário já compilado (cmake --build build).

set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP="$DIR/build/icu_news"
INPUT="${1:-$DIR/demo.txt}"
DELAY="${2:-0.6}"

# Permite passar só o nome do arquivo (resolve relativo ao diretório do script).
if [[ ! -f "$INPUT" && -f "$DIR/$INPUT" ]]; then
    INPUT="$DIR/$INPUT"
fi

if [[ ! -x "$APP" ]]; then
    echo "Binário não encontrado em $APP" >&2
    echo "Compile antes: cmake -S \"$DIR\" -B \"$DIR/build\" -G Ninja && cmake --build \"$DIR/build\"" >&2
    exit 1
fi
if [[ ! -f "$INPUT" ]]; then
    echo "Arquivo de entrada não encontrado: $INPUT" >&2
    exit 1
fi

echo "▶ Reproduzindo '$INPUT' em '$APP' (atraso ${DELAY}s por linha)..." >&2

# Envia cada linha do arquivo para o app com uma pausa, mantendo o pipe aberto
# até o fim para o app processar cada comando conforme "digitado".
{
    while IFS= read -r line || [[ -n "$line" ]]; do
        printf '%s\n' "$line"
        sleep "$DELAY"
    done < "$INPUT"
} | "$APP"
