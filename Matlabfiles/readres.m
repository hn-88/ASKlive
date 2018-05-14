indexi=1;
res=zeros(48,64,100);
while(indexi<101)
    res(:,:,indexi)=double(imread(sprintf('res%03d.png',indexi)));
    indexi=indexi+1;
end
    