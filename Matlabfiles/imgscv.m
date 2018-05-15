figure;
indexi=1;
vari = input('Enter variable name');
maxx = input('Enter max number of images');
V=vari;
while(1)
    imagesc(V(:,:,indexi));colorbar;
    title(sprintf('Image %d', indexi));
    
    x=kbhit();
    if (x=='q')
      break
    end
    
    if (x=='n')
     indexi++;
     if indexi>maxx
      indexi=1;
     end
    end
    
    if (x=='p')
     indexi--;
     if indexi<1
      indexi=maxx;
     end
    end
end
