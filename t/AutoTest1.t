#! /usr/bin/env perl
use strict;
use Test::More tests => 2;
use File::Copy;

my $src = << 'EOF';
package AutoTest1;

use warnings;
use strict;
use AutoTest;

sub h {
	return "hoge";
}

sub g {
	my $str = h(54);
	my $hash = {"a" => 12, "b" => 24};
	return (3, 4, 5, 6);
}

sub f {
	my @hash = g("args!");
	return 33;
}

my $a = f((1, 2, 3, 4));
EOF

SKIP: {
skip('/tmp/AutoTest1.t not writable', 3)
      unless (-d '/tmp' and -w '/tmp/');

my $script = 't/AutoTest1.pl';
my $t = '/tmp/AutoTest1.t';    # so far hardcoded
sub sdump ($$) {
  my $script = shift;
  my $src = shift;
  open F, ">", $script;
  print F $src;
  close F;
}

sdump $script, $src;
END { unlink $script; }
my $X = $^X =~ m/\s/ ? qq{"$^X"} : $^X;
my $result = `$X -Mblib $script`;

ok(-f $t and -s $t, "$t created");

ok(system($X,'-c',$t) == 0, "$t parsable");

# prepare 2nd test
$src =~ s/use AutoTest;//;
sdump 'AutoTest1.pm', $src;
File::Copy::mv($t,'t/AutoTest2.t');

}
