function position = colorMapPosition(pixelColor, colorMap, backgroundColor)

% convert to range [0,1]:
% - https://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio
oldMin = 1;
oldMax = size(colorMap,2);
newMin = 0;
newMax = 100;

for i = 1:size(colorMap,2)
    
    % check it color is background color
    if pixelColor(1,1,:) == backgroundColor
        position = NaN;
        break
    end
    
    % otherwise find position in color map
    if(pixelColor(1,1,:) == colorMap(1,i,:))
        position = (((i - oldMin) * (newMax - newMin)) / (oldMax - oldMin)) + newMin;
        break
    end 
end

