
public class DmScanlib {
    static {
        System.loadLibrary("DmScanlib");
    }

    static public native ScanlibData getScanlibDataTest(int verbose,
        String filename, ScanSettings scanSettings,
        ScanCoordinates scanCoordinates, DecodeProfile decodeProfile,
        DecodeSettings decodeSettings);

    static public native ScanlibData slIsTwainAvailable();

    static public native ScanlibData slSelectSourceAsDefault();

    static public native ScanlibData slGetScannerCapability();

    static public native ScanlibData slScanImage(int verbose, String filename,
        ScanSettings scanSettings, ScanCoordinates scanCoordinates);

    static public native ScanlibData slScanFlatbed(int verbose,
        String filename, ScanSettings scanSettings);

    static public native ScanlibData slDecodePlate(int verbose,
        ScanSettings scanSettings, ScanCoordinates scanCoordinates,
        DecodeProfile decodeProfile, DecodeSettings decodeSettings);

    static public native ScanlibData slDecodeImage(int verbose,
        String filename, DecodeProfile decodeProfile,
        DecodeSettings decodeSettings);

    @SuppressWarnings("unchecked")
    /***
	 * Method to retrieve any enum value by the string name/value pair.
	 * getEnum("RETURNCODE","SUCCESS") will create an RETURNCODE enum object
	 * with the value set to SUCCESS.
	 * @param enumName
	 * @param enumValue
	 * @return an Enum Object or Null if the Enum name/value pair is not found.
    */
    static public <T> T getEnum(String enumName, String enumValue) {

        Class<?> enumClass;
        try {
            enumClass = (Class<?>) Class.forName(enumName);
        } catch (ClassNotFoundException e) {
            return null;
        }

        java.lang.Object[] enumValues = (java.lang.Object[]) enumClass
            .getEnumConstants();

        for (int i = 0; i < enumValues.length; i++) {
            if (enumValues[i].toString() != null
                && enumValues[i].toString().equals(enumValue)) {
                return (T) enumClass.cast(enumValues[i]);
            }
        }

        return null;
    }
}
