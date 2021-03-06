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
 * modified from ASKlive.cpp
 * and LiveFrameGraph.cpp
 * 
 * Implementing Auto Focus - 
 * preliminary test with display of graph with
 * Std deviation, Max and Min pix values
 * 
 * * 
 * + key increases exposure time by 0.1 ms
 * - key decreases exposure time by 0.1 ms
 * u key increases exposure time by 1 ms
 * d key decreases exposure time by 1 ms
 * U key increases exposure time by 10 ms
 * D key decreases exposure time by 10 ms
 * ESC key quits
 * 
 * Hari Nandakumar
 * 01 Jul 2018
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

using namespace cv;


int main(int argc,char *argv[])
{
    int num = 0;
    qhyccd_handle *camhandle=NULL;
    int ret;
    char id[32];
    //char camtype[16];
    int found = 0;
    unsigned int w=1280,h=960,bpp=8,channels, cambitdepth=8; 
     
    int camtime = 1,camgain = 1,camspeed = 1,usbtraffic = 10;
    int camgamma = 1, binvalue=1;
    
     
    bool doneflag=0, skeypressed=0, bkeypressed=0;

    
    int  fps, key, bscanat;
    int t_start,t_end;
    
    std::ifstream infile("AFstats.ini");
    std::string tempstring;
    char dirdescr[60];
    sprintf(dirdescr, "_");
     
	namedWindow("show",0); // 0 = WINDOW_NORMAL
	moveWindow("show", 20, 0);
	
	Mat m, opm, opmvector;
	Scalar meansc, stddevsc;
	Mat plot_result, plot_result1, plot_result2;
	Mat data_x( 1, w/2, CV_64F );
    Mat data_y( 1, w/2, CV_64F );
    Mat data_y2( 1, w/2, CV_64F );
    
    for ( int i = 0; i < data_x.cols; i++ )
		{
			double x = ( i - data_x.cols / 2 );
			data_x.at<double>( 0, i ) = x;
			data_y.at<double>( 0, i ) = 0;
			data_y2.at<double>( 0, i ) = 0;
		}
        
	//Mat slice[numofm1slices];      // array of n images
	// not allowed on Windows, so making it a constant
	
	double minVal, maxVal,  pixVal;
	int graphindex=0;
	//minMaxLoc( m, &minVal, &maxVal, &minLoc, &maxLoc );

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
			
			infile.close();
		  }

		  else std::cout << "Unable to open ini file, using defaults.";
	   
	   
    ///////////////////////
    // init camera etc
    
    
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

        //////////////////////////////////////////
        //end of camera initialization
        //////////////////////////////////////////
        
        
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
        
        
	     
        while(1)
        { 
            ret = GetQHYCCDLiveFrame(camhandle,&w,&h,&bpp,&channels,m.data);
            
            if (ret == QHYCCD_SUCCESS)  
            {
            resize(m, opm, Size(), 0.5, 0.5, INTER_AREA);	// 2x2 binning (averaging) or else max is always 255
            imshow("show",opm);
            opm.copyTo(opmvector);
			opmvector.reshape(0,1);	//make it into a row array
			minMaxLoc(opmvector, &minVal, &maxVal);
			
			data_y.at<double>( 0, graphindex ) = (maxVal-minVal)/(maxVal+minVal);
			
			graphindex = graphindex + 1;
			if (graphindex > (w/2) )
			{
				graphindex=0;
				data_y=Mat::zeros(cv::Size(1, w/2), CV_64F);
			}
            
            Ptr<plot::Plot2d> plot = plot::Plot2d::create( data_x, data_y );
            plot->setInvertOrientation(1);				// to make it increase y axis upwards
			plot->setShowText(0);
			plot->setPlotLineColor(Scalar(0, 255, 255));
			 
			plot->setMinY(0);
			plot->setMaxY(1);
			 
			plot->render(plot_result1);
			
			meanStdDev(opmvector, meansc, stddevsc);
			data_y2.at<double>( 0, graphindex ) = stddevsc[0]/255.0;
			
			Ptr<plot::Plot2d> plot2 = plot::Plot2d::create( data_x, data_y2 );
            plot2->setInvertOrientation(1);				// to make it increase y axis upwards
			plot2->setShowText(0);
			plot2->setPlotLineColor(Scalar(0, 0, 255));
			plot2->setMinY(0);
			plot2->setMaxY(1);
			plot2->render(plot_result2);
			
			plot_result = plot_result1 + plot_result2;
			
			imshow( "Max - Min in yellow, Std Dev in red", plot_result );
                
            fps++;
            t_end = time(NULL);
                if(t_end - t_start >= 5)
                {
                    printf("fps = %d\n",fps / 5); 
                    
                    printf("Max intensity = %d\n", int(floor(maxVal)));
                    printf("Min intensity = %d\n", int(floor(minVal)));
                    printf("Std Deviation = %d\n", int(floor(stddevsc[0])));
                    printf("Norm. max-min = %d%\n", int(floor(100*(maxVal-minVal)/(maxVal+minVal))) );
                    fps = 0;
                    t_start = time(NULL);
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
				 
				default:
					break;
                
				} 
            
				if(doneflag==1)
				{
					
					
					break;
				}   

			 }  // if ret success end
        } // inner while loop end
        
	} // end of if found 
        
 
    
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
