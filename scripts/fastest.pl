#! /usr/bin/perl -w

# the codes no rival beats at each size, from paired rounds (the k-th line of a code and size is round k):
# a beats b if a was faster in all rounds but one (all, under three) by a median margin above the floor,
# in every file given; the floor is -f, or the largest margin pffft and pffft_twin show over each other;
# a target counted by its race's paths alone (QEMU) runs the faster one, so is beaten only by what beats each
# usage: perl fastest.pl [-f floor] [-t target] [-c old.fastest] <rounds> [<busy rounds>]
# stdout: "srof <n>: <codes, fastest first>[ | <target> beaten by <codes>]"; stderr: regression, beaten,
# changed and new against old.fastest, and integrity; exits 1 on a regression or an integrity line

use strict;

my $floor = 0.03;
my $target = "PUREfft";
my $old = "";
while (@ARGV > 1 && $ARGV[0] =~ /^-[ftc]$/) {
  my $o = shift(@ARGV);
  my $v = shift(@ARGV);
  if ($o eq "-f") { $floor = $v; } elsif ($o eq "-t") { $target = $v; } else { $old = $v; }
}
my @files = @ARGV;

my @t = (); # $t[file]{code}{size}: times in round order, undef for FAILED
my %sizes = ();
my %codes = ();
for my $i (0..$#files) {
  open(my $fh, "<", $files[$i]) or die "$files[$i]: $!\n";
  while (<$fh>) {
    chomp;
    my ($nam, $prob, $siz, $mflops, $tim) = split / /;
    next unless defined($tim);
    push(@{$t[$i]{$nam}{$siz}}, ($mflops eq "FAILED") ? undef : $tim);
    $sizes{$siz} = 1;
    $codes{$nam} = 1;
  }
  close($fh);
}

sub median {
  my @v = sort { $a <=> $b } @_;
  return $v[$#v / 2];
}

# over the rounds a and b both ran in file i: the median of b's time over a's, a's wins, the rounds
sub pair {
  my ($i, $a, $b, $n) = @_;
  my $ta = $t[$i]{$a}{$n};
  my $tb = $t[$i]{$b}{$n};
  return (undef, 0, 0) unless $ta && $tb;
  my $r = (@$ta < @$tb) ? scalar(@$ta) : scalar(@$tb);
  my @q = ();
  my $wins = 0;
  for my $k (0..$r-1) {
    my ($x, $y) = ($ta->[$k], $tb->[$k]);
    next unless defined($x) || defined($y);
    my $q = !defined($y) ? 9e99 : !defined($x) ? 0 : ($x > 0 ? $y / $x : 1);
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
    next unless defined($q) && $q > 0 && $q < 9e99;
    my $m = ($q > 1) ? $q - 1 : 1 / $q - 1;
    $fl[$i] = $m if $m > $fl[$i];
  }
}

sub beats {
  my ($a, $b, $n) = @_;
  for my $i (0..$#files) {
    my ($q, $wins, $r) = pair($i, $a, $b, $n);
    return 0 unless defined($q) && $r > 0;
    return 0 unless $wins >= ($r >= 3 ? $r - 1 : $r) && $q > 1 + $fl[$i];
  }
  return 1;
}

sub ours {
  my $c = shift;
  return $c eq $target || index($c, "${target}_") == 0;
}

sub time_at {
  my ($c, $n) = @_;
  my @v = grep { defined($_) } @{$t[0]{$c}{$n}};
  return @v ? median(@v) : 9e99;
}

my %was = (); # size: [codes, beaten by]
if ($old ne "" && open(my $fh, "<", $old)) {
  while (<$fh>) {
    next unless /^srof (\d+): ([^|]*?)\s*(?:\| \S+ beaten by (.*))?$/;
    $was{$1} = [$2, defined($3) ? $3 : ""];
  }
  close($fh);
}

my $bad = 0;
printf("# codes no rival beat in all rounds but one by more than the floor%s; floors %s\n",
  (@files > 1) ? ", in each condition" : "", join(" ", map { sprintf("%.1f%%", 100 * $_) } @fl));
for my $n (sort { $a <=> $b } keys %sizes) {
  my @c = grep { $_ ne "pffft_twin" && $t[0]{$_}{$n} } sort keys %codes;
  next unless @c;
  my @libs = grep { !ours($_) } @c;
  my @best = ();
  for my $a (@libs) {
    push(@best, $a) unless grep { $_ ne $a && beats($_, $a, $n) } @libs;
  }
  @best = sort { time_at($a, $n) <=> time_at($b, $n) } @best;
  my @paths = grep { ours($_) && $_ ne $target } @c;
  my @by = (grep { $_ eq $target } @c) ? grep { beats($_, $target, $n) } @libs
          : @paths ? grep { my $x = $_; !grep { !beats($x, $_, $n) } @paths } @libs : ();
  my $line = "srof $n: @best";
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
        print STDERR "integrity: $x srof $n has $k of $r rounds in $files[$i]\n";
        $bad = 1;
      }
    }
  }
  next if $old eq "";
  if (!exists($was{$n})) {
    print STDERR "new: srof $n: @best\n";
    next;
  }
  my ($oldbest, $oldby) = @{$was{$n}};
  if (@by && $oldby eq "") {
    print STDERR "regression: srof $n: $target beaten by @by\n";
    $bad = 1;
  } elsif (@by) {
    print STDERR "beaten: srof $n: $target beaten by @by\n";
  }
  my %o = map { $_ => 1 } split(/ /, $oldbest);
  print STDERR "changed: srof $n: $oldbest -> @best\n" unless grep { $o{$_} } @best;
}
exit($bad);
