#ifdef _WIN64
#include "stdafx.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
#endif
#ifdef _WIN64
//#include <qhyccd.h>
#endif


/*
 * modified from dynrange.cpp
 * 
 * Open saved images from dynrange.bin 
 * Calculate SNR with and without binning
 *  replicating snrofim.m 
 * 
 * Hari Nandakumar
 * 14 Mar 2018
 * */

//#define _WIN64
//#define __unix__
// the respective compilers do this defining part automatically

#include <stdio.h>
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
//#include <libqhy/qhyccd.h>
#endif

#include <string.h>

#include <time.h>
#include <sys/stat.h>
// this is for mkdir

 

#include <opencv2/opencv.hpp>
// used the above include when imshow was being shown as not declared
// removing
// #include <opencv/cv.h>
// #include <opencv/highgui.h>
 
#include <opencv2/plot.hpp>

using namespace cv;

double myStdDev(Mat m, Scalar mean, int w, int h)
{
	//meanStdDev seems to have a bug in opencv 3.3.1 & 3.4.0
	//so a workaround for single channel
	
	// Caution! currently implemented only for *single channel* Mat
	// of type CV_8U, CV_8S, CV_16U, CV_16S and CV_64F only!!
	
	double dstddev = 0.0;
	double dmean = mean(0); // only one channel
	
	int typeofmat = m.type();
	
	switch (typeofmat)
	{
		case CV_16UC1:
	
			for (int i = 0; i < w*h; i++)
			{
				if (dmean>m.at<ushort>(i))
				dstddev += (dmean - m.at<ushort>(i)) * (dmean - m.at<ushort>(i));
				else
				dstddev += (m.at<ushort>(i)-dmean) * (m.at<ushort>(i)-dmean);
			}
			break;
		
		case CV_16SC1:
		
			for (int i = 0; i < w*h; i++)
			{
				if (dmean>m.at<short>(i))
				dstddev += (dmean - m.at<short>(i)) * (dmean - m.at<short>(i));
				else
				dstddev += (m.at<short>(i)-dmean) * (m.at<short>(i)-dmean);
			}
			break;
		
		case CV_8UC1:
			
			for (int i = 0; i < w*h; i++)
			{
				if (dmean>m.at<uchar>(i))
				dstddev += (dmean - m.at<uchar>(i)) * (dmean - m.at<uchar>(i));
				else
				dstddev += (m.at<uchar>(i)-dmean) * (m.at<uchar>(i)-dmean);
			}
			break;
		
		case CV_8SC1:
		
			for (int i = 0; i < w*h; i++)
			{
				if (dmean>m.at<char>(i))
				dstddev += (dmean - m.at<char>(i)) * (dmean - m.at<char>(i));
				else
				dstddev += (m.at<char>(i)-dmean) * (m.at<char>(i)-dmean);
			}
			break;
		
		case CV_64FC1:
		
			for (int i = 0; i < w*h; i++)
			{
				if (dmean>m.at<double>(i))
				dstddev += (dmean - m.at<double>(i)) * (dmean - m.at<double>(i));
				else
				dstddev += (m.at<double>(i)-dmean) * (m.at<double>(i)-dmean);
			}
			break;
		
		default:
		
			dstddev=0;
		
	}

	dstddev = sqrt(dstddev / (w*h));
	return dstddev;				
}




