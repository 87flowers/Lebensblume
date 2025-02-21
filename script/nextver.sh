set -euo pipefail
if [[ $(git diff HEAD --stat) != '' ]]; then
  echo "Dirty tree. Commit first please."
  exit 1
fi
next_ver=`cat ./src/lb_version | awk -F. -v OFS=. '{$NF=$NF+1;print}'`
echo $next_ver > ./src/lb_version
zig build -Doptimize=ReleaseFast
version=`echo "usi" | ./zig-out/bin/lebensblume | grep "id name" | cut --delimiter=" " -f4`
if [[ -e "out/lebensblume-$version" ]]; then
  echo "Version $version alredy exists"
  exit 1
fi
rm "out/lebensblume-latest"
cp ./zig-out/bin/lebensblume "out/lebensblume-latest"
cp ./zig-out/bin/lebensblume "out/lebensblume-$version"
echo "Version $version stored"
echo "Running bench..."
bench=`./zig-out/bin/lebensblume bench | grep "bench results:" -A 3`
git add ./src/lb_version
git commit -m "[$version]" -m "$bench" -e
git push && git push github
