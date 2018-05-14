figure;
indexi=1;
while(1)
    imagesc(V(:,:,indexi));colorbar;
    title(sprintf('slice %d', indexi));
    
    x=kbhit();
    if (x=='q')
      break
    end
    
    if (x=='n')
     indexi++;
     if indexi>148
      indexi=1;
     end
    end
    
    if (x=='p')
     indexi--;
     if indexi<1
      indexi=148;
     end
    end
end
