#! /bin/sh
# Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

# benchFFT's desc2web.sh for scripts/collect's documents, as fftw.org/speed: <pages>/<machine>.html for each machine
# (the plots, description, compilers and flags, verdicts, the raw data) and <pages>/index.html (method, each machine's
# plot for each kind of measurement, info2html.scm's FFT list)
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
css=':root{color-scheme:light dark;--bg:#fff;--fg:#000;--line:#888;--link:#0645ad;--seen:#663399;--tip:#fff}
@media (prefers-color-scheme:dark){:root{--bg:#1c1c1e;--fg:#d4d4d4;--line:#5c5c5c;--link:#8ab4f8;--seen:#c58af9;--tip:#28282b}}
body{margin:8px 12px;background:var(--bg);color:var(--fg)}a{color:var(--link)}a:visited{color:var(--seen)}
h1{font-size:1.5em;margin:.2em 0 .4em}h2{font-size:1.25em;margin:.6em 0 .3em}h3{margin:.4em 0 .2em}
p,ul,pre{margin:.3em 0}ul{padding-left:1.4em}pre{white-space:pre-wrap}figure{margin:0;text-align:center}
figure{max-width:100%}figcaption{font-size:.9em;margin:.1em 0 .4em}img{max-width:100%;height:auto}
.row{display:flex;flex-wrap:wrap;justify-content:center;align-items:flex-start;gap:4px 16px}.row>div{flex:1 1 30em}
ul:has(>li>h2){columns:22em;column-gap:2em}li:has(>h2){break-inside:avoid}li>h2{font-size:1.05em;margin:.2em 0 0}a{overflow-wrap:anywhere}
.plot{position:relative;display:inline-block;max-width:100%}.plot img,.plot picture{display:block}
.rule{position:absolute;border-left:1px dashed var(--fg);opacity:.6;pointer-events:none}
.tip{position:absolute;z-index:1;pointer-events:none;background:var(--tip);color:var(--fg);border:1px solid var(--line);box-shadow:0 1px 4px #0004;padding:3px 6px;font:12px/1.4 sans-serif;text-align:left;white-space:nowrap}
.tip div{display:flex;align-items:center}.tip i{width:9px;height:9px;margin-right:5px}.tip span{margin-left:auto;padding-left:12px}.tip .k{font-weight:bold}'
js=`cat <<'EOF'
addEventListener("DOMContentLoaded", () => { // a panel's nearest size: each library's value there, best first
	for (const p of document.querySelectorAll(".plot")) {
		const d = JSON.parse(p.querySelector("script").textContent), img = p.querySelector("img");
		const tip = p.querySelector(".tip"), rule = p.querySelector(".rule");
		p.onmouseleave = () => { tip.hidden = rule.hidden = true; };
		p.onmousemove = e => {
			const r = img.getBoundingClientRect(), o = p.getBoundingClientRect();
			const x = (e.clientX - r.left) / r.width, y = (e.clientY - r.top) / r.height;
			const q = d.panels.find(q => x >= q.box[0] && x <= q.box[2] && y >= q.box[1] && y <= q.box[3]);
			if (!q) return p.onmouseleave();
			let i = 0;
			q.x.forEach((v, j) => { if (Math.abs(v - x) < Math.abs(q.x[i] - x)) i = j; });
			const up = q.unit == "GFLOPS", f = v => q.unit == "rms error" ? v.toExponential(2) : +v.toPrecision(3);
			const s = q.series.filter(s => s.v[i] != null).sort((a, b) => up ? b.v[i] - a.v[i] : a.v[i] - b.v[i]);
			const dark = matchMedia("(prefers-color-scheme: dark)").matches; // the .dark.png's colours
			tip.innerHTML = "<b>N = " + q.sizes[i] + "</b> (" + q.unit + ")" + s.map(s => "<div" + (s.name == "K2FFT" ? ' class="k"' : "") +
				'><i style="background:' + (dark ? s.dark : s.color) + '"></i>' + s.name + "<span>" + f(s.v[i]) + "</span></div>").join("");
			tip.hidden = rule.hidden = false;
			Object.assign(rule.style, {left: r.left - o.left + q.x[i] * r.width + "px", top: r.top - o.top + q.box[1] * r.height + "px",
				height: (q.box[3] - q.box[1]) * r.height + "px"});
			const cx = e.clientX - o.left, cy = e.clientY - o.top;
			tip.style.left = (cx + 14 + tip.offsetWidth > o.width ? cx - 14 - tip.offsetWidth : cx + 14) + "px";
			tip.style.top = Math.max(0, Math.min(cy + 14, o.height - tip.offsetHeight)) + "px";
		};
	}
});
EOF`
head_() { printf '<!DOCTYPE html>\n<html>\n<head>\n<meta charset="utf-8">\n<meta name="color-scheme" content="light dark">\n<title>%s</title>\n<style>\n%s\n</style>\n<script>\n%s\n</script>\n</head>\n<body>\n' "$1" "$css" "$js"; }
fig() { # <png> <alt> <caption> [link]: numbered, at half its pixels, as scripts/figure draws them at twice the size; its .dark.png on dark pages, its .json hovered
	[ -f "$1" ] || return 0
	n=$((n+1))
	w=`od -An -tu1 -j16 -N4 "$1" | awk '{print int(($1 * 16777216 + $2 * 65536 + $3 * 256 + $4) / 2)}'`
	s=$link/${1#$docs/}
	i="<img src=\"$s\" alt=\"$2\" width=\"$w\">"
	[ ! -f "${1%.png}.dark.png" ] || i="<picture><source media=\"(prefers-color-scheme: dark)\" srcset=\"${s%.png}.dark.png\">$i</picture>"
	[ -z "${4-}" ] || i="<a href=\"$4\">$i</a>"
	[ ! -f "${1%.png}.json" ] || i="<div class=\"plot\">$i<div class=\"rule\" hidden></div><div class=\"tip\" hidden></div><script type=\"application/json\">`cat "${1%.png}.json"`</script></div>"
	echo "<figure style=\"width:${w}px\">$i<figcaption>Figure $n: $3</figcaption></figure>"
}

# from a machine's .sysinfo and .info
cpu() { sed -n '/^== cpu/,/^== /{s/^Model name: *//p; s/^machdep.cpu.brand_string: *//p; s/^model name[[:space:]]*: *//p; s/^isa[[:space:]]*: *//p}' "$1" | head -1; }
emu() { sed -n 's/^emulator: *//p' "$1" | head -1; }
machine() { # the CPU measured: the emulated one, else the host's
	q=`emu "$1"`
	[ -n "$q" ] || { cpu "$1"; return; }
	q=`echo "$q" | sed -n 's/.*-cpu \([^ ]*\).*/QEMU \1/p; s/.*-M \([^ ]*\).*/QEMU \1/p' | head -1`
	echo "${q:-QEMU's default CPU}"
}
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
gfft() { # a class in GFFT's words: srof single precision R2C
	echo "$1" | awk '{b = substr($1, 4, 1) == "b"; k = substr($1, 2, 1)
		t = k == "r" ? (b ? "C2R" : "R2C") : (k == "c" ? (b ? "inverse C2C" : "C2C") : "R2R")
		print (substr($1, 1, 1) == "d" ? "double" : "single") " precision " t (substr($1, 3, 1) == "i" ? " in-place" : "")}'
}
caption() { # <machine> [class]: a speed figure's, as GFFT's
	case `kind "$1"` in timed) k="" ;; counts) k=", QEMU's instruction counts at 1 ns each" ;; *) k=", the board's cycle counts at 1 ns each" ;; esac
	c=`cc "$1.info" | esc`
	echo "Performance of `gfft ${2-srof}` FFT on `machine "$1.sysinfo" | esc` (`basename "$1" | sed 's/_.*//'`${c:+, $c}$k)."
}
setup() { echo "Evaluations of `gfft ${2-srof}` FFT setup time on `machine "$1.sysinfo" | esc`. The dotted gray line indicates the unit of second."; }
accuracy() { echo "Evaluations of `gfft ${2-srof}` FFT accuracy (libbench2's rms error) on `machine "$1.sysinfo" | esc`."; }
libs() { # a .sysinfo's library versions and K2FFT's commit, commits cut to 12
	{ sed -n 's/^set(\([A-Z0-9]*\)_\(VERSION\|RELEASE\|COMMIT\) \([^)]*\)).*/\1 \3/p' "$1"; sed -n '/^== commits of/{n;s/^\([0-9a-f]\{12\}\).*/K2FFT commit \1/p;q;}' "$1"; } |
		sed 's/ \([0-9a-f]\{12\}\)[0-9a-f]\{28\}$/ \1/' | esc | awk '{printf "%s%s", (NR > 1 ? ", " : ""), $0}'
}
describe() { # a class's words, as benchFFT's: srof, single-precision, real-data, out-of-place, forward
	echo "$1" | awk '{
		d = (substr($1, 1, 1) == "d" ? "double" : "single") "-precision"
		k = substr($1, 2, 1); d = d ", " (k == "c" ? "complex" : (k == "k" ? "real-to-real" : "real")) "-data"
		d = d ", " (substr($1, 3, 1) == "i" ? "in-place" : "out-of-place")
		w = substr($1, 4, 1); if (w == "f") d = d ", forward"; else if (w == "b") d = d ", backward"
		print d }'
}
ours() { # K2FFT's verdict line for class $2 (srof), at its sizes (multiples of 32)
	awk -v k="${2-srof}" '$1 == k && $2 % 32 == 0 {n++; if (index($0, "| K2FFT beaten by ")) {lost++; split($0, a, "beaten by "); m = split(a[2], b, " "); for (i = 1; i <= m; i++) by[b[i]]++}}
	END {if (!n) exit; if (!lost) {printf "unbeaten at all %d sizes", n; exit} printf "beaten at %d of %d, by", lost, n; for (c in by) printf " %s %d", c, by[c]}' "$1"
}

