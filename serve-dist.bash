
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]:-$0}"; )" &> /dev/null && pwd 2> /dev/null; )";

cd "$SCRIPT_DIR" || exit 1

mkdir -p "$SCRIPT_DIR"/dist

cd "$SCRIPT_DIR"/dist

ifconfig  | grep -E 'inet\s'
ls
python3 -m http.server 7777

