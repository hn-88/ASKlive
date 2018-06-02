#ifdef _WIN64
#include "stdafx.h"
#include "windows.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
#endif
#ifdef _WIN64
#include <qhyccd.h>
#endif


/*
 * modified from camexppixbinned.cpp
 * and LiveFrameSampleOCV.cpp
 * adding software binning from
 * binsavedimages.cpp
 * 
 * Implementing ASK background subtraction
 * for TD OCT
 * with averaging over multiple frames
 * and inputs from ini file.
 * 
 * Captures frames on receipt of s key 
 * and b for background.
 * (from Arduino emulating a keyboard using
 * KeyboardWrite function.)
 * 
 * + key increases exposure time by 0.1 ms
 * - key decreases exposure time by 0.1 ms
 * u key increases exposure time by 1 ms
 * d key decreases exposure time by 1 ms
 * U key increases exposure time by 10 ms
 * D key decreases exposure time by 10 ms
 * ESC key quits
 * 
 * Hari Nandakumar
 * 17 Feb 2018
 * 15 Mar 2018 for ASKlivebin
 
 * modified 21 Feb to write inside acquisition loop
 * 			22 Feb to display live updating window with processing
 * 					including hilbert transform
 *          05 Mar - cross-platform changes - ifdef directives
 * 			15 Mar - adding 4x4 binning code
 * 			16 Apr - generalized binning code, saving to dir with timestamp
 * 			17 Apr - vary exp time with + and -, switch case instead of if statements
 * 			29 Mar - normalizing the hilbert function's output
 * 			
 *			01 May - adding offset, save dir
 * 			04 May - removed reload config code
 * 			05 May - merge with ASKlivebin.cpp
 *			07 May - Visual Studio changes to compile on Windows
 * 			09 May - bug fix assert same size error in accumulate
 * 			10 May - bug fix - need to multiply by 255 (or normalizing factor) before imwrite
 * 			17 May - adding max intensity projection - MIP
 * 			 2 Jun - cleaning up the case statement, adding faster exp changes
 */

//#define _WIN64
//#define __unix__

#include <stdio.h>
#include <stdlib.h>

#ifdef __unix__
#include <unistd.h>
#include <libqhy/qhyccd.h>
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
	// does row-wise Hilbert transform and not column-wise as in Matlab.
	
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
	
	hilb.convertTo(hilb, CV_64F, 1.0 / (1.1*m.cols) );	// normalizing the transform
	
	return hilb;				// output is of type CV_64FC2 - two channels because complex
}


