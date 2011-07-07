#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#include <stdio.h>
#include <dirent.h>


#define HISTOGRAM_RESOLUTION 256

CvHistogram* generateHistogram( IplImage* image){

     //size of the histogram -1D histogram
    
    int hsize[] = {HISTOGRAM_RESOLUTION};

    //ranges - grayscale 0 to 256
    float xranges[] = { 0, 256 };
    float* ranges[] = { xranges };

    IplImage* gray = NULL;
	
    //create an 8 bit single channel image to hold a
    //grayscale version of the original picture
    gray = cvCreateImage( cvGetSize(image), 8, 1 );
    cvCvtColor( image, gray, CV_BGR2GRAY );

    IplImage* planes[] = { gray };

    CvHistogram* hist = cvCreateHist( 1, hsize, CV_HIST_ARRAY, ranges,1);
    cvCalcHist( planes, hist, 0, NULL);

    cvReleaseImage(&gray);

    return hist;
}

CvHistogram* loadHistogram(){

	CvHistogram* hist = NULL;

	CvArr* bins = cvLoad("hist.xml" );
	int new_hist_type = CV_IS_MATND(bins) ? CV_HIST_ARRAY :CV_HIST_SPARSE;
	int dummy_size = 5;
	hist = cvCreateHist( 1, &dummy_size, new_hist_type, 0, 1 );

	if( new_hist_type == CV_HIST_ARRAY ){
		cvReleaseData( hist->bins );
		hist->mat = *(CvMatND*)bins;
		hist->bins = &hist->mat;
		cvIncRefData( hist->bins );
	}
	else{
		cvReleaseSparseMat( (CvSparseMat**)&hist->bins );
		hist->bins = bins;
	}

	return hist;
}

void saveHistogram(CvHistogram* hist, char * file){
	cvSave( file, hist->bins );
}


void drawHistogram(IplImage* imgHistogram, CvHistogram* hist,CvScalar color){

    float max_value, min_value;

    cvGetMinMaxHistValue( hist,&min_value,&max_value);
    
    cvRectangle(imgHistogram, cvPoint(0,0), cvPoint(256,300), CV_RGB(0,0,0),-1);

    //draw the histogram :P
    for(int i=0; i < HISTOGRAM_RESOLUTION; i++){
            float value = cvQueryHistValue_1D( hist, i);
            int normalized = cvRound(value*300/max_value);
            cvLine(imgHistogram,cvPoint(i,300), cvPoint(i,300-normalized), color);
    }
}


int listdir(const char *path) {
  struct dirent *entry;
  DIR *dp;
 
  dp = opendir(path);
  if (dp == NULL) {
    perror("opendir");
    return -1;
  }
 
  while((entry = readdir(dp)))
    puts(entry->d_name);
 
  closedir(dp);
  return 0;
}

int main( int argc, char** argv ){
    IplImage* image = NULL;

    if( argc != 2 || !(image = cvLoadImage(argv[1])) )
        return -1;

    listdir(argv[1]);
   
    CvHistogram* hist = generateHistogram(image);
    CvHistogram* hist2 = loadHistogram();

    //cv::normalize(hist.mat, hist.mat, 1, 0, cv::NORM_L1);
    
  
    printf("CV_COMP_CORREL: %3.5f\n",cvCompareHist(hist,hist2,CV_COMP_CORREL));

    cvNamedWindow("histogram",1);
   
    IplImage* imgHistogram = cvCreateImage(cvSize(HISTOGRAM_RESOLUTION, 300),8,3);
    drawHistogram(imgHistogram,hist,CV_RGB(255,0,0));

    cvShowImage( "histogram", imgHistogram );

    cvNamedWindow("histogram 2",1);
   
    IplImage* imgHistogram2 = cvCreateImage(cvSize(HISTOGRAM_RESOLUTION, 300),8,3);
    drawHistogram(imgHistogram2,hist2,CV_RGB(0,255,0));

    cvShowImage( "histogram 2", imgHistogram2);

    cvReleaseHist( &hist );
    cvReleaseHist( &hist2 );
    cvReleaseImage(&image);

    cvWaitKey();

    return 0;
}

