#include <cv.h>
#include <cxtypes.h>
#include <cvaux.h>
#include <highgui.h>
#include <stdio.h>
#include <math.h>
#include <dirent.h>
#include <vector>
#include <string>

#define HISTOGRAM_RESOLUTION 256


void normalizeHistogram(CvHistogram* hist){

	CvMat mat;
	cvGetMat(hist->bins, &mat, 0, 1);

	float max_value, min_value;
	cvGetMinMaxHistValue(hist, &min_value, &max_value);

	for (int i = 0; i < HISTOGRAM_RESOLUTION; i++) {
		float value = cvQueryHistValue_1D( hist, i);
		int normalized = cvRound(value * 300 / max_value);
		cvmSet(&mat, i, 0,normalized);
	}
}

void roundHistogram(CvHistogram* hist){

	CvMat mat;
	cvGetMat(hist->bins, &mat, 0, 1);

	float max_value, min_value;
	cvGetMinMaxHistValue(hist, &min_value, &max_value);

	int buffer[HISTOGRAM_RESOLUTION];
	for (int i = 0; i < HISTOGRAM_RESOLUTION; i++)
		buffer[i] = cvQueryHistValue_1D( hist, i);

	#define ROUNDWIDTH 5
	for (int i = ROUNDWIDTH; i < HISTOGRAM_RESOLUTION-ROUNDWIDTH; i++) {

		double avg = 0;

		for(int j = i-ROUNDWIDTH; j <= i + ROUNDWIDTH; j++)
			avg += buffer[j];
		avg /= ROUNDWIDTH*2 + 1 ;

		cvmSet(&mat, i, 0,(int)(avg));
	}
}



CvHistogram* generateHistogram(IplImage* image) {

	//size of the histogram -1D histogram

	int hsize[] = { HISTOGRAM_RESOLUTION };

	//ranges - grayscale 0 to 256
	float xranges[] = { 0, 256 };
	float* ranges[] = { xranges };

	IplImage* gray = NULL;

	//create an 8 bit single channel image to hold a
	//grayscale version of the original picture
	gray = cvCreateImage(cvGetSize(image), 8, 1);
	cvCvtColor(image, gray, CV_BGR2GRAY);

	IplImage* planes[] = { gray };

	CvHistogram* hist = cvCreateHist(1, hsize, CV_HIST_ARRAY, ranges, 1);
	cvCalcHist(planes, hist, 0, NULL);
	cvReleaseImage(&gray);

	normalizeHistogram(hist);
	roundHistogram(hist);

	return hist;
}

CvHistogram* loadHistogram() {

	CvHistogram* hist = NULL;

	CvArr* bins = cvLoad("average.xml");
	if (bins == NULL) {
		printf("ERROR: histogram could not be loaded\n");
		exit(-1);
	}

	int new_hist_type = CV_IS_MATND(bins) ? CV_HIST_ARRAY : CV_HIST_SPARSE;
	int dummy_size = 5;
	hist = cvCreateHist(1, &dummy_size, new_hist_type, 0, 1);

	if (new_hist_type == CV_HIST_ARRAY) {
		cvReleaseData(hist->bins);
		hist->mat = *(CvMatND*) bins;
		hist->bins = &hist->mat;
		cvIncRefData(hist->bins);

	} else {
		cvReleaseSparseMat((CvSparseMat**) &hist->bins);
		hist->bins = bins;
	}

	return hist;
}


void saveHistogram(CvHistogram* hist, char * file) {
	cvSave(file, hist->bins);
}

void drawHistogram(IplImage* imgHistogram, CvHistogram* hist, CvScalar color) {

	float max_value, min_value;

	cvGetMinMaxHistValue(hist, &min_value, &max_value);

	cvRectangle(imgHistogram, cvPoint(0, 0), cvPoint(256, 300), CV_RGB(0,0,0),
			-1);

	max_value = 300;

	//draw the histogram :P
	for (int i = 0; i < HISTOGRAM_RESOLUTION; i++) {
		float value = cvQueryHistValue_1D( hist, i);
		cvLine(imgHistogram, cvPoint(i, 300), cvPoint(i, 300 - value),
				color);
	}
}