int main(int argc,char *argv[])
{
    int num = 0;
    qhyccd_handle *camhandle=NULL;
    int ret;
    char id[32];
    //char camtype[16];
    int found = 0;
    unsigned int w,h,bpp=8,channels, cambitdepth=16, numofframes=100; 
    unsigned int numofm1slices=10, numofm2slices=10, firstaccum, secondaccum;
    unsigned int offsetx=0, offsety=0;
    unsigned int indexi, indexbk, opw, oph;

     
    int camtime = 1,camgain = 1,camspeed = 1,cambinx = 2,cambiny = 2,usbtraffic = 10;
    int camgamma = 1, binvalue=1, normfactor=1, normfactorforsave=255;
    
     
    bool doneflag=0, skeypressed=0, bkeypressed=0;
    
    w=640;
    h=480;
    
    int  fps, key, bscanat;
    int t_start,t_end;
    
    std::ifstream infile("ASKlive.ini");
    std::string tempstring;
    char dirdescr[60];
    sprintf(dirdescr, "_");
     
	namedWindow("show",0); // 0 = WINDOW_NORMAL
	moveWindow("show", 20, 0);
	namedWindow("result",0); // 0 = WINDOW_NORMAL
	moveWindow("result", 400, 0);
	namedWindow("Bscan",0); // 0 = WINDOW_NORMAL
	moveWindow("Bscan", 800, 0);
	namedWindow("MIP",0); // 0 = WINDOW_NORMAL
	moveWindow("MIP", 1200, 0);
	
	Mat m, opm, opmvector, hilb, xsquared, ysquared, absvalue, bscan, mipscan;
	//Mat slice[numofm1slices];      // array of n images
	// not allowed on Windows, so making it a constant
	Mat slice[1000];
	Mat bk[1000];      // array of n images
	Mat res[1000];
	Mat realcomplex[2];			// for splitting complex into real and complex
	double minVal, maxVal, pixVal;
	//minMaxLoc( m, &minVal, &maxVal, &minLoc, &maxLoc );
	
	
	char dirname[80];
	char filename[20];
	char pathname[40];
	struct tm *timenow;
	
	time_t now = time(NULL);
	
    // inputs from ini file
    if (infile.is_open())
		  {
			
			infile >> tempstring;
			infile >> tempstring;
			infile >> tempstring;
			// first three lines of ini file are comments
			infile >> camgain  ;
			infile >> tempstring;
			infile >> camtime  ;
			infile >> tempstring;
			infile >> bpp  ;
			infile >> tempstring;
			infile >> w  ;
			infile >> tempstring;
			infile >> h  ;
			infile >> tempstring;
			infile >> camspeed  ;
			infile >> tempstring;
			infile >> cambinx  ;
			infile >> tempstring;
			infile >> cambiny  ;
			infile >> tempstring;
			infile >> usbtraffic;
			infile >> tempstring;
			infile >> numofframes;
			infile >> tempstring;
			infile >> numofm1slices;
			infile >> tempstring;
			infile >> numofm2slices;
			infile >> tempstring;
			infile >> bscanat;
			infile >> tempstring;
			infile >> binvalue;
			infile >> tempstring;
			infile >> offsetx;
			infile >> tempstring;
			infile >> offsety;
			infile >> tempstring;
			infile >> dirdescr;
			infile >> tempstring;
			infile >> normfactor;
			infile.close();
		  }

		  else std::cout << "Unable to open ini file, using defaults.";
	   
	   
    ///////////////////////
    // init camera etc
    
    firstaccum = numofframes;
    secondaccum = firstaccum;
    cambitdepth = bpp;
    opw = w/binvalue;
	oph = h/binvalue;
	normfactorforsave = 255*normfactor;
    timenow = localtime(&now);
	
	strftime(dirname, sizeof(dirname), "%Y-%m-%d_%H_%M_%S-", timenow);

	strcat(dirname, dirdescr);
#ifdef _WIN64
	CreateDirectoryA(dirname, NULL);
	cv::FileStorage outfile;
	outfile.open("ASKoutput.xml", cv::FileStorage::WRITE);
#else
	mkdir(dirname, 0755);
#endif

#ifdef __unix__	
	sprintf(filename, "ASKoutput.m");
	strcpy(pathname,dirname);
	strcat(pathname,"/");
	strcat(pathname,filename);
	std::ofstream outfile(pathname);
#endif
    
    

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
    
    if(found != 1)
    {
        printf("The camera is not QHYCCD or other error \n");
        goto failure;
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
        ret = SetQHYCCDStreamMode(camhandle,1);
    

        ret = InitQHYCCD(camhandle);
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("Init QHYCCD success!\n");
        }
        else
        {
            printf("Init QHYCCD fail code:%d\n",ret);
            goto failure;
        }
        
        
        
       ret = IsQHYCCDControlAvailable(camhandle,CONTROL_TRANSFERBIT);
        if(ret == QHYCCD_SUCCESS)
        {
            ret = SetQHYCCDBitsMode(camhandle,cambitdepth);
            if(ret != QHYCCD_SUCCESS)
            {
                printf("SetQHYCCDBitsMode failed\n");
                
                getchar();
                return 1;
            }

           
                     
        }  
              

        ret = SetQHYCCDResolution(camhandle,0,0, w, h); //handle, xpos,ypos,xwidth,ywidth
        if(ret == QHYCCD_SUCCESS)
        {
            printf("SetQHYCCDResolution success - width = %d !\n", w); 
        }
        else
        {
            printf("SetQHYCCDResolution fail\n");
            goto failure;
        }
        
        //ret = SetQHYCCDBinMode(camhandle,cambinx, cambiny); 
        // this seems to cause crosshatching in image with old Linux SDK
        /////////////////////////////////////////////////////////////
        //if(ret == QHYCCD_SUCCESS)
        //{
            ////printf("SetQHYCCDBinMode success - width = %d !\n", w); 
        //}
        //else
        //{
            //printf("SetQHYCCDBinMode fail\n");
            //goto failure;
        //}
        
        
        ret = SetQHYCCDParam(camhandle, CONTROL_USBTRAFFIC, usbtraffic); //handle, parameter name, usbtraffic (which can be 0..100 perhaps)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_USBTRAFFIC success!\n");
        }
        else
        {
            printf("CONTROL_USBTRAFFIC fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_SPEED, camspeed); //handle, parameter name, speed (which can be 0,1,2)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_CONTROL_SPEED success!\n");
        }
        else
        {
            printf("CONTROL_CONTROL_SPEED fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_EXPOSURE success!\n");
        }
        else
        {
            printf("CONTROL_EXPOSURE fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_GAIN, camgain); //handle, parameter name, gain (which can be 0..99)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_GAIN success!\n");
        }
        else
        {
            printf("CONTROL_GAIN fail\n");
            goto failure;
        }
        
        ret = SetQHYCCDParam(camhandle, CONTROL_GAMMA, camgamma); //handle, parameter name, gamma (which can be 0..2 perhaps)
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("CONTROL_GAMMA success!\n");
        }
        else
        {
            printf("CONTROL_GAMMA fail\n");
            goto failure;
        }
        
        for (indexi=0; indexi<numofm1slices; indexi++)
        {
			slice[indexi] = Mat::zeros(cv::Size(opw, oph), CV_32F); 
		}
		for (indexbk=0; indexbk<numofm2slices; indexbk++)
        {
			bk[indexbk] = Mat::zeros(cv::Size(opw, oph), CV_32F); 
		}
		
		bscan = Mat::zeros(cv::Size(opw, numofm2slices), CV_64F);
		mipscan = Mat::zeros(cv::Size(opw, numofm2slices), CV_64F);
        
        if (cambitdepth==8)
        {
			
			m  = Mat::zeros(cv::Size(w, h), CV_8U);    
		}
        else // is 16 bit
        {
			m  = Mat::zeros(cv::Size(w, h), CV_16U);
		}
		 
		
        ret = BeginQHYCCDLive(camhandle);
        if(ret == QHYCCD_SUCCESS)
        {
            printf("BeginQHYCCDLive success!\n");
            key=waitKey(300);
        }
        else
        {
            printf("BeginQHYCCDLive failed\n");
            goto failure;
        }

        /////////////////////////////////////////
        /////////////////////////////////////////
        //outfile<<"%Data cube in MATLAB compatible format - m(h,w,slice)"<<std::endl;
        
        firstaccum=numofframes;
		secondaccum=firstaccum;	
		doneflag = 0;
		
        ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
        if(ret == QHYCCD_SUCCESS)
        {
            printf("CONTROL_EXPOSURE =%d success!\n", camtime);
        }
        else
        {
            printf("CONTROL_EXPOSURE fail\n");
            goto failure;
        }
        t_start = time(NULL);
        fps = 0;
        
        indexi = 0;
        indexbk = 0;
	     
        while(1)
        { 
            ret = GetQHYCCDLiveFrame(camhandle,&w,&h,&bpp,&channels,m.data);
            
            if (ret == QHYCCD_SUCCESS)  
            {
            resize(m, opm, Size(), 1.0/binvalue, 1.0/binvalue, INTER_AREA);	// binning (averaging)
            imshow("show",opm);
            
            fps++;
            t_end = time(NULL);
                if(t_end - t_start >= 5)
                {
                    printf("fps = %d\n",fps / 5); 
                    opm.copyTo(opmvector);
                    opmvector.reshape(0,1);	//make it into a row array
                    minMaxLoc(opmvector, &minVal, &maxVal);
                    printf("Max intensity = %d\n", int(floor(maxVal)));
                    fps = 0;
                    t_start = time(NULL);
                }
            
            
                
                if (skeypressed==1)	
                if (firstaccum>0)
					{ 
						accumulate(opm, slice[indexi]);
						firstaccum--;
					}
				 
				else if (firstaccum==0)
					    {
							skeypressed=0;	// accumulation of m1 is done
							
							if (cambitdepth==16)
							slice[indexi].convertTo(slice[indexi], CV_16U, 1.0/numofframes);			// these were just accumulated,
													// dividing by numofframes to get average value
							if (cambitdepth==8)
							slice[indexi].convertTo(slice[indexi], CV_8U, 1.0/numofframes);		
							
							/* outfile<<"%Data cube in MATLAB compatible format - m(w,h,slice)"<<std::endl;
							// imagesc(slice(:,:,1)) shows the same image as is seen onscreen
				
							outfile<<"slice(:,:,";
							outfile<<indexi+1;		// making the output starting index 1 instead of 0		
							outfile<<")=";
							outfile<<slice[indexi];
							outfile<<";"<<std::endl;*/
							
							//char filename[80];
							
							sprintf(filename, "slice%03d.png",indexi+1);
#ifdef __unix__
							strcpy(pathname,dirname);
							strcat(pathname,"/");
							strcat(pathname,filename);
							imwrite(pathname, slice[indexi]);
#else
							imwrite(filename, slice[indexi]);
#endif
							
							indexi++;
							if (indexi < numofm1slices)
							firstaccum=numofframes;							// ready to acquire next set
							
							printf("Frame acquisition %d done.\n", indexi);	// indexi+1 so that this starts from 1
							
									
						}
							
				if (bkeypressed==1)
				if (secondaccum>0)
							{
								accumulate(opm, bk[indexbk]);
								secondaccum--;
							}
				else if (secondaccum==0)
								{
									bkeypressed=0;	// accumulation of bk done
									
									if (cambitdepth==16)
									bk[indexbk].convertTo(bk[indexbk], CV_16U, 1.0/numofframes);			// these were just accumulated,
													// dividing by numofframes to get average value
									if (cambitdepth==8)
									bk[indexbk].convertTo(bk[indexbk], CV_8U, 1.0/numofframes);	
									
									subtract(slice[indexbk], bk[indexbk], res[indexbk], noArray(), CV_64F);
									hilb=Hilbert(res[indexbk]);
									split(hilb, realcomplex);	
									multiply(realcomplex[0], realcomplex[0], xsquared);
									multiply(realcomplex[1], realcomplex[1], ysquared);	
									add(xsquared, ysquared, res[indexbk]);
									pow(res[indexbk], 0.5, res[indexbk]);
									res[indexbk].convertTo(res[indexbk], CV_64F, 1.0/(std::pow(2.0, cambitdepth)) ); 
														//normalize res to 0-1 range since it is a double
									/*
									outfile<<"bk(:,:,";
									outfile<<indexbk+1;		// making the output starting index 1 instead of 0		
									outfile<<")=";
									outfile<<bk[indexbk];
									outfile<<";"<<std::endl;
									*/
#ifdef __unix__
									outfile<<"absvalue(:,:,";
									outfile<<indexbk+1;		// making the output starting index 1 instead of 0		
									outfile<<")=";
									outfile<<res[indexbk];
									outfile<<";"<<std::endl;
#else 
									sprintf(filename, "absvalue%u", indexbk + 1);
									outfile<<filename<<res[indexbk];
#endif
									
									
									sprintf(filename, "bk%03d.png",indexbk+1);
#ifdef __unix__
									strcpy(pathname,dirname);
									strcat(pathname,"/");
									strcat(pathname,filename);
									imwrite(pathname, bk[indexbk]);
#else
									imwrite(filename, bk[indexbk]);
#endif
									
									sprintf(filename, "res%03d.png",indexbk+1);
#ifdef __unix__
									strcpy(pathname,dirname);
									strcat(pathname,"/");
									strcat(pathname,filename);
									imwrite(pathname, normfactorforsave*res[indexbk]);
#else
									imwrite(filename, normfactorforsave*res[indexbk]); 
#endif
									// imwrite takes the Mat value to be 0-255
									// imshow auto-scales 0-1 to 255 for CV64F
									imshow("result", normfactor*res[indexbk]);
									
									res[indexbk].row(bscanat).copyTo(bscan.row(indexbk));
									imshow("Bscan",normfactor*bscan);
									
									res[indexbk].row(bscanat).copyTo(mipscan.row(indexbk));
									
									for(int j=0; j<opw; j++)
										for (int k=0; k<oph; k++)
									{
										pixVal = res[indexbk].at<double>(k, j) ;
										if (pixVal>mipscan.at<double>(indexbk, j) )
											mipscan.at<double>(indexbk, j) = pixVal;
											
									}
									imshow("MIP",normfactor*mipscan);
									
									indexbk++;
									printf("BK acquisition %d done.\n", indexbk);
									
									
									if (indexbk < numofm2slices)
									secondaccum=numofframes;				// ready to acquire next set
									else
									doneflag=1;
								}

					 
					 
                key=waitKey(50); // wait 50 milliseconds for keypress
                
                switch (key) 
                {
                
                case 27: //ESC key
					doneflag=1;
					break;
					
					case '+':
				 
						camtime = camtime + 100;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break;
						
					case '-':
				 
						camtime = camtime - 100;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break;

					case 'U':
				 
						camtime = camtime + 10000;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break;
					case 'D':
				 
						camtime = camtime - 10000;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break;
					case 'u':
				 
						camtime = camtime + 1000;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break;
					case 'd':
				 
						camtime = camtime - 1000;
						ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, camtime); //handle, parameter name, exposure time (which is in us)
						if(ret == QHYCCD_SUCCESS)
						{
							printf("CONTROL_EXPOSURE =%d success!\n", camtime);
						}
						else
						{
							printf("CONTROL_EXPOSURE fail\n");
							goto failure;
						}
						break;
					
						
                case 's':  
					if (indexi < numofm1slices)	// don't allow captures after the end of assigned number of slices
							skeypressed=1;
					break;
                
                case 'b':  
					if (indexbk < numofm2slices)	// don't allow captures after the end of assigned number of slices
							bkeypressed=1;
					break;
					
				 
				default:
					break;
                
				} 
            
				if(doneflag==1)
				{
					
					numofm1slices=indexi;		// in case acquisition was aborted, no need to export a lot of zeros
					numofm2slices=indexbk;
					
					
					break;
				}   

			 }  // if ret success end
        } // inner while loop end
        
	} // end of if found 
        
