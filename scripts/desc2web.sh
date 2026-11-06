#! /bin/sh
# Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

# benchFFT's desc2web.sh for scripts/collect's documents, as fftw.org/speed: <pages>/<machine>.html for each machine
# (description, compilers and flags, verdicts, the codes no rival beat at each size, the plots, the raw data) and
# <pages>/index.html (method, one row per machine for each kind of measurement, info2html.scm's FFT list)
# usage: sh scripts/desc2web.sh <documents dir> <pages dir> [run url]   (the pages dir beside the documents dir)

sd=`dirname $0`
case $sd in
    /*) ;;
    *) sd=`pwd`/$sd ;;
esac
docs=`cd "$1" && pwd`
mkdir -p "$2"
pages=`cd "$2" && pwd`
run=${3-}
link=../`basename "$docs"` # the plots and raw data stay where collect left them

esc() { sed 's/&/\&amp;/g; s/</\&lt;/g; s/>/\&gt;/g'; }
head_() { printf '<!DOCTYPE html>\n<html>\n<head>\n<meta charset="utf-8">\n<title>%s</title>\n</head>\n<body text="#000000" bgcolor="#ffffff">\n' "$1"; }

# from a machine's .sysinfo and .info
cpu() { sed -n '/^== cpu/,/^== /{s/^Model name: *//p; s/^machdep.cpu.brand_string: *//p; s/^model name[[:space:]]*: *//p; s/^isa[[:space:]]*: *//p}' "$1" | head -1; }
emu() { sed -n 's/^emulator: *//p' "$1" | head -1; }
cc() { grep "(cc" "$1" | head -1 | sed 's/^(cc *"//;s/")$//'; }
cxx() { grep "(cxx" "$1" | head -1 | sed 's/^(cxx *"//;s/")$//'; }
kind() { # what the numbers are
	if [ -s "$1.speed" ]; then echo timed; elif [ -s "$1.counts.speed" ]; then echo counts; elif [ -s "$1.cycles.speed" ]; then echo cycles; fi
}
family() {
	case $1 in
		*x64*) echo "1 x86-64" ;; *i686*) echo "2 x86" ;; *arm64*) echo "3 ARM64" ;; *armhf* | *armv6* | *armel*) echo "4 32-bit ARM" ;;
		*riscv64*) echo "5 RISC-V" ;; cortex-m*) echo "6 Cortex-M" ;; esp32*) echo "7 Xtensa (ESP32)" ;; *) echo "8 other" ;;
	esac
}
checks() {
	if [ ! -f "$1" ]; then echo "not recorded"; elif tail -1 "$1" | grep -q '^exit 0$'; then echo "passed"; else echo "failed"; fi
}

# a .fastest's sizes by class: "class sizes code count", codes most unbeaten first
classes() {
	awk '/^srof/ {
		n = $2 + 0; x = n; while (x % 2 == 0) x /= 2
		c = (x == 1) ? 1 : (n % 32 == 0 ? 2 : 3)
		sizes[c]++
		for (i = 3; i <= NF && $i != "|"; i++) k[c " " $i]++
	}
	END { for (e in k) { split(e, a, " "); print a[1], sizes[a[1]], a[2], k[e] } }' "$1" | sort -k1,1n -k4,4nr -k3,3
}
CLASS1="powers of two"
CLASS2="other multiples of 32 (PFFFT's sizes)"
CLASS3="the rest"
summary() { # one class's line: "ipp 6, zldsp 4, ..." over its sizes
	classes "$1" | awk -v c="$2" -v max="${3-99}" '$1 == c && shown < max {printf "%s%s %d", (shown ? ", " : ""), $3, $4; shown++; n = $2} END {if (n) printf " of %d", n}'
}
ours() { # PUREfft's verdict line, at its sizes (multiples of 32)
	awk '/^srof/ && $2 % 32 == 0 {n++; if (index($0, "| PUREfft beaten by ")) {lost++; split($0, a, "beaten by "); m = split(a[2], b, " "); for (i = 1; i <= m; i++) by[b[i]]++}}
	END {if (!n) exit; if (!lost) {printf "unbeaten at all %d sizes", n; exit} printf "beaten at %d of %d, by", lost, n; for (c in by) printf " %s %d", c, by[c]}' "$1"
}

