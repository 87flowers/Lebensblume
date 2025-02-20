set -euo pipefail
bench=`./zig-out/bin/lebensblume bench | grep "bench results:" -A 3`
git commit -m "$bench" -e
