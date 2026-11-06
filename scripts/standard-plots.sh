#! /bin/sh

# benchFFT 3.1's scripts/standard-plots.sh, for PUREfft's one problem class (srof, rank 1), as PNG as fftw.org/speed
# usage: sh standard-plots.sh <data> [sizes file]   (the sizes file keeps those alone, e.g. PUREfft's)

sd=`dirname $0`
case $sd in
    /*) ;;
    *) sd=`pwd`/$sd ;;
esac

data=$1
only=${2-}
prefix=""
case $data in
    *.busy.speed) type=speed; dname=`basename $data .speed`; prefix="busy, after 256 KB of other writes: " ;;
    *.counts.speed) type=speed; dname=`basename $data .speed`; prefix="QEMU instruction counts, 1 instruction = 1 ns: " ;;
    *.cycles.speed) type=speed; dname=`basename $data .speed`; prefix="the board's cycle counts, 1 cycle = 1 ns: " ;;
    *.speed) type=speed; dname=`basename $data .speed` ;;
    *.accuracy) type=accuracy; dname=`basename $data .accuracy` ;;
esac
if test -n "$only"; then
    kept=`mktemp`
    awk 'NR == FNR {keep[$1] = 1; next} keep[$3]' "$only" $data > $kept
    data=$kept
    trap 'rm -f $kept' EXIT
fi

rank=1
problem=srxx
pat=`echo $problem | sed 's/xx/[io][fb]/g'`

if test $type = speed; then

plotflags="--no-dups"

for p2 in p2 np2; do
    test `egrep "$pat" $data | perl $sd/grep-rank.pl $rank | perl $sd/grep-${p2}.pl | wc -l` -gt 1 || continue

    (sh $sd/plot-title.sh $problem $rank $p2 "$prefix";
	egrep "$pat" $data |
	perl $sd/grep-rank.pl $rank |
	perl $sd/grep-${p2}.pl | perl $sd/grace-plot.pl $plotflags) |
	gracebat -pipe -hdevice PNG -printfile ${dname}.${rank}d.${problem}.${p2}.png
    echo "${dname}.${rank}d.${problem}.${p2}.png"
done

else # $type = accuracy

plotflags="--no-dups --plot-worst --accuracy"

test `egrep "$pat" $data | perl $sd/grep-rank.pl $rank | wc -l` -gt 1 || exit 0

(sh $sd/plot-title.sh $problem $rank;
egrep "$pat" $data |
    perl $sd/grep-rank.pl $rank |
    perl $sd/grace-plot.pl $plotflags) |
    gracebat -pipe -hdevice PNG -printfile ${dname}.${rank}d.${problem}.png
echo "${dname}.${rank}d.${problem}.png"

(sh $sd/plot-title.sh $problem $rank;
 echo "@subtitle \"enlargement of most accurate FFTs\"";
 egrep "$pat" $data |
    perl $sd/grep-rank.pl $rank |
    perl $sd/grace-plot.pl $plotflags --accurate-only) |
    gracebat -pipe -hdevice PNG -printfile ${dname}.${rank}d.${problem}.acc.png
echo "${dname}.${rank}d.${problem}.acc.png"

(sh $sd/plot-title.sh $problem $rank;
 echo "@subtitle \"enlargement of most accurate FFTs, powers of two only\"";
 egrep "$pat" $data |
    perl $sd/grep-rank.pl $rank | perl $sd/grep-p2.pl |
    perl $sd/grace-plot.pl $plotflags --accurate-only) |
    gracebat -pipe -hdevice PNG -printfile ${dname}.${rank}d.${problem}.acc.p2.png
echo "${dname}.${rank}d.${problem}.acc.p2.png"

fi