for d in "$docs"/*/; do
	m=`basename "$d"`
	f=$d$m
	[ -f "$f.sysinfo" ] || continue
	label=${m%%_*}
	c=`cpu "$f.sysinfo"`
	e=`emu "$f.sysinfo"`
	[ -n "$c" ] || c=`echo "$e" | sed -n 's/.*-cpu \([^ ]*\).*/QEMU \1/p; s/.*-M \([^ ]*\).*/QEMU \1/p' | head -1`
	[ -n "$c" ] || c="QEMU's default CPU"
	title="$c, $label, `cc "$f.info"`"
	page=$pages/$m.html
	{
		head_ "`echo "$title" | esc`"
		echo "<h1 align=\"center\">`echo "$title" | esc`</h1>"
		echo "<p>"
		echo "$c" | esc
		sed -n 's/^\(L[123]d\{0,1\} cache\): *\(.*\)/, \1 \2/p' "$f.sysinfo" | tr -d '\n' | esc
		sed -n 's/^CPU(s): *\(.*\)/, \1 CPUs/p' "$f.sysinfo" | head -1 | esc
		sed -n 's/^Hypervisor vendor: *\(.*\)/, a VM on \1/p' "$f.sysinfo" | esc
		echo "."
		sed -n '/^== uname/{n;p;}' "$f.sysinfo" | esc
		sed -n 's/^image: *\(..*\)/<br>Runner image: \1/p' "$f.sysinfo"
		[ -z "$e" ] || echo "<br>Emulated: <code>`echo "$e" | esc`</code>"
		echo "<br>Rounds: `sed -n '/^== rounds/{n;p;}' "$f.sysinfo"`."
		[ "`kind "$f"`" = timed ] && echo " Only the single-CPU performance was benchmarked."
		echo "</p>"
		echo "<p>Compilers and flags (unless overridden):</p> <ul>"
		echo "<li>C: <code>`cc "$f.info" | esc`</code> `sed -n 's/^CMAKE_C_FLAGS\(_RELEASE\)\{0,1\}:STRING=\(..*\)/\2/p' "$f.sysinfo" | tr '\n' ' ' | esc`"
		echo "<li>C++: <code>`cxx "$f.info" | esc`</code> `sed -n 's/^CMAKE_CXX_FLAGS\(_RELEASE\)\{0,1\}:STRING=\(..*\)/\2/p' "$f.sysinfo" | tr '\n' ' ' | esc`"
		sed -n 's/^set(\([A-Z0-9]*\)_\(VERSION\|RELEASE\|COMMIT\) \([^)]*\)).*/<li>\1 \3/p' "$f.sysinfo" | esc | sed 's/&lt;li&gt;/<li>/'
		sed -n '/^== commits of/{n;s/^\([0-9a-f]\{12\}\).*/<li>PUREfft commit \1/p;q;}' "$f.sysinfo"
		echo "</ul>"
		echo "<p>Checks: `checks "$f.ctest"`. PUREfft: `ours "$f.fastest" | esc`.</p>"
		[ -s "$f.verdicts" ] && { echo "<pre>"; esc < "$f.verdicts"; echo "</pre>"; }
		if [ -f "$f.fastest" ]; then
			echo "<p>`head -1 "$f.fastest" | sed 's/^# *//' | esc`:</p> <ul>"
			for k in 1 2 3; do
				s=`summary "$f.fastest" $k`
				eval "name=\$CLASS$k"
				[ -n "$s" ] && echo "<li>$name: $s"
			done
			echo "</ul>"
			echo '<table border="1" cellpadding="3"><tr><th>n</th><th>no rival beat these, fastest first</th><th>PUREfft beaten by</th></tr>'
			sed -n 's/^srof \([0-9]*\): \([^|]*\)\(| PUREfft beaten by \(.*\)\)\{0,1\}$/<tr><td align="right">\1<\/td><td>\2<\/td><td>\4<\/td><\/tr>/p' "$f.fastest"
			echo "</table>"
		fi
		echo "<p>Raw data files:"
		for x in speed busy.speed counts.speed cycles.speed accuracy tbl busy.tbl counts.tbl cycles.tbl info sysinfo ctest verdicts fastest measures; do
			[ -f "$f.$x" ] && echo "<a href=\"$link/$m/$m.$x\">$m.$x</a>"
		done
		echo "</p>"
		echo "<hr>"
		for x in 1d.srxx.p2 1d.srxx.np2 busy.1d.srxx.p2 busy.1d.srxx.np2 counts.1d.srxx.p2 counts.1d.srxx.np2 cycles.1d.srxx.p2 cycles.1d.srxx.np2 \
			1d.srxx 1d.srxx.acc 1d.srxx.acc.p2 simd-real-pow2-4-rel-pffft simd-real-non2-4-rel-pffft; do
			[ -f "$f.$x.png" ] && echo "<p align=\"center\"><img src=\"$link/$m/$m.$x.png\" alt=\"$m.$x\">"
		done
		printf '</body>\n</html>\n'
	} > "$page"
	echo "$m.html"
