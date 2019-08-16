
imageNumber = 1;
lineLength = 51;
halflineLength = floor(lineLength/2);

imageWidth = size(heatMap,2);
imageHeight = size(heatMap,1); 

plasma1D = imread('plasma_1D.png');
backgroundColor = [255, 255, 255];

heatMap = imread(strcat(strcat('heatMap',int2str(imageNumber)),'.png'));

while 1 == 1
    
    subplot(1,2,1), imshow(heatMap);
    
    hp = impixelinfo;
    set(hp,'Position',[5 1 300 20]);
    
    childHandles = get(hp,'Children');
    
    % BREAK POINT HERE ! --------------------------------------------------
    positions = extractBetween(get(childHandles(1),'String'), '(',')');
    %----------------------------------------------------------------------
    
    coordinates = split(positions,','); 
    x = str2double(char(cell2mat(coordinates(1,1)))); x
    y = str2double(char(strrep(cell2mat(coordinates(2,1)), ' ', ''))); y
    
    subplot(1,2,1), line([x-halflineLength,x+halflineLength],[y,y]);%,'Color',[1,0,0])

    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

    slope = zeros(1, lineLength);
    counter = 0;

    for iterator = x-halflineLength:x+halflineLength

        counter = counter + 1;
        currentValue = colorMapPosition(heatMap(y,iterator,:), plasma1D, backgroundColor);

        if ~isnan(currentValue)
            slope(1, counter) = currentValue;
        else
             slope(1, counter) = 0;
        end

    end

    lineSpace = linspace(0, 100, lineLength);
    subplot(1,2,2), plot(lineSpace,slope);
end