public class DecodeSettings  {
	int plateNum;
	boolean isVertical;

	double scanGap;
	double cellDistance;
	double gapX;
	double gapY;

	int corrections;
	int squareDev;
	int edgeThresh;

	public DecodeSettings( boolean isVertical,
				int plateNum,
				int corrections,
				int squareDev,
				int edgeThresh,
				double scanGap,
				double cellDistance,
				double gapX,
				double gapY){

		this.plateNum = plateNum;
		this.isVertical = isVertical;
		this.scanGap = scanGap;
		this.cellDistance = cellDistance;
		this.gapX = gapX;
		this.gapY = gapY;

		this.corrections = corrections;
		this.squareDev = squareDev;
		this.edgeThresh = edgeThresh;
	}

}
