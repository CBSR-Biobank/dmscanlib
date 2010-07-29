#!/bin/perl

@platedim = ( 2.0, 0.6 , 6.4 , 3.6 );
$squaredev = 15;
$threshold = 5;
$gap = 0.085;
$celldist = 0.340;
$dpi = 400;
$brightness = 0;
$contrast = 0;

$brightnessSign = " ";
$contrastSign = " ";

while(1){
	$brightness=int(-1000+rand(2000)); 
	$contrast=int(-1000+rand(2000)); 

	if($brightness < 0){
		$brightnessSign = "-";
		$brightness = abs($brightness);
	}

	if($contrast < 0){
		$contrastSign = "-";
		$contrast = abs($contrast);
	}
	
	open(FH, ">>scannedTubes.log");
	print FH sprintf("TOSCAN/{ Brightness: %s%.4d, Contrast: %s%.4d } \n",$brightnessSign,$brightness,$contrastSign,$contrast);
	close(FH);
	
	`rm -f dmscanlib.txt`;
	$dmscanlibOut = `./scanlib.exe -l $platedim[0] -t $platedim[1] -r $platedim[2] -b $platedim[3] -p 1 --square-dev $squaredev --threshold $threshold --gap $gap --celldist $celldist --dpi $dpi --brightness $brightness --contrast $contrast -d --debug 9`;



	if(!(-e "dmscanlib.txt")){
		printf("FAILD Scanned: %.2d\t { Brightness: %s%.4d, Contrast: %s%.4d } \n",0,$brightnessSign,$brightness,$contrastSign,$contrast);
		open(FH, ">>failTubes.log");
		print FH sprintf("FAILD Scanned: %.2d\t { Brightness: %s%.4d, Contrast: %s%.4d } \n",0,$brightnessSign,$brightness,$contrastSign,$contrast);
		print FH $dmscanlibOut;
		close(FH);
	}
	else{
		$tubesScanned = int(`cat dmscanlib.txt | wc -l`) - 1;
		open(FH, ">>successTubes.log");
		print FH sprintf("%.2d,%s%.4d,%s%.4d\n",$tubesScanned,$brightnessSign,$brightness,$contrastSign,$contrast);
		close(FH);
		printf("Tubes Scanned: %.2d\t { Brightness: %s%.4d, Contrast: %s%.4d } \n",$tubesScanned,$brightnessSign,$brightness,$contrastSign,$contrast);
	}
}