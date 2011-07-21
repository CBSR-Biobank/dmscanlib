
public class DmScanlib_JNI_Test {

     public static void DmScanlib_getScanlibDataCopyTest(){
	
	System.out.println("===========================");

	System.out.println("getScanlibDataTest Sample:");

	ScanlibData sd = DmScanlib.getScanlibDataTest(5,"happy",
						new ScanSettings(2,4,9),
						new ScanCoordinates(3.0,4.1,5.2,6.3),
						new DecodeProfile(1,3,5),
						new DecodeSettings(false,1,2,3,4,1.1,1.2,1.3,1.4));

	for(String ss :  sd.getBarcodes())
		System.out.println("Barcodes: " + ss);

	System.out.println("Ret code:" + sd.getReturnCode());
	System.out.println("Ret msg:" + sd.getReturnMessage());
		


	System.out.println("===========================");

	String [] messages = new String[]{"hello","johnny","mango","marion","fresh"};


	boolean testFail = false;

	for(int z= 0; z < 10 && (testFail == false); z++){ //1000*10000 iterations is about a minute

		int [] st = {-5,1,-2,3,-4,5,-60,7,-84,9};

		double [] dt = {0.001,0.002,-50.004,-0.005,0,-64.021,-6.302,-6.013};
	
		for(int i = 0;i < 10000; i++){
			st[i  % st.length]++;
			dt[i % dt.length] += 0.4701 * (i % 5);

			/*
			for(int j = 0 ; j < st.length; j++)
				System.out.print(st[j] + ",");		
			System.out.println("");

			for(int j = 0 ; j < dt.length; j++)
				System.out.printf("%5.5f,",dt[j]);		
			System.out.println("");
			*/

			int ii = 0;		

			boolean vertical = (i % 2 == 0);
			int verbose = (i % 5) + 1;

			int dpi = st[ii++];
			int brightness =st[ii++];
			int contrast = st[ii++];
			ScanSettings scanSettings = new ScanSettings(dpi,brightness,contrast);

			int da = st[ii++];
			int db = st[ii++];
			int dc = st[ii++];
			DecodeProfile decodeProfile = new DecodeProfile(da,db,dc);

			int platenum = st[ii++];
			int corrections = st[ii++];
			int squaredev = st[ii++];
			int edgethresh = st[ii++];

			ii = 0;
		
			double top = dt[ii++];
			double left = dt[ii++];
			double right = dt[ii++];
			double bottom = dt[ii++];
			ScanCoordinates scanCoordinates = new ScanCoordinates(top,left,right,bottom);


			double scangap = dt[ii++];
			double celld = dt[ii++];
			double gapx = dt[ii++];
			double gapy = dt[ii++];
			DecodeSettings decodeSettings = new DecodeSettings(
								vertical,
								platenum,
								corrections,
								squaredev,
								edgethresh,
								scangap,celld,gapx,gapy);
		

	
			int scanSettingsSumInt = dpi + brightness + contrast;
			int decodeProfileSumInt = da + db + dc;
			int decodeSettingsInt = platenum + corrections + squaredev + edgethresh;
			int verticalInt = vertical ? 1 : 0;
		
			double scanCoordinateSumDbl = top + left + right + bottom;
			double decodeSettingsDbl = scangap + celld + gapx + gapy;	

			int predictedReturnCode =
			(scanSettingsSumInt + 
			 decodeProfileSumInt + 
			 decodeSettingsInt)*(verbose + verticalInt) +
			(int)((scanCoordinateSumDbl + decodeSettingsDbl)*10000) << 10;


			ScanlibData s = DmScanlib.getScanlibDataTest(verbose,
							    messages[verbose-1],
							    scanSettings,
							    scanCoordinates,
							    decodeProfile,
							    decodeSettings);

			if(s == null){
				System.out.println("null string"); 
				testFail = true;				
				break;
			}

			int returnCode = s.getReturnCode();

			if(returnCode  != predictedReturnCode || 
			  !messages[verbose-1].equals( s.getReturnMessage())){
				System.err.println("Test failed");
				testFail = true;				
				break;
			}
		
		}
	}
	if(!testFail){
		System.out.println("getScanlibDataTest test passed");
	}
	else{
		System.out.println("WARNING: getScanlibDataTest test FAILED" );
	}

	System.out.println("===========================");

	ScanlibData twainAvailableData = DmScanlib.slIsTwainAvailable();

	if(twainAvailableData.getReturnCode() == 5 && 
	   twainAvailableData.getReturnMessage().equals("success")){
		System.out.println("Twain test passed");
	}
	else{
		System.out.println("WARNING: Twain test FAILED");
	}
	System.out.println("===========================");
	ScanlibData defaultSource = DmScanlib.slSelectSourceAsDefault();

	if(defaultSource.getReturnCode() == 6 && 
	   defaultSource.getReturnMessage().equals("scanner 1")){
		System.out.println("defaultSource test passed");
	}
	else{
		System.out.println("WARNING: defaultSource test FAILED");
	}
	System.out.println("===========================");
	ScanlibData scannerCapability = DmScanlib.slGetScannerCapability();

	if(scannerCapability.getReturnCode() == 7 && 
	   scannerCapability.getReturnMessage().equals("very capable")){
		System.out.println("scannerCapability test passed");
	}
	else{
		System.out.println("WARNING: scannerCapability test FAILED");
	}

	System.out.println("===========================");

	ScanSettings flatBedSettings = new ScanSettings(1,2,3);

	ScanlibData flatbedScanData = DmScanlib.slScanFlatbed(
						5, "flatbed",flatBedSettings);
	
	if(flatbedScanData.getReturnCode() == 11 && 
	   flatbedScanData.getReturnMessage().equals("flatbed")){
		System.out.println("flatbedScanData test passed");
	}
	else{
		System.out.println("WARNING: flatbedScanData test FAILED" );
	}
	
	System.out.println("===========================");

	ScanSettings scanImageSettings = new ScanSettings(1,2,3);
	ScanCoordinates scanImageCoordinates = new ScanCoordinates(4,5,6,7);

	ScanlibData imageScanData = DmScanlib.slScanImage(8, "imagey",scanImageSettings,
								     scanImageCoordinates);
	
	if(imageScanData.getReturnCode() == 36 && 
	   imageScanData.getReturnMessage().equals("imagey")){
		System.out.println("slScanImage test passed");
	}
	else{
		System.out.println("WARNING: slScanImage test FAILED" );
	}
	
	System.out.println("===========================");

	ScanSettings decodePlateScanSettings = new ScanSettings(1,2,3);
	ScanCoordinates decodePlateCoordinates = new ScanCoordinates(4,5,6,7);
	DecodeProfile decodePlateProfile = new DecodeProfile(8,9,11);
	DecodeSettings decodePlateSettings = new DecodeSettings(true,3,1,5,6,7,4,1,4);

	ScanlibData decodePlateData = DmScanlib.slDecodePlate(9,decodePlateScanSettings,
							     decodePlateCoordinates,
							     decodePlateProfile,
							     decodePlateSettings);

	if(decodePlateData.getReturnCode() == 97 && 
	   decodePlateData.getReturnMessage().equals("henry jones")){
		System.out.println("slDecodePlate test passed");
	}
	else{
		System.out.println("WARNING: slDecodePlate test FAILED" );
	}
	
	System.out.println("===========================");

	DecodeProfile decodeImageProfile = new DecodeProfile(8,9,11);
	DecodeSettings decodeImageSettings = new DecodeSettings(true,3,1,5,6,7,4,1,4);

	ScanlibData decodeImageData = DmScanlib.slDecodeImage(9,"freshh",
							     decodeImageProfile,
							     decodeImageSettings);

	if(decodeImageData.getReturnCode() == 69 && 
	   decodeImageData.getReturnMessage().equals("freshh")){
		System.out.println("slDecodeImage test passed");
	}
	else{
		System.out.println("WARNING: slDecodeImage test FAILED" );
	}
	
	System.out.println("===========================");

	System.out.println("Finished.");

     }



	
    public static void main(String[] args) {
	DmScanlib_getScanlibDataCopyTest();
    }
	
}
