#!/bin/perl
use List::Util qw[min max];

#"nuk_600_normal.bmp","nuk_300_normal.bmp","nuk_400_normal.bmp",
foreach((
	   "01_600_dark.bmp","01_300_dark.bmp","01_400_dark.bmp",
	   "01_300_normal.bmp","01_400_normal.bmp","01_600_normal.bmp",
	   "02_300_normal.bmp","02_400_normal.bmp","02_600_normal.bmp",
	   "03_300_normal.bmp","03_400_normal.bmp","03_600_normal.bmp"
	   )){

$input = $_;

$old = "./dmscanlib.exe --debug 9 -d -p 1 -i $input ";
$new = "./dmscanlib.exe --debug 9 --super -i $input";

`rm dmscanlib.txt`;
print "Old: $old\n";
$init_time = time();
`$old`;
$end_time = time();

$oldtube_count = int(`cat dmscanlib.txt | wc -l`) - 1;

print "Tube count : " . $oldtube_count ." tubes\n";
print "Time taken: " . ($end_time-$init_time). " sec\n";

$old_time = $end_time-$init_time;

print "\n";

`rm dmscanlib.txt`;
print "New: $new \n";
$init_time = time();
`$new`;
$end_time = time();

$newtube_count = int(`cat dmscanlib.txt | wc -l`) - 1;

print "Tube count : " . $newtube_count . " tubes\n";
print "Time taken: " . ($end_time-$init_time). " sec\n";

$new_time = $end_time-$init_time;

print "\n";

printf("Accuracy Improvement (Extra Tubes Scanned): %d\n",$newtube_count - $oldtube_count);
printf("Speed Improvement (Time saved): %d\n",($old_time-$new_time));
print "\n";

}