for d in "$docs"/*/; do
	m=`basename "$d"`
	f=$d$m
	[ -f "$f.sysinfo" ] || continue
	label=${m%%_*}
	c=`machine "$f.sysinfo"`
	e=`emu "$f.sysinfo"`
	k=`cc "$f.info"`
	title="$c, $label${k:+, $k}" # a board records no compiler
	page=$pages/$m.html
	{
		n=0
		head_ "`echo "$title" | esc`"
		echo "<h1 align=\"center\">`echo "$title" | esc`</h1>"
		echo '<div class="row">' # the figures, then the text in what room they leave
		fig "$f.png" "$m: GFLOPS against N" "`caption "$f"`"
		fig "$f.setup.png" "$m: setup time against N" "`setup "$f"`"
		fig "$f.accuracy.png" "$m: rms error against N" "`accuracy "$f"`"
		echo '<div>'
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
		[ -z "$k" ] || echo "<p>Compilers and flags (unless overridden): C <code>`echo "$k" | esc`</code> `sed -n 's/^CMAKE_C_FLAGS\(_RELEASE\)\{0,1\}:STRING=\(..*\)/\2/p' "$f.sysinfo" | tr '\n' ' ' | esc`;
C++ <code>`cxx "$f.info" | esc`</code> `sed -n 's/^CMAKE_CXX_FLAGS\(_RELEASE\)\{0,1\}:STRING=\(..*\)/\2/p' "$f.sysinfo" | tr '\n' ' ' | esc`.
<br>Libraries: `libs "$f.sysinfo"`.</p>"
		echo "<p>Raw data files:"
		for x in speed busy.speed counts.speed cycles.speed accuracy tbl busy.tbl counts.tbl cycles.tbl info sysinfo ctest verdicts fastest measures; do
			[ -f "$f.$x" ] && echo "<a href=\"$link/$m/$m.$x\">$m.$x</a>"
		done
		echo "</p>"
		echo '</div><div>'
		echo "<p>Checks: `checks "$f.ctest"`. K2FFT: `ours "$f.fastest" | esc`.</p>"
		[ -s "$f.verdicts" ] && { echo "<pre>"; esc < "$f.verdicts"; echo "</pre>"; }
		if [ -f "$f.fastest" ]; then
			echo "<p>`head -1 "$f.fastest" | sed 's/^# *//' | esc`:</p> <ul>"
			for k in 1 2 3; do
				s=`summary "$f.fastest" $k`
				eval "name=\$CLASS$k"
				[ -n "$s" ] && echo "<li>$name: $s"
			done
			echo "</ul>"
		fi
		echo '</div></div>' # each size's verdict: the .fastest; its speeds: the plots' hover
		for k in `awk '/^[a-z]+ / && $1 != "srof" && !seen[$1]++ {print $1}' "$f.fastest" 2> /dev/null`; do # the other classes
			echo "<h2>$k: `describe $k`</h2>"
			echo '<div class="row">'
			fig "$f.$k.png" "$m $k: GFLOPS against N" "`caption "$f" $k`"
			fig "$f.$k.setup.png" "$m $k: setup time against N" "`setup "$f" $k`"
			fig "$f.$k.accuracy.png" "$m $k: rms error against N" "`accuracy "$f" $k`"
			echo '</div>'
			echo "<p>K2FFT: `ours "$f.fastest" $k | esc`.</p>"
		done
		printf '</body>\n</html>\n'
	} > "$page"
	echo "$m.html"