//cvGetMat http://www710.univ-lyon1.fr/~bouakaz/OpenCV-0.9.5/docs/ref/OpenCVRef_BasicFuncs.htm
double averageHistogramValue(CvHistogram* hist) {
	CvMat mat;
	cvGetMat(hist->bins, &mat, 0, 1);

	if (mat.cols != 1) {
		printf("ERROR: histogram contained more than one column\n");
		exit(-1);
	}

	double sum = 0;
	for (int row = 0; row < mat.rows; row++) {
		sum += cvmGet(&mat, row, 0);
	}
	return sum / mat.rows;
}

//CV_COMP_CORREL clone
double customCorrelation(CvHistogram* h1, CvHistogram* h2) {
	CvMat mat1;
	CvMat mat2;
	double avgh1;
	double avgh2;

	cvGetMat(h1->bins, &mat1, 0, 1);

	cvGetMat(h2->bins, &mat2, 0, 1);

	//cvSave( "saveme.xml",&mat1 );

	avgh1 = averageHistogramValue(h1);
	avgh2 = averageHistogramValue(h2);

	//printf("mat1: %d/rows %d/cols %3.3f/avg\n", mat1.rows, mat1.cols, avgh1);
	//printf("mat2: %d/rows %d/cols %3.3f/avg\n", mat2.rows, mat2.cols, avgh2);

	double top = 0;
	double bottomLeft = 0;
	double bottomRight = 0;
	for (int i = 0; i < mat1.rows; i++) {
		top += (cvmGet(&mat1, i, 0) - avgh1) * (cvmGet(&mat2, i, 0) - avgh2);
		bottomLeft += pow(cvmGet(&mat1, i, 0) - avgh1, 2);
		bottomRight += pow(cvmGet(&mat2, i, 0) - avgh2, 2);
	}
	return top / sqrt(bottomLeft * bottomRight);
}

int listdir(const char *path) {
	struct dirent *entry;
	DIR *dp;

	dp = opendir(path);
	if (dp == NULL) {
		perror("opendir");
		return -1;
	}

	while ((entry = readdir(dp)))
		puts(entry->d_name);

	closedir(dp);
	return 0;
}

int histogramCompareImage(int argc, char** argv) {

	IplImage* image = NULL;

	if (argc != 2 || !(image = cvLoadImage(argv[1]))) {
		return -1;
	}

	//listdir(argv[1]);

	CvHistogram* hist = generateHistogram(image);
	CvHistogram* hist2 = loadHistogram();

	//cv::normalize(hist.mat, hist.mat, 1, 0, cv::NORM_L1);
	printf("CUSTOM: %3.9f\n", customCorrelation(hist, hist2));

	printf("CV_COMP_CORREL: %3.9f\n",
			cvCompareHist(hist, hist2, CV_COMP_CORREL));

	cvNamedWindow("histogram", 1);

	IplImage* imgHistogram = cvCreateImage(cvSize(HISTOGRAM_RESOLUTION, 300), 8,
			3);
	drawHistogram(imgHistogram, hist, CV_RGB(255,0,0));

	cvShowImage("histogram", imgHistogram);

	cvNamedWindow("histogram 2", 1);

	IplImage* imgHistogram2 = cvCreateImage(cvSize(HISTOGRAM_RESOLUTION, 300),
			8, 3);
	drawHistogram(imgHistogram2, hist2, CV_RGB(0,255,0));

	cvShowImage("histogram 2", imgHistogram2);

	cvReleaseHist(&hist);
	cvReleaseHist(&hist2);
	cvReleaseImage(&image);

	cvWaitKey();

	return 0;
}
void createHistogramWindow(const char * windowName,CvHistogram* hist,CvScalar color){
	cvNamedWindow(windowName, 1);
	IplImage* imgHistogram = cvCreateImage(cvSize(HISTOGRAM_RESOLUTION, 300), 8,
			3);
	drawHistogram(imgHistogram, hist, color);
	cvShowImage(windowName, imgHistogram);
	cvReleaseImage(&imgHistogram);
}

