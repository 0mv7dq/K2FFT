#! /usr/bin/perl -w

# the codes no rival beats at each problem (class and tensor), over paired rounds of benchFFT's speed lines (the k-th of a
# code and problem is round k): a beats b if faster in all rounds but one (all, under three) by a median margin over the floor, in every file;
# the floor is the larger of -f and the widest pffft/pffft_twin margin; K2FFT isn't a code, its line is checked against them
# usage: perl fastest.pl [-f floor] [-c old.fastest] <rounds> [<busy rounds>]
# stdout: "<class> <tensor>: <codes, fastest first>[ | K2FFT beaten by <codes>]", as "srof 1024: ..."; stderr: regression, beaten,
# changed and new against old.fastest, and integrity; exits 1 on a regression or an integrity line

use strict;
use Getopt::Std;
use constant NEVER => 9e99; # a FAILED round's time

my %opt = (f => 0.03, c => "");
getopts("f:c:", \%opt) or die "usage: perl fastest.pl [-f floor] [-c old.fastest] <rounds> [<busy rounds>]\n";
my $floor = $opt{f};
my $old = $opt{c};
my $target = "K2FFT";
my @files = @ARGV;

my @t = (); # $t[file]{code}{"class tensor"}: times in round order, undef for FAILED
my %sizes = (); # "class tensor"
my %classes = (); # class: its first line's order
my %codes = ();
for my $i (0..$#files) {
  open(my $fh, "<", $files[$i]) or die "$files[$i]: $!\n";
  while (<$fh>) {
    chomp;
    my ($nam, $prob, $siz, $mflops, $tim) = split / /;
    next unless defined($tim);
    my $n = "$prob $siz";
    push(@{$t[$i]{$nam}{$n}}, ($mflops eq "FAILED") ? undef : $tim);
    $sizes{$n} = 1;
    $classes{$prob} = scalar(keys %classes) unless exists($classes{$prob});
    $codes{$nam} = 1;
  }
  close($fh);
}

# by class, as the lines first name them, then by the tensor's first size
sub by_problem {
  my ($ca, $ta) = split(/ /, $a);
  my ($cb, $tb) = split(/ /, $b);
  my ($na) = $ta =~ /^(\d+)/;
  my ($nb) = $tb =~ /^(\d+)/;
  return $classes{$ca} <=> $classes{$cb} || ($na // 0) <=> ($nb // 0) || $ta cmp $tb;
}

# libbench2's report.c: t[n / 2] of the sorted times
sub median {
  my @v = sort { $a <=> $b } @_;
  return $v[@v / 2];
}

# over the rounds codes u and v both ran in file i: the median of v's time over u's, u's wins, the rounds
sub pair {
  my ($i, $u, $v, $n) = @_;
  my $tu = $t[$i]{$u}{$n};
  my $tv = $t[$i]{$v}{$n};
  return (undef, 0, 0) unless $tu && $tv;
  my $r = (@$tu < @$tv) ? scalar(@$tu) : scalar(@$tv);
  my @q = ();
  my $wins = 0;
  for my $k (0..$r-1) {
    my ($x, $y) = ($tu->[$k], $tv->[$k]);
    next unless defined($x) || defined($y);
    my $q = !defined($y) ? NEVER : !defined($x) ? 0 : ($x > 0 ? $y / $x : 1);
    push(@q, $q);
    $wins++ if $q > 1;
  }
  return (@q ? median(@q) : undef, $wins, scalar(@q));
}

my @fl = ();
for my $i (0..$#files) {
  $fl[$i] = $floor;
  next unless $t[$i]{"pffft"} && $t[$i]{"pffft_twin"};
  for my $n (keys %sizes) {
    my ($q) = pair($i, "pffft", "pffft_twin", $n);
    next unless defined($q) && $q > 0 && $q < NEVER;
    my $m = ($q > 1) ? $q - 1 : 1 / $q - 1;
    $fl[$i] = $m if $m > $fl[$i];
  }
}

sub beats {
  my ($u, $v, $n) = @_;
  for my $i (0..$#files) {
    my ($q, $wins, $r) = pair($i, $u, $v, $n);
    return 0 unless defined($q) && $r > 0;
    return 0 unless $wins >= ($r >= 3 ? $r - 1 : $r) && $q > 1 + $fl[$i];
  }
  return 1;
}

sub ours {
  my $c = shift;
  return $c eq $target;
}

sub time_at {
  my ($c, $n) = @_;
  my @v = grep { defined($_) } @{$t[0]{$c}{$n}};
  return @v ? median(@v) : NEVER;
}

my %was = (); # "class tensor": [codes, beaten by]
if ($old ne "" && open(my $fh, "<", $old)) {
  while (<$fh>) {
    next unless /^(\S+ \S+): ([^|]*?)\s*(?:\| \S+ beaten by (.*))?$/;
    $was{$1} = [$2, defined($3) ? $3 : ""];
  }
  close($fh);
}

my $bad = 0;
printf("# codes no rival beat in all rounds but one by more than the floor%s; floors %s\n",
  (@files > 1) ? ", in each condition" : "", join(" ", map { sprintf("%.1f%%", 100 * $_) } @fl));
for my $n (sort by_problem keys %sizes) {
  my @c = grep { $_ ne "pffft_twin" && $t[0]{$_}{$n} } sort keys %codes;
  next unless @c;
  my @libs = grep { !ours($_) } @c;
  my @best = ();
  for my $lib (@libs) {
    push(@best, $lib) unless grep { $_ ne $lib && beats($_, $lib, $n) } @libs;
  }
  @best = sort { time_at($a, $n) <=> time_at($b, $n) } @best;
  my @by = (grep { $_ eq $target } @c) ? grep { beats($_, $target, $n) } @libs : ();
  my $line = "$n: @best";
  $line .= " | $target beaten by @by" if @by;
  print "$line\n";

  my $r = 0;
  for my $x (@c) {
    my $k = scalar(@{$t[0]{$x}{$n}});
    $r = $k if $k > $r;
  }
  for my $x (@c) {
    for my $i (0..$#files) {
      my $k = $t[$i]{$x}{$n} ? scalar(grep { defined($_) } @{$t[$i]{$x}{$n}}) : 0;
      if ($k < $r) {
        print STDERR "integrity: $x $n has $k of $r rounds in $files[$i]\n";
        $bad = 1;
      }
    }
  }
  next if $old eq "";
  if (!exists($was{$n})) {
    print STDERR "new: $n: @best\n";
    next;
  }
  my ($oldbest, $oldby) = @{$was{$n}};
  if (@by && $oldby eq "") {
    print STDERR "regression: $n: $target beaten by @by\n";
    $bad = 1;
  } elsif (@by) {
    print STDERR "beaten: $n: $target beaten by @by\n";
  }
  my %o = map { $_ => 1 } split(/ /, $oldbest);
  print STDERR "changed: $n: $oldbest -> @best\n" unless grep { $o{$_} } @best;
}
exit($bad);
