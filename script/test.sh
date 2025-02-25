set -euo pipefail
script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd $script_dir/..
git checkout -- src/lb_version.txt
if [[ $(git diff HEAD --stat) != '' ]]; then
  echo "Dirty tree. Commit first please."
  popd
  exit 1
fi
devver=$(git describe --always --dirty)
echo $devver > src/lb_version.txt
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
echo "dev hash:"
sha1sum build/lebensblume
eta ~/shogibooks/4moves_v1_shogi.epd build/lebensblume out/lebensblume-latest 8+0.08 $1 $2
git checkout -- src/lb_version.txt
popd
