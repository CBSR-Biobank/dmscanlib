

public class DmScanlib {
    	static {
        	System.loadLibrary("DmScanlib");
   	}

   	static public native ScanlibData getScanlibDataTest(int verbose, String filename,
							ScanSettings scanSettings,
							ScanCoordinates scanCoordinates,
							DecodeProfile decodeProfile,
							DecodeSettings decodeSettings);


    	static public native ScanlibData slIsTwainAvailable();

	
	static public native ScanlibData slSelectSourceAsDefault();


	static public native ScanlibData slGetScannerCapability();


 	static public native ScanlibData slScanImage(int verbose, String filename,
							ScanSettings scanSettings,
							ScanCoordinates scanCoordinates);


	static public native ScanlibData slScanFlatbed(int verbose, String filename, 
							ScanSettings scanSettings);


	static public native ScanlibData slDecodePlate(int verbose, 								ScanSettings scanSettings,
							ScanCoordinates scanCoordinates,
							DecodeProfile decodeProfile,
							DecodeSettings decodeSettings);


	static public native ScanlibData slDecodeImage(int verbose,String filename,
							DecodeProfile decodeProfile,
							DecodeSettings decodeSettings);
}