int main(int argc,char *argv[])
{
	double avgpixvalue[60];
	int exptime[60];
	double snrvalues[5];
	double snrvaluesb[5];
	double snrxaxis[5];
	double stddev, meanv; 
	char dataFileName[80];
	Mat a,b,abin,bbin, subtractedframe, subtractedframeb, tempframe;
	Scalar meanvec, stdvec;
	Mat q0, q0x, plot_result;
	Mat q1,  plot_result1;
	Mat q2,  plot_result2;
	std::ofstream outfile("outputbsi.m");	
	
	int w=640;
	int h=480;
	
	//abin=Mat::zeros(cv::Size(w, h), CV_64F);
	//bbin=abin;
	namedWindow( "Plot1", WINDOW_AUTOSIZE );// Create a window for display.
	namedWindow( "Plot", WINDOW_AUTOSIZE );// Create a window for display.
	moveWindow("Plot1", 20, 0);
	moveWindow("Plot", 700, 300);
	
	for (int k=1; k<6;k++)
	{
		
		 sprintf(dataFileName , "DR%03d.png", k*10);
		 a = imread(dataFileName, CV_LOAD_IMAGE_ANYDEPTH);
		 //a = a.convertTo();
		 resize(a, abin, Size(), 0.25, 0.25, INTER_AREA);
		 sprintf(dataFileName , "DR%03d.png", k*10+1);
		 b = imread(dataFileName, CV_LOAD_IMAGE_ANYDEPTH);
		 resize(b, bbin, Size(), 0.25, 0.25, INTER_AREA);
		 
		 /*b4=imresize(b,0.25,'bilinear'); % this is not enough in Octave.
		 % imresize apparently is not doing averaging / binning.
		 % so we explicitly do binning / averaging as below
		 for l=1:120
			for m=1:160
			   a4(l,m)=a(l*4,m*4)+a(l*4-1,m*4)+a(l*4-2,m*4)+a(l*4-3,m*4) ...
				 +a(l*4,m*4-1)+a(l*4-1,m*4-1)+a(l*4-2,m*4-1)+a(l*4-3,m*4-1) ...
				 +a(l*4,m*4-2)+a(l*4-1,m*4-2)+a(l*4-2,m*4-2)+a(l*4-3,m*4-2) ...
				 +a(l*4,m*4-3)+a(l*4-1,m*4-3)+a(l*4-2,m*4-3)+a(l*4-3,m*4-3);
			  b4(l,m)=b(l*4,m*4)+b(l*4-1,m*4)+b(l*4-2,m*4)+b(l*4-3,m*4) ...
				 +b(l*4,m*4-1)+b(l*4-1,m*4-1)+b(l*4-2,m*4-1)+b(l*4-3,m*4-1) ...
				 +b(l*4,m*4-2)+b(l*4-1,m*4-2)+b(l*4-2,m*4-2)+b(l*4-3,m*4-2) ...
				 +b(l*4,m*4-3)+b(l*4-1,m*4-3)+b(l*4-2,m*4-3)+b(l*4-3,m*4-3);
			  end
		 end
		 a4=a4./16;
		 b4=b4./16;*/
			  
		 
		 subtract(a, b, subtractedframe, noArray(), CV_64F);
		 subtractedframe = subtractedframe.reshape(1);
		 tempframe 		 = a.reshape(1);
		 tempframe.convertTo(tempframe,CV_64F);
		 
		 //meanStdDev(subtractedframe, meanvec, stdvec);
		 meanvec = mean(subtractedframe);
		 stddev=myStdDev(subtractedframe, meanvec, w, h);
		 
		 meanvec = mean(tempframe);
		 meanv=meanvec(0);
									
		 snrvalues[k-1]= 2.*meanv/stddev;
		 snrxaxis[k-1]=meanv;
		 
		 subtract(abin, bbin, subtractedframeb, noArray(), CV_64F);
		 subtractedframeb = subtractedframeb.reshape(1);
		 
		 //meanStdDev(subtractedframe, meanvec, stdvec);
		 meanvec = mean(subtractedframeb);
		 stddev=myStdDev(subtractedframeb, meanvec, w/4, h/4);
		 
		 snrvaluesb[k-1]= 2.*meanv/stddev;
		
		//imshow( "show", a );                   // Show our image inside it.
		//waitKey(0);                                          // Wait for a keystroke in the window
		
	}
	
	 /*
		 
		figure;
		plot(snrxaxis,sqrt(snrxaxis),'k');
		hold on;
		plot(snrxaxis,snrvalues,'.');
		plot(snrxaxis,snrvalues4,'o');
		title('SNR');
		xlabel('Average Pixel value');
		ylabel('SNR');
		legend('Shot noise limit', 'Without binning', '4x4 binned', "location", 'northwest');    
		*/
		
    q0x = Mat(1, 5, CV_64F, snrxaxis).clone() ;
    q0  = Mat(1, 5, CV_64F, snrvalues).clone() ;
    q1  = Mat(1, 5, CV_64F, snrvaluesb).clone() ;
	Ptr<plot::Plot2d> plotg = plot::Plot2d::create( q0x, q0 );
		//plot has y going downwards by default, so 
		plotg->setInvertOrientation(1);
		plotg->setMinY(0.0);
		plotg->setMaxY(300.0);
		plotg->setShowText(0);
		plotg->setPlotLineColor( Scalar( 0, 0, 255 ));
	plotg->render(plot_result);
	imshow( "Plot", plot_result );
	waitKey(0);                                          // Wait for a keystroke in the window */
	
	plotg = plot::Plot2d::create( q0x, q1 );
		//plot has y going downwards by default, so 
		plotg->setInvertOrientation(1);
		plotg->setMinY(0.0);
		plotg->setMaxY(300.0);
		plotg->setShowText(0);
		plotg->setPlotLineColor( Scalar( 0, 255, 255 ));
	plotg->render(plot_result1);
	imshow( "Plot1", plot_result1  );
	waitKey(0); 
	
	sqrt(q0x,q2);
	plotg = plot::Plot2d::create( q0x, q2 );
		//plot has y going downwards by default, so 
		plotg->setInvertOrientation(1);
		plotg->setMinY(0.0);
		plotg->setMaxY(300.0);
		plotg->setShowText(0);
		plotg->setPlotLineColor( Scalar( 255, 255, 255 ));
	plotg->render(plot_result2);
	imshow( "Plot2", plot_result2  );
	waitKey(0);  
	imshow( "Added", plot_result + plot_result1 + plot_result2  );     
	waitKey(0);
	
	/*
	std::cout<<"x is "<<q0x<<std::endl;
	std::cout<<"x is "<<snrxaxis[0]<<","<<snrxaxis[1]<<","<<snrxaxis[2]<<","<<snrxaxis[3]<<","<<snrxaxis[4]<<std::endl;
	
	std::cout<<"y is "<<q0<<std::endl;	
    std::cout<<"y is "<<snrvalues[0]<<","<<snrvalues[1]<<","<<snrvalues[2]<<","<<snrvalues[3]<<","<<snrvalues[4]<<std::endl;
	*/
	
	outfile<<"csnrxaxis="<<q0x<<std::endl;
	outfile<<"csnrvalues="<<q0<<std::endl;
	outfile<<"csnrvaluesb="<<q1<<std::endl;
	 
	
	return 0;

}
