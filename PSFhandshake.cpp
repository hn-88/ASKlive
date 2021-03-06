#ifdef _WIN64
#include "stdafx.h"
// anything before a precompiled header is ignored, 
// so no endif here! add #endif to compile on __unix__ !
#endif
#ifdef _WIN64
#include <qhyccd.h>
#endif


/*
 * modified from ASKlive.cpp
 * 
 * modified from camexppixbinned.cpp
 * and LiveFrameSampleOCV.cpp
 * 
 * Axial PSF 
 * for TD OCT
 * with single frames and binning
 * and inputs from ini file.
 * 
 * Captures frames on receipt of 's' character via serial port 
 * (from Arduino)
 * and sends back an 'a' character after finishing capture.
 * 
 * Hari Nandakumar
 * 11 Apr 2018
 * 
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
 
 
#include <opencv2/plot.hpp>

// from https://github.com/todbot/arduino-serial/
//      http://todbot.com/blog/2013/04/29/arduino-serial-updated/

#include "arduino-serial-lib.c"
#include "arduino-serial-lib.h"



using namespace cv;

int main(int argc,char *argv[])
{
    int num = 0;
    qhyccd_handle *camhandle;
    int ret;
    char id[32];
    char camtype[16];
    int found = 0;
    unsigned int w,h,bpp=8,channels,capturenumber, cambitdepth=16, numofframes=100;
    unsigned int opw, oph, offsetx, offsety, whitevalue; 
    unsigned int numofm1slices=10, numofm2slices=10, firstaccum, secondaccum;
     
    int camtime = 1,camgain = 1,camspeed = 1,cambinx = 2,cambiny = 2,usbtraffic = 10;
    int camgamma = 1, indexi, indexbk;
    
    std::ofstream outfile("PSFoutput.m");
    bool doneflag=0, skeypressed=0, bkeypressed=0;
    
    w=640;
    h=480;
    
    int  fps, key, xpos, ypos, binvalue;
    int t_start,t_end;
    const char* cack = "a";
    char* serialinput;
    int serialdevice = serialport_init("/dev/ttyACM0", 115200);
    int serialreturn;
    
    std::ifstream infile("PSFlive.ini");
    std::string tempstring;
    
    
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
			infile >> xpos;
			infile >> tempstring;
			infile >> ypos;
			infile >> tempstring;
			infile >> binvalue;
			infile >> tempstring;
			infile >> offsetx;
			infile >> tempstring;
			infile >> offsety;
			infile.close();
		  }

		  else std::cout << "Unable to open ini file, using defaults.";
	   
	   
    ///////////////////////
    
    firstaccum = numofframes;
    secondaccum = firstaccum;
    cambitdepth = bpp;
    

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
              

        ret = SetQHYCCDResolution(camhandle,offsetx,offsety, w, h); //handle, xpos,ypos,xwidth,ywidth
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
         
        namedWindow("show",0); // 0 = WINDOW_NORMAL
        moveWindow("show", 20, 0);
        namedWindow("plot",0); // 0 = WINDOW_NORMAL
        moveWindow("plot", 400, 300);
        
        Mat m, opm, dispm;
        Mat plot_result;
        Mat data_x( 1, numofm1slices, CV_64F );
        Mat data_y( 1, numofm1slices, CV_64F );
        
        for ( int i = 0; i < data_x.cols; i++ )
		{
			data_x.at<double>( 0, i ) = i;
			data_y.at<double>( 0, i ) = 178.0;
		}

        
        Point pt0, pt1, pt2;
        pt0 = Point(xpos, ypos);
        pt1 = Point(xpos-1, ypos-1);
        pt2 = Point(xpos+1, ypos+1);
        
        //Mat slice[numofm1slices];      // array of n images
        // not allowed on Windows, so making it a constant
        Mat slice[1000];
        unsigned int pixvalue[1000];
         
        opw = w/binvalue;
        oph = h/binvalue;
        
        for (indexi=0; indexi<numofm1slices; indexi++)
        {
			slice[indexi] = Mat::zeros(cv::Size(opw, oph), CV_32F); 
		}
		 
        
        if (cambitdepth==8)
        {
			
			m  = Mat::zeros(cv::Size(w, h), CV_8U);  
			whitevalue = 255;  
		}
        else // is 16 bit
        {
			m  = Mat::zeros(cv::Size(w, h), CV_16U);
			whitevalue = 65535;
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
        outfile<<"%Data cube in MATLAB compatible format - m(h,w,slice)"<<std::endl;
        
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
            resize(m, opm, Size(), 1.0/binvalue, 1.0/binvalue, INTER_AREA);	// binvalue x binvalue binning (averaging)
            dispm=opm;
          
            // create a rectangle around the pixel used for axial PSF
            // just for display
            
            rectangle(dispm, pt1, pt2, Scalar(whitevalue) );
            
            // plot the PSF
            
            Ptr<plot::Plot2d> plot = plot::Plot2d::create( data_x, -data_y );
                //plot has y going downwards by default, so -data_y
                //plot->setMinY(-255.0);
                //plot->setMaxY(0.0);
                plot->render(plot_result);
                imshow( "plot", plot_result );
                
            
            imshow("show",dispm);
            
            fps++;
            t_end = time(NULL);
                if(t_end - t_start >= 5)
                {
                    printf("fps = %d\n",fps / 5); 
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
							{
								slice[indexi].convertTo(slice[indexi], CV_16U, 1.0/numofframes);			// these were just accumulated,
													// dividing by numofframes to get average value
								pixvalue[indexi]=slice[indexi].at<ushort>(xpos,ypos);
								data_y.at<double>( 0, indexi ) = pixvalue[indexi];
							}
							
							if (cambitdepth==8)
							{
								slice[indexi].convertTo(slice[indexi], CV_8U, 1.0/numofframes);	
								pixvalue[indexi]=	slice[indexi].at<uchar>(xpos,ypos);
								data_y.at<double>( 0, indexi ) = pixvalue[indexi];
							}
							
							//outfile<<"%Data cube in MATLAB compatible format - m(w,h,slice)"<<std::endl;
							// imagesc(slice(:,:,1)) shows the same image as is seen onscreen
				
							/*
							 * outfile<<"slice(:,:,";
							outfile<<indexi+1;		// making the output starting index 1 instead of 0		
							outfile<<")=";
							outfile<<slice[indexi];
							outfile<<";"<<std::endl;*/
							
							char filename[80];
							sprintf(filename, "slice%03d.png",indexi+1);
							imwrite(filename, slice[indexi]);
							
							// tell the arduino that acquisition is done
							serialreturn = serialport_write(serialdevice,cack);
							// does not work yet. Some serial settings issue maybe. 
							
							indexi++;
							if (indexi < numofm1slices)
							firstaccum=numofframes;							// ready to acquire next set
							else
							doneflag=1;
							
							//printf("Frame acquisition %d done.\n", indexi);	// indexi+1 so that this starts from 1
							
									
						}
							
				
					 
					 
                // read serial port for 's' character, waiting upto 100 msec
                serialreturn = serialport_read_until(serialdevice, serialinput, 's', 1, 100);
                // returns 0 if success
                if (serialreturn == 0) 
					key='s';
					
				key=waitKey(1); // wait 1 milliseconds for keypress
                
                if (key == 27) // ESC key
                {
					doneflag=1;
				}
                
                if (key == 's')
                if (indexi < numofm1slices)	// don't allow captures after the end of assigned number of slices
                skeypressed=1;
                
            } 
            
            if(doneflag==1)
			{
				
				numofm1slices=indexi;		// in case acquisition was aborted, no need to export a lot of zeros
				
				outfile<<"pixvalue=[";
				for (int i = 0; i<numofm1slices-1; i++)
					outfile<<pixvalue[i]<<", ";
				outfile<<pixvalue[numofm1slices-1]; 
				outfile<<"];";
                break;
			}   

        } // while loop end
        
         
         
		
    } // end of if found
    else
    {
        printf("The camera is not QHYCCD or other error \n");
        goto failure;
    }
    
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

