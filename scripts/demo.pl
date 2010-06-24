#!/bin/perl


foreach((
	   "01_600_dark.bmp","01_300_dark.bmp","01_400_dark.bmp",
	   "01_300_normal.bmp","01_400_normal.bmp","01_600_normal.bmp",
	   "02_300_normal.bmp","02_400_normal.bmp","02_600_normal.bmp",
	   "03_300_normal.bmp","03_400_normal.bmp","03_600_normal.bmp",
	   "nuk_600_normal.bmp","nuk_300_normal.bmp","nuk_400_normal.bmp"
	   )){

$input = $_;

$old = "./scanlib.exe --debug 9 -d -p 1 -i $input ";
$new = "./scanlib.exe --debug 9 --super -i $input";

`rm scanlib.txt`;
print "============$input================\n\n"; 
print "OLD scanlib:\n";
print "Running: $old\n";
$init_time = time();
`$old`;
$end_time = time();

$oldtube_count = int(`cat scanlib.txt | wc -l`) - 1;

print "Tube count (+1): " . $oldtube_count ." tubes\n";
print "Time taken: " . ($end_time-$init_time). " sec\n";

print "\n\n";

`rm scanlib.txt`;
print "NEW scanlib:\n";
print "Running: $new \n";
$init_time = time();
`$new`;
$end_time = time();

$newtube_count = int(`cat scanlib.txt | wc -l`) - 1;

print "Tube count (+1): " . $newtube_count . " tubes\n";
print "Time taken: " . ($end_time-$init_time). " sec\n";

if($newtube_count < $oldtube_count){
	print "BAD  NUMBER OF TUBES SCANNED\n";
	sleep(5000);
}
print "=============End================\n\n";

}