struct HistogramWrapper {
  CvHistogram * hist;
  std::string filename;
} ;


int averageHistogramsInDirectory(int argc, char** argv) {

	std::string directory;

	std::vector<HistogramWrapper> histograms;
	std::vector<HistogramWrapper>::iterator it;

	if (argc == 2)
		directory = std::string(argv[1]);
	else
		return -1;

	CvHistogram* hist2 = loadHistogram(); // loads from hardcoded xml file

	struct dirent *entry;
	DIR *dp;
	dp = opendir(directory.c_str());
	if (dp == NULL) {
		perror("opendir");
		return -1;
	}

	while ((entry = readdir(dp))) {
		std::string filename = std::string(entry->d_name);
		if (filename.find(".bmp") == std::string::npos || filename.find("missed") == std::string::npos)
			continue;

		printf("Loading image: %s\n", filename.c_str());
		IplImage* image = cvLoadImage((directory + filename).c_str());
		if (image == NULL) {
			printf("ERROR: could not load image: %s\n",
					(directory + filename).c_str());
			exit(-1);
		}

		CvHistogram* hist = generateHistogram(image);
		if (hist == NULL) {
			printf("ERROR: could not create histogram for image: %s\n",
					(directory + filename).c_str());
			exit(-1);
		}
		HistogramWrapper hw;
		hw.hist = hist;
		hw.filename = filename;

		histograms.push_back(hw);

		cvReleaseImage(&image);
	}
	closedir(dp);

	CvMat tmpMatrix;
	cvGetMat((*histograms.begin()).hist->bins, &tmpMatrix, 0, 1);

	CvMat * avgHistMatrix = cvCloneMat(&tmpMatrix);
	for (int i = 0; i < avgHistMatrix->rows; i++)
		cvmSet(avgHistMatrix, i, 0, 0);

	CvMat* bufferMatrix;


	CvHistogram * avgHist =loadHistogram();
	createHistogramWindow("average",avgHist,CV_RGB(0,255,0));
	cvWaitKey();

	for (it = histograms.begin(); it < histograms.end(); it++) {

		CvMat histMatrix;
		CvHistogram * hist;

		hist = (*it).hist;
		cvGetMat(hist->bins, &histMatrix, 0, 1);

		// addition with buffer
		bufferMatrix = cvCloneMat(avgHistMatrix);
		cvAdd(bufferMatrix, &histMatrix, avgHistMatrix);
		cvReleaseMat(&bufferMatrix);

		double correlation = customCorrelation(hist, hist2);

		if(correlation < 0.7){
		char stringBuffer [100];
		sprintf (stringBuffer,"%s: %3.9f\n",(*it).filename.c_str(), correlation);
		createHistogramWindow(stringBuffer,hist,CV_RGB(255,0,0));
		cvWaitKey();
		}


	}

	// average
	for (int i = 0; i < avgHistMatrix->rows; i++) {
		cvmSet(avgHistMatrix, i, 0,
				(int)cvmGet(avgHistMatrix, i, 0) / histograms.size());
	}

	cvSave("average2.xml", avgHistMatrix);

/*
	 float xranges[] = { 0, 256 };
	 float* ranges[] = { xranges };
	 int hsize[] = { HISTOGRAM_RESOLUTION };

	 CvHistogram * avgHist = cvCreateHist( 1, hsize,  CV_HIST_ARRAY, ranges, 1 );
	 cvReleaseData( avgHist->bins );
	 avgHist->mat = *(CvMatND*)avgHistMatrix; //note
	 avgHist->bins = &avgHist->mat;
	 cvIncRefData( avgHist->bins );

*/


	return 0;
}

int main(int argc, char** argv) {

	return averageHistogramsInDirectory(argc, argv);
}

