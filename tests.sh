# Get absolute path of the silk.sh script
silk_sh=$(realpath silk.sh)

shopt -s globstar
for f in ./tests/**/silk.c; do
  echo "Starting test for: $f" 
  $silk_sh --pedantic --file  "$(realpath "$f")" || { exit 1; }
done
