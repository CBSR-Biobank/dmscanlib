#include <cv.h>
#include <cvaux.h>
#include <highgui.h>
#include <stdio.h>

IplImage* image= 0;
IplImage* imgHistogram = 0;
IplImage* gray= 0;

CvHistogram* hist;



int main( int argc, char** argv ){

    if( argc != 2 || !(image = cvLoadImage(argv[1])) )
        return -1;

    //size of the histogram -1D histogram
    int bins = 256;
    int hsize[] = {bins};

    //max and min value of the histogram
    float max_value = 0, min_value = 0;

    //value and normalized value
    float value;
    int normalized;

    //ranges - grayscale 0 to 256
    float xranges[] = { 0, 256 };
    float* ranges[] = { xranges };

    //create an 8 bit single channel image to hold a
    //grayscale version of the original picture
    gray = cvCreateImage( cvGetSize(image), 8, 1 );
    cvCvtColor( image, gray, CV_BGR2GRAY );

    //Create 3 windows to show the results
    cvNamedWindow("original",1);
    cvNamedWindow("gray",1);
    cvNamedWindow("histogram",1);

    //planes to obtain the histogram, in this case just one
    IplImage* planes[] = { gray };

    //get the histogram and some info about it
    hist = cvCreateHist( 1, hsize, CV_HIST_ARRAY, ranges,1);
    cvCalcHist( planes, hist, 0, NULL);
    cvGetMinMaxHistValue( hist, &min_value, &max_value);
    printf("min: %f, max: %f\n", min_value, max_value);

    //cvSave( "hist.xml", hist->bins );
	//FIXME
	//cv::normalize(hist.mat, hist.mat, 1, 0, cv::NORM_L1);

    //create an 8 bits single channel image to hold the histogram
    //paint it white

CvHistogram* hist2 = 0;
CvArr* bins2 = cvLoad( "hist.xml" );
int new_hist_type = CV_IS_MATND(bins2) ? CV_HIST_ARRAY :
CV_HIST_SPARSE;

int dummy_size = 5;
hist2 = cvCreateHist( 1, &dummy_size, new_hist_type, 0, 1 );

if( new_hist_type == CV_HIST_ARRAY )
{
cvReleaseData( hist2->bins );
hist2->mat = *(CvMatND*)bins2;
hist2->bins = &hist2->mat;
cvIncRefData( hist2->bins );

}
else
{
cvReleaseSparseMat( (CvSparseMat**)&hist2->bins );
hist2->bins = bins2;
}

    printf("CV_COMP_CORREL: %3.5f\n",cvCompareHist(hist,hist2,CV_COMP_CORREL));

    imgHistogram = cvCreateImage(cvSize(bins, 300),8,1);
    cvRectangle(imgHistogram, cvPoint(0,0), cvPoint(256,300), CV_RGB(255,255,255),-1);

    //draw the histogram :P
    for(int i=0; i < bins; i++){
            value = cvQueryHistValue_1D( hist, i);
            normalized = cvRound(value*300/max_value);
            cvLine(imgHistogram,cvPoint(i,300), cvPoint(i,300-normalized), CV_RGB(0,0,0));
    }
    cvReleaseHist( &hist );

    //show the image results
    cvShowImage( "original", image );
    cvShowImage( "gray", gray );
    cvShowImage( "histogram", imgHistogram );

    cvWaitKey();

    return 0;
}

