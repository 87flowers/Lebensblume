set -euo pipefail
if [[ $(git diff --stat) != '' ]]; then
  echo "Dirty tree. Commit first please."
  exit 1
fi
zig build -Doptimize=ReleaseFast
version=`echo "usi" | ./zig-out/bin/lebensblume | grep "id name" | cut --delimiter=" " -f4`
if [[ -e "out/lebensblume-$version" ]]; then
  echo "Version $version alredy exists"
  exit 1
fi
cp ./zig-out/bin/lebensblume "out/lebensblume-$version"
cp ./zig-out/bin/lebensblume "out/lebensblume-latest"
echo "Version $version stored"
