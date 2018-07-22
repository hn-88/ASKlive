pkg load signal;

for k = 1:19
	
	dataFileName = strcat('altframeres_', num2str(k,'%03d'),'-',num2str(0,'%03d'), '.png');
	if exist(dataFileName, 'file')
		a = double(imread(dataFileName));
		else
		fprintf('File %s does not exist.\n', dataFileName);
	end
	
	for p = 1:9	
	 
		dataFileName = strcat('altframeres_', num2str(k,'%03d'),'-',num2str(p,'%03d'), '.png');
	
		if exist(dataFileName, 'file')
			a = a + double(imread(dataFileName));
			 
			outFileName = strcat('altframeres_', num2str(k,'%03d'), '.png');
			imwrite(uint8(a/3),outFileName,'png'); % normfactor of 5, and divided by 10. But saturating, so /3
		else
			fprintf('File %s does not exist.\n', dataFileName);
		end
	
	end
		
end