done

# the index: method, the figures a kind of measurement, rows by ISA family, the FFTs
{
	head_ "K2FFT benchmark results"
	echo '<h1 align="center">K2FFT benchmark results</h1>'
	echo "<p>`date -u +%Y-%m-%d`${run:+, <a href=\"$run\">the CI run</a>}.</p>"
	cat <<EOF
<h2>Method</h2>
<ul>
<li>srof: single-precision, real-data, out-of-place, forward, 1d, at every 2^a 3^b 5^c, a &gt;= 1, from 32 to the largest size K2FFT takes there (65536; 16384 on Cortex-M, 4096 on ESP32): every size it takes, its multiples of 32, and the others between.
<li>mflops = 2.5 N log2 N / t[µs], as benchFFT: not an actual flop count, but a scaling that compares sizes on one graph.
<li>Timed: FFTW's libbench2, one process a code, size and round, 5 rounds in alternating order, on one core; the plots show the median round. Busy: timed after 256 KB of other writes.
<li>QEMU instruction counts: (N(3) - N(1)) / 2 instructions a transform, at 1 instruction = 1 ns. A board's cycle counts: the same, from its cycle counter.
<li>No rival beat a code if none was faster in all rounds but one by more than the floor, both conditions; the floor is the larger of 3% and the widest pffft/pffft_twin spread; counts are one round with 15%.
<li>Accuracy: libbench2's relative rms error, as <a href="https://www.fftw.org/accuracy/method.html">benchFFT's</a>. Setup time: libbench2's, planning included.
<li>Plots as GFFT's (ICPP 2023) figures 10-14: K2FFT against FFTW, the vendor's library and the libraries no rival beat at some size. "vs FFTW 1.17x AVG.": K2FFT's speed over FFTW's, geometric mean over the panel's sizes; "fastest": the fastest rival at each size, K2FFT's own kernel (hwyfft) left out.
</ul>
EOF
	echo "<h2>Results</h2>"
	n=0
	for k in timed counts cycles; do # by kind, as the summary, so the two-panel figures pair up
		ms=`for d in "$docs"/*/; do m=\`basename "$d"\`; [ "\`kind "$d$m"\`" = $k ] && [ -f "$d$m.png" ] && echo "\`family "$m" | cut -c1\` $m"; done | sort | cut -d' ' -f2`
		[ -n "$ms" ] || continue
		case $k in timed) echo "<h3>Timed on real CPUs</h3>" ;; counts) echo "<h3>QEMU instruction counts</h3>" ;; *) echo "<h3>Cycle counts on real boards</h3>" ;; esac
		echo '<div class="row">'
		for m in $ms; do fig "$docs/$m/$m.png" "$m" "`caption "$docs/$m/$m"`" "$m.html"; done
		echo '</div>'
	done
	echo "<h2>The FFTs</h2>"
	echo "<p>Not every FFT is benchmarked on every machine.</p>"
	if command -v guile > /dev/null; then
		cat "$docs"/*/*.info | grep '^[()]' | guile -s "$sd/info2html.scm" # an info's stray build output breaks it
	else
		echo "<p>(benchFFT's info2html.scm needs guile)</p>"
	fi
	cat <<EOF
<h2>Caveats</h2>
<ul>
<li>Counts measure work, not time: QEMU doesn't emulate how long an instruction takes.
<li>The macOS and Windows ARM64 runners are VMs; CI runners are shared, which the paired rounds and the floor answer.
<li>Counts, cycles and times don't compare with each other, nor one machine's with another's.
<li>oneMKL and IPP pick their code by CPU, and GitHub's x86-64 runners are mostly AMD EPYCs: the papers' oneMKL figures are from Intel Xeons, their AOCL-FFTW figures from an EPYC.
</ul>
</body>
</html>
EOF
} > "$pages/index.html"
echo "index.html"
