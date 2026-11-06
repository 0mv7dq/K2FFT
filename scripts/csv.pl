#! /usr/bin/perl -w

# a .speed file as one of PFFFT's benchmark CSVs (bench_pffft.c), for its plots.sh:
# type 4, each code's time relative to pffft's, or 6, mflops; nan where a code can't do the size
# usage: perl csv.pl 4|6 pow2|non2 < x.speed > x.simd-real-pow2-4-rel-pffft.csv

my ($type, $part) = @ARGV;
my %t = ();
my %m = ();
my %codes = ();
my %sizes = ();
while (<STDIN>) {
  chomp;
  my ($nam, $prob, $siz, $mflops, $tim) = split / /;
  next if !defined($tim) || $mflops eq "FAILED";
  next if ($part eq "pow2") != (($siz & ($siz - 1)) == 0);
  $t{$nam}{$siz} = $tim;
  $m{$nam}{$siz} = $mflops;
  $codes{$nam} = 1;
  $sizes{$siz} = 1;
}

my @c = sort keys %codes;
print "size, log2, ", join("", map { "$_, " } @c), "\n";
for my $n (sort { $a <=> $b } keys %sizes) {
  next if $type == 4 && !$t{"pffft"}{$n};
  printf("%d, %.3f, ", $n, log($n) / log(2));
  for my $c (@c) {
    if (!defined($t{$c}{$n})) {
      print "nan, ";
    } elsif ($type == 4) {
      printf("%f, ", $t{$c}{$n} / $t{"pffft"}{$n});
    } else {
      printf("%f, ", $m{$c}{$n});
    }
  }
  print "\n";
}
