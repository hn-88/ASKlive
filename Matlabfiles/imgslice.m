figure;
indexi=1;
while(1)
    S=imread(sprintf('slice%03d.png', indexi));
    imagesc(S);colorbar;
    title(sprintf('slice %d', indexi));
    
    k = waitforbuttonpress;
    % 28 leftarrow
    % 29 rightarrow
    % 30 uparrow
    % 31 downarrow
    x = double(get(gcf,'CurrentCharacter'))
    if (x=='q')
      break
    end
    
    if (x=='n')
     indexi=indexi+1;
     if indexi>100
      indexi=1;
     end
    end
    
    if (x=='p')
     indexi=indexi-1;
     if indexi<1
      indexi=100;
     end
    end
end