#ifdef __unix__
        outfile<<"bscan=";
		outfile<<bscan;
		outfile<<";"<<std::endl;
		outfile<<"MIP=";
		outfile<<mipscan;
		outfile<<";"<<std::endl;
		outfile<<"% Parameters were - camgain, camtime, bpp, w , h , camspeed, usbtraffic, numofframesavg, numofm1slices, binvalue, offsetx, offsety, normfactor"<<std::endl;
				outfile<<"% "<<camgain; 
				outfile<<", "<<camtime;  
				outfile<<", "<<bpp; 
				outfile<<", "<<w ; 
				outfile<<", "<<h ; 
				outfile<<", "<<camspeed ;
				outfile<<", "<<usbtraffic ;
				outfile<<", "<<numofframes ;
				outfile<<", "<<numofm1slices ;
				outfile<<", "<<binvalue ;
				outfile<<", "<<offsetx ;
				outfile<<", "<<offsety;
				outfile<<", "<<normfactor;

		 
		strcpy(pathname,dirname);
		strcat(pathname,"/");
		strcat(pathname,"bscan.png");
		imwrite(pathname, normfactorforsave*bscan);
				
		strcpy(pathname,dirname);
		strcat(pathname,"/");
		strcat(pathname,"MIP.png");
		imwrite(pathname, normfactorforsave*mipscan);
#else
		imwrite("bscan.png", normfactorforsave*bscan);
		imwrite("MIP.png", normfactorforsave*mipscan);
		outfile << "bscan" << bscan;
		outfile << "MIP" << mipscan;
		outfile << "camgain" << camgain;
		outfile << "camtime" << camtime;
		 
		//outfile << "bpp" << bpp;
		//outfile << "w" << w;
		//outfile << "h" << h;
		//outfile << "camspeed" << camspeed;
		//outfile << "usbtraffic" << usbtraffic;
		//outfile << "numofframes" << numofframes;
		//outfile << "numofm1slices" << numofm1slices;
		//outfile << "binvalue" << binvalue;
		//outfile << "offsetx" << offsetx;
		//outfile << "offsety" << offsety;
		outfile << "normfactor" << normfactor;
		 
#endif

         
		

 
    
    if(camhandle)
    {
        StopQHYCCDLive(camhandle);

        ret = CloseQHYCCD(camhandle);
        if(ret == QHYCCD_SUCCESS)
        {
            printf("Close QHYCCD success!\n");
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
