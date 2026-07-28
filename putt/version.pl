#!/usr/bin/perl -w
use strict;
use warnings;
#
# declare local variables
#
my $pgm = "putt";
my $PGM = uc $pgm;
my $n;
my @infile;
my $ver;
my $vdt;
my $i;
#
# get version number from version.h
#
$n = "version.h";
open FILE,$n or die "can't open ".$n;
@infile = <FILE>;
foreach(@infile)
{
    if($_ =~ /#define\sVER.*/) {
	($ver) = $_ =~ /.*(\d\.\d\d[a-z]*), .*"/;
	($vdt) = $_ =~ /.*\d\.\d\d[a-z]*, (.*)"/;
    }
}
close FILE;
print "version is $ver\n";

# update version in $pgm.iss
#
$n = "$pgm.iss";
open FILE,$n or die "can't open ".$n; 
@infile = <FILE>;
$i = 0;
foreach(@infile)
{
    if($_ =~ /AppVer.*/) {
	$infile[$i] =~ s//AppVerName=BTNRH $PGM $ver/;
    }
    $i ++;
} 
close FILE;
open FILE, ">".$n;
print FILE @infile;
close FILE;

# update version in readme.win
#
$n = "readme.win";
open FILE,$n or die "can't open ".$n; 
@infile = <FILE>;
$i = 0;
foreach(@infile)
{
    if($_ =~ /version.*/) {
	$infile[$i] =~ s//version $ver, $vdt/;
    }
    $i ++;
} 
close FILE;
open FILE, ">".$n;
print FILE @infile;
close FILE;
