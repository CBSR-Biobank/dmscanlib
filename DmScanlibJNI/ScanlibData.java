
public class ScanlibData  {
	private int returnCode;
	private String returnMessage;
	
	private String [] barcodes;
	
	public ScanlibData(String [] barcodes, String returnMessage, int returnCode){
		this.barcodes = barcodes;
		this.returnCode = returnCode;
		this.returnMessage = returnMessage;
	}

	public String [] getBarcodes(){
		return barcodes;
	}

	public int getReturnCode(){
		return returnCode;
	}

	public String getReturnMessage(){
		return returnMessage;
	}
}
