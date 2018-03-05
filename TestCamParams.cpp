/*
 * modified from camexppixbinned.cpp
 * and LiveFrameSampleOCV.cpp
 * 
 * Implementing ASK background subtraction
 * for TD OCT
 * with averaging over multiple frames
 * and inputs from ini file.
 * 
 * Captures frames on receipt of s key 
 * (from Arduino emulating a keyboard using
 * KeyboardWrite function.)
 * 
 * Hari Nandakumar
 * 17 Feb 2018
 * modified 21 Feb to write inside acquisition loop
 * 			22 Feb to display live updating window with processing
 * 					including hilbert transform
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libqhy/qhyccd.h>
#include <libqhy/qhy5ii.h>
#include <sys/time.h>
#include <sys/stat.h>
// this is for mkdir

 

#include <opencv2/opencv.hpp>
// used the above include when imshow was being shown as not declared
// removing
// #include <opencv/cv.h>
// #include <opencv/highgui.h>
 

using namespace cv;

Mat Hilbert(Mat m)
{
	//   hilbert transform algorithm from http://in.mathworks.com/help/signal/ref/hilbert.html
	
		//1.  It calculates the FFT of the input sequence, storing the result in a vector x.

		//2. It creates a vector h whose elements h(i) have the values:

        //		1 for i = 1, (n/2)+1 - in our case, 0 and n/2 since our array is zero indexed.

        //		2 for i = 2, 3, ... , (n/2)

        //		0 for i = (n/2)+2, ... , n

		//3. It calculates the element-wise product of x and h.

		//4. It calculates the inverse FFT of the sequence obtained in step 3 and returns the first n elements of the result.
		
	// written for single channel only
	// does not optimize DFT size by zero padding - assumes it is already optimal size.
	
	m.convertTo(m,CV_64F);		// work with <double> single channel
	Mat x, h, prod, hilb;	
	dft(m, x, DFT_ROWS + DFT_COMPLEX_OUTPUT);		// Fourier transform rowwise
							// output is of type CV_64FC2 - two channels because complex
	Mat h1[2];
	h1[0] = m;
	h1[1] = m;
	int i;
	
	h1[0].col(0)		= 1;
	h1[0].col(m.cols/2)	= 1;
	h1[1].col(0)		= 1;
	h1[1].col(m.cols/2)	= 1;
	
	for (i=1; i<m.cols/2; i++)
	{
		h1[0].col(i)=2;
		h1[1].col(i)=2;
	}
		
	
	for (i=m.cols/2+1; i<m.cols; i++)
	{
		h1[0].col(i)=0;
		h1[1].col(i)=0;
	}
		
	merge(h1, 2, h);
		
	multiply(x, h, prod);
	
	dft(prod, hilb, DFT_INVERSE + DFT_ROWS + DFT_COMPLEX_OUTPUT);		// Inv Fourier transform rowwise
	
	return hilb;				// output is of type CV_64FC2 - two channels because complex
}


int main(int argc,char *argv[])
{
    int num = 0;
    qhyccd_handle *camhandle;
    int ret;
    char id[32];
    char camtype[16];
    int found = 0;
    unsigned int w,h,bpp=8,channels,capturenumber, cambitdepth=16, numofframes=100; 
    unsigned int numofm1slices=10, numofm2slices=10, firstaccum, secondaccum;
     
    int camtime = 1,camgain = 1,camspeed = 1,cambinx = 2,cambiny = 2,usbtraffic = 10;
    int camgamma = 1, indexi, indexbk;
    double min, max, step;
     
    bool doneflag=0, skeypressed=0, bkeypressed=0;
    
    w=640;
    h=480;
    
    int  fps, key, bscanat;
    struct timeval tval_before, tval_after, tval_result;
     
    std::string tempstring;
    
   
    

    ret = InitQHYCCDResource();
    if(ret != QHYCCD_SUCCESS)
    {
        printf("Init SDK not successful!\n");
    }
    
    num = ScanQHYCCD();
    if(num > 0)
    {
        printf("Found QHYCCD,the num is %d \n",num);
    }
    else
    {
        printf("QHYCCD camera not found, please check the usb cable.\n");
        goto failure;
    }

    for(int i = 0;i < num;i++)
    {
        ret = GetQHYCCDId(i,id);
        if(ret == QHYCCD_SUCCESS)
        {
            printf("connected to the first camera from the list,id is %s\n",id);
            found = 1;
            break;
        }
    }

    if(found == 1)
    {
        camhandle = OpenQHYCCD(id);
        if(camhandle != NULL)
        {
            //printf("Open QHYCCD success!\n");
        }
        else
        {
            printf("Open QHYCCD failed \n");
            goto failure;
        }
        
        //ret = QHY5II::GetControlMinMaxStepValue(CONTROL_BRIGHTNESS, &min, &max, &step);
        if(ret == QHYCCD_SUCCESS)
			{
				printf("GetControlMinMaxStepValue success!\n");
			}
			else
			{
				goto failure;
			}

        
	}
	
	
    ret = ReleaseQHYCCDResource();
    if(ret == QHYCCD_SUCCESS)
    {
        printf("Release SDK Resource  success!\n");
    }
    else
    {
        goto failure;
    }

    return 0;

failure:
    printf("Fatal error !! \n");
    return 1;
}