done

# the index: method, then a table a kind, rows by ISA family
row() { # <m>
	f=$docs/$1/$1
	c=`cpu "$f.sysinfo"`
	[ -n "$c" ] || c=`emu "$f.sysinfo" | sed -n 's/.*-cpu \([^ ]*\).*/QEMU \1/p; s/.*-M \([^ ]*\).*/QEMU \1/p' | head -1`
	printf '<tr><td><a href="%s.html">%s</a></td><td>%s</td><td>%s</td><td>%s</td>' "$1" "${1%%_*}" "`echo "${c:-QEMU} " | esc`" "`cc "$f.info" | esc`" "`checks "$f.ctest"`"
	for k in 1 2 3; do printf '<td>%s</td>' "`summary "$f.fastest" $k 3`"; done
	printf '<td>%s</td></tr>\n' "`ours "$f.fastest" | esc`"
}
table() { # <kind> <heading>
	ms=`for d in "$docs"/*/; do m=\`basename "$d"\`; [ "\`kind "$d$m"\`" = "$1" ] && echo "\`family "$m" | cut -c1\` $m"; done | sort | cut -d' ' -f2`
	[ -n "$ms" ] || return 0
	echo "<h2>$2</h2>"
	echo "<table border=\"1\" cellpadding=\"3\"><tr><th>machine</th><th>CPU</th><th>compiler</th><th>checks</th><th>powers of two</th><th>other multiples of 32</th><th>the rest</th><th>PUREfft</th></tr>"
	group=""
	for m in $ms; do
		g=`family "$m" | cut -d' ' -f2-`
		[ "$g" = "$group" ] || echo "<tr><th colspan=\"8\" align=\"left\">$g</th></tr>"
		group=$g
		row "$m"
	done
	echo "</table>"
}
{
	head_ "PUREfft benchmark results"
	echo '<h1 align="center">PUREfft benchmark results</h1>'
	echo "<p>`date -u +%Y-%m-%d`${run:+, <a href=\"$run\">the CI run</a>}.</p>"
	cat <<EOF
<h2>Method</h2>
<ul>
<li>srof: single-precision, real-data, out-of-place, forward, 1d, at every 2^a 3^b 5^c, a &gt;= 1, from 64 to 16384 (149 sizes).
<li>mflops = 2.5 N log2 N / t[µs], as benchFFT: not an actual flop count, but a scaling that compares sizes on one graph.
<li>Timed: FFTW's libbench2, one process a code, size and round, 5 rounds in alternating order, on one core; the plots show the median round. Busy: timed after 256 KB of other writes.
<li>QEMU instruction counts: (N(3) - N(1)) / 2 instructions a transform, at 1 instruction = 1 ns. A board's cycle counts: the same, from its cycle counter.
<li>No rival beat a code if none was faster in all rounds but one by more than the floor, both conditions; the floor is the larger of 3% and the widest pffft/pffft_twin spread; counts are one round with 15%.
<li>Accuracy: libbench2's relative rms error, as <a href="https://www.fftw.org/accuracy/method.html">benchFFT's</a>.
</ul>
EOF
	table timed "Timed on real CPUs"
	table counts "QEMU instruction counts"
	table cycles "Cycle counts on real boards"
	echo "<h2>The FFTs</h2>"
	echo "<p>Not every FFT is benchmarked on every machine.</p>"
	if command -v guile > /dev/null; then
		cat "$docs"/*/*.info | guile -s "$sd/info2html.scm"
	else
		echo "<p>(benchFFT's info2html.scm needs guile)</p>"
	fi
	cat <<EOF
<h2>Caveats</h2>
<ul>
<li>Counts measure work, not time: QEMU doesn't emulate how long an instruction takes.
<li>The macOS and Windows ARM64 runners are VMs; CI runners are shared, which the paired rounds and the floor answer.
<li>Counts, cycles and times don't compare across tables, nor machines across rows.
</ul>
</body>
</html>
EOF
} > "$pages/index.html"
echo "index.html"
