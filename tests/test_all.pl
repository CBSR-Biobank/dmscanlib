#!/usr/bin/perl

$scanLibBarcodesFile = "dmscanlib.txt";
$scanLib = "../Linux-Debug/dmscanlib.exe";
$imgDir = "./images";
$barcodesTextDir = "./barcodes";

sub decodeImage
{
	my ($imagePath) = @_;
	my $output = `${scanLib} -d --plate 1  -i ${imagePath}`;

	my @successMatches = ($ouput =~ m/SC_SUCCESS/g);

	if(length(@successMatches) == 0)
	{
		print "Error occured.\n";
		print $output;
		return -1;	
	}

	`rm *.bmp`;
	`rm *.pnm`;

	if(!(-e ${scanLibBarcodesFile}))
	{
		print "Error occured.\n";
		print "Could not find ${scanLibBarcodesFile}.";
		return -1;	
	}
	
	my $lineCount = `cat ${scanLibBarcodesFile} | wc -l`;

	chomp($lineCount);	
	
	return ($lineCount - 1);

}


#print "Got barcode #: " . decodeImage("${imgDir}/001.bmp") . "\n";


my $foundBarcodesSum = 0.0;
my $expectedBarcodesSum = 0.0;

my $decodedCount = 0.0;
my $successCount = 0.0;

opendir(DIR, "images");

my @images = sort readdir(DIR); 

foreach(@images)
{
	if($_ =~ m/\.bmp$/i)
	{

		if(-e ${scanLibBarcodesFile})
		{
			`rm ${scanLibBarcodesFile}`;
		}

		print "Got " . decodeImage("${imgDir}/$_") . " barcodes for $_\n";
		$decodedCount += 1;
		
		if(-e ${scanLibBarcodesFile})
		{
			my $fileNumber = substr($_,0,3);

			if(-e "${barcodesTextDir}/${fileNumber}.txt")
			{
				$foundBarcodesSum += `cat ${scanLibBarcodesFile} | wc -l`;
				$expectedBarcodesSum += `cat ${barcodesTextDir}/${fileNumber}.txt | wc -l`;

				if(length(`diff ${scanLibBarcodesFile} ${barcodesTextDir}/${fileNumber}.txt`) != 0){
					print "Barcodes Mismatch: ${scanLibBarcodesFile} && ${barcodesTextDir}/${fileNumber}.txt differ.\n";			
				}
				else{
					print "Barcodes match.\n";
					$successCount += 1;			
				}
			}
			else
			{
				print "New barcodes file ${barcodesTextDir}/${fileNumber}.txt was added. (Not considered a succesful match)\n";
				`mv ${scanLibBarcodesFile} ${barcodesTextDir}/${fileNumber}.txt`;
			}

		}
		else
		{
			print "Could not find barcode file for $_\n";
		}
	}
}
print "\n";
print "Total Barcodes Found: ($foundBarcodesSum/$expectedBarcodesSum).\n";
print "Matched Barcodes: ($successCount/$decodedCount).\n";
print "Accuracy: " . $successCount/$decodedCount*100.0 . "%\n";
if($successCount != $decodedCount){
	print "WARNING: SOME BARCODES WHERE NOT MATCHED CORRECTLY.\n";
}




