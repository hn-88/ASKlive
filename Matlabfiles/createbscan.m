pkg load signal;
bscanat=15;
bscan = zeros(99,128);
for k = 1:99
	%for p = 0:9	
	 
	dataFileName = strcat('altframeres_', num2str(k,'%03d'), '.png');
         
	if exist(dataFileName, 'file')
		a = imread(dataFileName);
        bscan(k,:)=a(15,:);
										  
      
        %outFileName = strcat('altframeres_', num2str(k,'%03d'), '.png');
        %imwrite(uint8(res*5),outFileName,'png');
	else
		fprintf('File %s does not exist.\n', dataFileName);
	end
	
	%end
	imwrite(uint8(bscan),'altframeBscan.png','png')	
end

imagesc(bscan)

%imagesc(imread('altframeBscan.png'))
% used as alt frame with layers missing