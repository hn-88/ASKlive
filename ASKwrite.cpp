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
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libqhy/qhyccd.h>
#include <sys/time.h>
#include <sys/stat.h>
// this is for mkdir

 

#include <opencv2/opencv.hpp>
// used the above include when imshow was being shown as not declared
// removing
// #include <opencv/cv.h>
// #include <opencv/highgui.h>
 

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
    Scalar meansubtracted,  pixv1,  pixv2 ;
    double stdsubtracted, stdpixv1, stdpixv2;
    Scalar meanavgsubtracted,  meanavgframe1,  meanavgframe2;
    double  stdavgsubtracted, stdavgframe1, stdavgframe2; 
    std::ofstream outfile("ASKoutput.m");
    bool doneflag=0, skeypressed=0, bkeypressed=0;
    
    w=640;
    h=480;
    
    int  fps, key;
    struct timeval tval_before, tval_after, tval_result;
    std::ifstream infile("ASKwrite.ini");
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
			infile >> numofm2slices;
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
        
        ret = SetQHYCCDBinMode(camhandle,cambinx, cambiny); 
        if(ret == QHYCCD_SUCCESS)
        {
            //printf("SetQHYCCDBinMode success - width = %d !\n", w); 
        }
        else
        {
            printf("SetQHYCCDBinMode fail\n");
            goto failure;
        }
        
        
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
        double mean1, mean2, std1, std2;
        Scalar scMEAN, scSTD;
        Mat m, m1, m2, subtractedpixv, m1avg, m2avg, subtractedavgframe;
        Mat slice[numofm1slices];      // array of n images
        Mat bk[numofm2slices];      // array of n images
        for (indexi=0; indexi<numofm1slices; indexi++)
        {
			slice[indexi] = Mat::zeros(cv::Size(w, h), CV_32F); 
		}
		for (indexbk=0; indexbk<numofm2slices; indexbk++)
        {
			bk[indexbk] = Mat::zeros(cv::Size(w, h), CV_32F); 
		}
        
        m1 = Mat::zeros(cv::Size(w, h), CV_32F);  
		m2 = Mat::zeros(cv::Size(w, h), CV_32F);
        
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
        gettimeofday(&tval_before, NULL);
        fps = 0;
        
        indexi = 0;
        indexbk = 0;
	     
        while(1)
        { 
            ret = GetQHYCCDLiveFrame(camhandle,&w,&h,&bpp,&channels,m.data);
            
            if (ret == QHYCCD_SUCCESS)  
            {
            
            imshow("show",m);
            
			fps++;
            gettimeofday(&tval_after, NULL);
            timersub(&tval_after, &tval_before, &tval_result);
            
                if((long int)tval_result.tv_sec >= 5)
                {
                    printf("fps = %d\n",fps / 5); 
                    fps = 0;
                    gettimeofday(&tval_before, NULL);
                }
            
            
                
                if (skeypressed==1)	
                if (firstaccum>0)
					{ 
						accumulate(m, slice[indexi]);
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
							
							//outfile<<"%Data cube in MATLAB compatible format - m(w,h,slice)"<<std::endl;
							// imagesc(slice(:,:,1)) shows the same image as is seen onscreen
				
							outfile<<"slice(:,:,";
							outfile<<indexi+1;		// making the output starting index 1 instead of 0		
							outfile<<")=";
							outfile<<slice[indexi];
							outfile<<";"<<std::endl;
							
							char filename[80];
							sprintf(filename, "slice%03d.png",indexi+1);
							imwrite(filename, slice[indexi]);
							
							indexi++;
							if (indexi < numofm1slices)
							firstaccum=numofframes;							// ready to acquire next set
							
							printf("Frame acquisition %d done.\n", indexi);	// indexi+1 so that this starts from 1
							
									
						}
							
				if (bkeypressed==1)
				if (secondaccum>0)
							{
								accumulate(m, bk[indexbk]);
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
									
									outfile<<"bk(:,:,";
									outfile<<indexbk+1;		// making the output starting index 1 instead of 0		
									outfile<<")=";
									outfile<<bk[indexbk];
									outfile<<";"<<std::endl;
									
									char filename[80];
									sprintf(filename, "bk%03d.png",indexbk+1);
									imwrite(filename, bk[indexbk]);
									
									indexbk++;
									printf("BK acquisition %d done.\n", indexbk);
									
									
									if (indexbk < numofm2slices)
									secondaccum=numofframes;				// ready to acquire next set
									else
									doneflag=1;
								}

					 
					 
                key=waitKey(3); // wait 3 milliseconds for keypress
                
                if (key == 27) // ESC key
                {
					doneflag=1;
				}
                
                if (key == 's')
                if (indexi < numofm1slices)	// don't allow captures after the end of assigned number of slices
                skeypressed=1;
                
                if (key == 'b')
                if (indexbk < numofm2slices)	// don't allow captures after the end of assigned number of slices
                bkeypressed=1;
                
            } 
            
            if(doneflag==1)
			{
				
				numofm1slices=indexi;		// in case acquisition was aborted, no need to export a lot of zeros
				numofm2slices=indexbk;
				
				
				//pixv1=mean(slice[0]);
				//pixv2=mean(bk[0]);
			
				//std::cout<<"mean1="<< pixv1(0) <<"mean2="<< pixv2(0)<<std::endl; //only the first element is of interest for us, rest zeros
				
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
