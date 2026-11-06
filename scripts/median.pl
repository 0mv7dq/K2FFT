#! /usr/bin/perl -w

# the median round of each code at each size, as libbench2's report.c over its repeats: the line with the middle time, FAILED the slowest
# usage: perl median.pl < rounds > medians

my %lines = ();
my @keys = ();
while (<>) {
  my ($nam, $prob, $siz, $mflops, $tim, $setup_tim) = split / /;
  next unless defined($tim);
  my $key = "$nam $prob $siz";
  push(@keys, $key) unless exists($lines{$key});
  push(@{$lines{$key}}, $_);
}

for my $key (@keys) {
  my @l = sort { time_of($a) <=> time_of($b) } @{$lines{$key}};
  print $l[$#l / 2];
}

sub time_of {
  my (undef, undef, undef, $mflops, $tim) = split / /, $_[0];
  return ($mflops eq "FAILED") ? 9e99 : $tim;
}
