clc;
clear all;

numberOfImages = 85;

for i = 1:numberOfImages
    
    currentNumber = int2str(i);

    scatterPlot = imread(strcat(strcat('clustMe',currentNumber),'.png'));
    [centers, radii, metric] = imfindcircles(scatterPlot, [5 8],'ObjectPolarity','dark','Sensitivity',1.0,'Method','twostage');

    imshow(scatterPlot);
    viscircles(centers, radii,'Color','b');

    % save image of detected elements
    img = getframe(gcf);
    imwrite(img.cdata, [strcat(strcat('clustMe',currentNumber),'_detected'), '.png']);

    outPutMatrix = zeros(size(centers,1), 4);
    centers(:,1) = centers(:,1)*(-1) + abs(max(centers(:,1)));
    centers(:,2) = centers(:,2)*(-1) + abs(max(centers(:,2)));

    % write x-, y-coordinates, Radius, and Color
    outPutMatrix(:,1:2) = centers;
    outPutMatrix(:,3:4) = ones(size(centers,1),2);
    csvwrite(strcat(strcat('clustMe',currentNumber),'.csv'),outPutMatrix);

    % create a header
    cHeader = {'X' 'Y' 'Radius' 'Color'};
    textHeader = strjoin(cHeader, ',');

    %write header to file
    fid = fopen(strcat(strcat('clustMe',currentNumber),'.csv'),'w'); 
    fprintf(fid,'%s\n',textHeader);
    fclose(fid);

    %write data to end of file
    dlmwrite(strcat(strcat('clustMe',currentNumber),'.csv'),outPutMatrix,'-append');
    
end