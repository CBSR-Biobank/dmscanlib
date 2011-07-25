
public class ScanlibData  {
	private int returnCode;

	private DmScanlibReturnCode returnCodeEnum;

	private String returnMessage;
	private String [] barcodes;


	public ScanlibData(String [] barcodes, String returnMessage, 
			   int returnCode, DmScanlibReturnCode returnCodeEnum){
		this.barcodes = barcodes;
		this.returnCode = returnCode;
		this.returnMessage = returnMessage;
		this.returnCodeEnum = returnCodeEnum;
	}

	public String [] getBarcodes(){
		return barcodes;
	}

	public DmScanlibReturnCode getReturnCodeEnum(){
		return  returnCodeEnum;
	}

	public int getReturnCode(){
		return returnCode;
	}

	public String getReturnMessage(){
		return returnMessage;
	}
}
