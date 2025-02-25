set -euo pipefail
if [[ $(git diff HEAD --stat) != '' ]]; then
  echo "Dirty tree. Commit first please."
  exit 1
fi
next_ver=`cat ./src/lb_version.txt | awk -F. -v OFS=. '{$NF=$NF+1;print}'`
echo $next_ver > ./src/lb_version.txt
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
version=`echo "usi" | ./build/lebensblume | grep "id name" | cut --delimiter=" " -f4`
if [[ -e "out/lebensblume-$version" ]]; then
  echo "Version $version alredy exists"
  exit 1
fi
rm "out/lebensblume-latest"
cp ./build/lebensblume "out/lebensblume-latest"
cp ./build/lebensblume "out/lebensblume-$version"
echo "Version $version stored"
echo "Running bench..."
bench=`./build/lebensblume bench | grep "bench results:" -A 3`
git add ./src/lb_version.txt
git commit -m "[$version]" -m "$bench" -e
git push
