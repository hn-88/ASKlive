pkg load image;

avgpixvalue=[1:60];
exptime=[10:10:600];
snrvalues=[1:5];
snrvalues4=snrvalues;
snrxaxis=[100:100:500];

for k=1:60

        %disp(k);
        fflush(stdout);
		
	      dataFileName = strcat('DR', num2str(k,'%03d'), '.png');
     
	      if exist(dataFileName, 'file')
		      a = double(imread(dataFileName));
          avgpixvalue(k)=mean(mean(a));
          %disp(avgpixvalue(k));
          %disp(exptime(k));
          fflush(stdout);
        else
          disp(dataFileName)
          disp('Not Found')
        end

end

figure;
plot(exptime, avgpixvalue);

for k=1:5
     dataFileName = strcat('DR', num2str(k*10,'%03d'), '.png');
     a = double(imread(dataFileName));
     a4=imresize(a,0.25,'bilinear');
     dataFileName = strcat('DR', num2str(k*10 + 1,'%03d'), '.png');
     b = double(imread(dataFileName));
     b4=imresize(b,0.25,'bilinear'); % this is not enough
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
     b4=b4./16;
          
     
     subtractedframe = b-a;
     subtractedframe = reshape(subtractedframe,1,[]);
     stddev = std(subtractedframe);
     meanv = mean(mean(a));
     snrvalues(k)= 2.*meanv./stddev;
     snrxaxis(k)=meanv;
     
     subtractedframe = b4-a4;
     subtractedframe = reshape(subtractedframe,1,[]);
     stddev = std(subtractedframe);
     %meanv = mean(mean(a));
     snrvalues4(k)= 2.*meanv./stddev;
end

figure;
plot(snrxaxis,sqrt(snrxaxis),'k');
hold on;
plot(snrxaxis,snrvalues,'.');
plot(snrxaxis,snrvalues4,'o');
title('SNR');
xlabel('Average Pixel value');
ylabel('SNR');
legend('Shot noise limit', 'Without binning', '4x4 binned', "location", 'northwest');    