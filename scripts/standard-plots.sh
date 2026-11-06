#! /bin/sh

# benchFFT 3.1's scripts/standard-plots.sh, for PUREfft's one problem class (srof, rank 1), as SVG

sd=`dirname $0`
case $sd in
    /*) ;;
    *) sd=`pwd`/$sd ;;
esac

data=$1
prefix=""
case $data in
    *.busy.speed) type=speed; dname=`basename $data .speed`; prefix="busy, after 256 KB of other writes: " ;;
    *.counts.speed) type=speed; dname=`basename $data .speed`; prefix="QEMU instruction counts, 1 instruction = 1 ns: " ;;
    *.speed) type=speed; dname=`basename $data .speed` ;;
    *.accuracy) type=accuracy; dname=`basename $data .accuracy` ;;
esac

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
	gracebat -pipe -hdevice SVG -printfile ${dname}.${rank}d.${problem}.${p2}.svg
    echo "${dname}.${rank}d.${problem}.${p2}.svg"
done

else # $type = accuracy

plotflags="--no-dups --plot-worst --accuracy"

test `egrep "$pat" $data | perl $sd/grep-rank.pl $rank | wc -l` -gt 1 || exit 0

(sh $sd/plot-title.sh $problem $rank;
egrep "$pat" $data |
    perl $sd/grep-rank.pl $rank |
    perl $sd/grace-plot.pl $plotflags) |
    gracebat -pipe -hdevice SVG -printfile ${dname}.${rank}d.${problem}.svg
echo "${dname}.${rank}d.${problem}.svg"

(sh $sd/plot-title.sh $problem $rank;
 echo "@subtitle \"enlargement of most accurate FFTs\"";
 egrep "$pat" $data |
    perl $sd/grep-rank.pl $rank |
    perl $sd/grace-plot.pl $plotflags --accurate-only) |
    gracebat -pipe -hdevice SVG -printfile ${dname}.${rank}d.${problem}.acc.svg
echo "${dname}.${rank}d.${problem}.acc.svg"

(sh $sd/plot-title.sh $problem $rank;
 echo "@subtitle \"enlargement of most accurate FFTs, powers of two only\"";
 egrep "$pat" $data |
    perl $sd/grep-rank.pl $rank | perl $sd/grep-p2.pl |
    perl $sd/grace-plot.pl $plotflags --accurate-only) |
    gracebat -pipe -hdevice SVG -printfile ${dname}.${rank}d.${problem}.acc.p2.svg
echo "${dname}.${rank}d.${problem}.acc.p2.svg"

fi
