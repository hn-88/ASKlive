// im10xbin.cpp : Defines the entry point for the console application.
//

#ifdef _WIN64
#include "stdafx.h"
#include "windows.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
// #endif



/*
* opens image file, does 10x10 binning, saves it to filename10x.png
* 
* Hari Nandakumar
* 18 May 2018
*  
*/

//#define _WIN64
//#define __unix__

#include <stdio.h>
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
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


using namespace cv;

std::string remove_extension(const std::string& filename) {
	size_t lastdot = filename.find_last_of(".");
	if (lastdot == std::string::npos) return filename;
	return filename.substr(0, lastdot);
}


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		std::cout << " Usage: im10xbin ImageToLoadAndBin" << std::endl;
		return -1;
	}

	Mat image, opm;
	image = imread(argv[1]);					  // Read the file

	if (!image.data)                              // Check for invalid input
	{
		std::cout << "Could not open or find the image" << std::endl;
		return -1;
	}

	resize(image, opm, Size(), 1.0 / 10, 1.0 / 10, INTER_AREA);	// binning (averaging)

	std::string fullfilename(argv[1]);
	std::string newfilename;
	newfilename = remove_extension(fullfilename);
	newfilename = newfilename + "10x.png";
	imwrite(newfilename, opm);

    return 0;
}

