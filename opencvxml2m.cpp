#ifdef _WIN64
#include "stdafx.h"
#include "windows.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
#endif


/*
 * 
 * Opens OpenCV FileStorage xml file, saves data as matlab compatible m file
 * 
 * Hari Nandakumar
 * 15 May 2018  
 */

//#define _WIN64
//#define __unix__

#include <stdio.h>
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
#endif
#include <fstream>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
// this is for mkdir

#include <opencv2/opencv.hpp>


int main()
{

//  using idea from
// https://stackoverflow.com/questions/27697451/how-to-convert-an-opencv-mat-that-has-been-written-in-an-xml-file-back-into-an-i/

	cv::Mat m, bscan;
	int camgain, camtime, normfactor;
	cv::FileStorage fs("ASKoutput.xml", cv::FileStorage::READ);
	std::ofstream outfile("ASKxml2m.m");
	char stringvar[80];
	
	fs["camgain"] >> camgain;
	fs["camtime"] >> camtime;
	fs["normfactor"] >> normfactor;
	fs["bscan"] >> bscan;
	
	
	for (int indexi = 1; indexi<101; indexi++)
	{
		sprintf(stringvar, "absvalue%d", indexi);	
		fs[stringvar] >> m;
		outfile<<"absvalue(:,:,";
		outfile<<indexi;		
		outfile<<")=";
		outfile<< m;
		outfile<<";"<<std::endl;
	}
	
	outfile<<"bscan=";
	outfile<< bscan;
	outfile<<";"<<std::endl;
	
	outfile<<"camgain=";
	outfile<< camgain;
	outfile<<";"<<std::endl;
	
	outfile<<"camtime=";
	outfile<< camtime;
	outfile<<";"<<std::endl;
	
	outfile<<"normfactor=";
	outfile<< normfactor;
	outfile<<";"<<std::endl;
	
    return 0;
}
