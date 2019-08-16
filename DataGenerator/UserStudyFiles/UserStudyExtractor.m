clear;
clc;

imageNumber = 1;
boxSize = 33;
halfBoxSize = floor(boxSize/2);

plasma1D = imread('plasma_1D.png');
backgroundColor = [255, 255, 255];

heatMap = imread(strcat(strcat('heatMap',int2str(imageNumber)),'.png'));

imageWidth = size(heatMap,2);
imageHeight = size(heatMap,1); 

if exist(strcat(strcat('transformedMap_',int2str(imageNumber),'.mat')) , 'file') == 2
    load(strcat(strcat('transformedMap_',int2str(imageNumber),'.mat')));
else
    transformedMap = zeros(imageHeight, imageWidth, 'uint8');

    for x = 1:imageWidth
       for y = 1:imageHeight
           transformedMap(y,x) = colorMapPosition(heatMap(y,x,:), plasma1D, backgroundColor);
       end
    end

    filename = strcat(strcat('transformedMap_',int2str(imageNumber)),'.mat');
    save(filename, 'transformedMap');

end

if exist(strcat(strcat(strcat(strcat('resultMatrix_',int2str(imageNumber)),'_BoxSize'),int2str(boxSize)),'.mat') , 'file') == 2
    load(strcat(strcat(strcat(strcat('resultMatrix_',int2str(imageNumber)),'_BoxSize'),int2str(boxSize)),'.mat'));
else
    resultsMap = zeros(imageHeight, imageWidth, 'double');

    for x = 1+halfBoxSize:imageWidth-halfBoxSize
        for y = 1+halfBoxSize:imageHeight-halfBoxSize

            position = double(0);
            addCount = double(0);

            if x == 11
                if y == 11
                   asd = 123; 
                end
            end
            
            for u = x-halfBoxSize:x+halfBoxSize
                for v = y-halfBoxSize:y+halfBoxSize

                   currentValue = double(transformedMap(v,u)); 

                   if currentValue ~= 0
                       position = position + currentValue;
                       addCount = addCount + double(1);
                   end

                end  
            end

            if addCount ~= 0
                resultsMap(y,x) = position/addCount;
            else
                resultsMap(y,x) = double(0);
            end
        end  
    end

    filename = strcat(strcat(strcat(strcat('resultMatrix_',int2str(imageNumber)),'_BoxSize'),int2str(boxSize)),'.mat');
    save(filename, 'resultsMap');
end

% print recommendations for possible selections

for value = 5:5:100
    occurrences = resultsMap == value;    
    disp(strcat('Value: ',int2str(value)));    
    sum(occurrences(:) == 1)
end

value = 60;
occurrences = resultsMap == value;
[row,col] = find(occurrences==1);

imshow(heatMap);
axis on
hold on;

for i = 1:size(row,1)
    plot(col(i,1), row(i,1), 'r+', 'MarkerSize', 20, 'LineWidth', 3);
end
