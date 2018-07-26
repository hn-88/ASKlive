pkg load signal;

for k = 1:99
	for p = 0:9	
	 
	dataFileName = strcat('slice', num2str(k,'%03d'),'-',num2str(p,'%03d'), '.png');
    backgroundFileName = strcat('slice', num2str(k+1,'%03d'),'-',num2str(p,'%03d'), '.png');    
	if exist(dataFileName, 'file')
		a = double(imread(dataFileName));
        background = double(imread(backgroundFileName));
        b = (a - background);
        res=abs(hilbert(b));
										  
      
        outFileName = strcat('altframeres_', num2str(k,'%03d'),'-',num2str(p,'%03d'), '.png');
        imwrite(uint8(res*5),outFileName,'png');
	else
		fprintf('File %s does not exist.\n', dataFileName);
	end
	
	end
		
end
