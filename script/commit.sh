set -euo pipefail
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
bench=`./build/lebensblume bench | grep "bench results:" -A 3`
git commit -m "$bench" -e
