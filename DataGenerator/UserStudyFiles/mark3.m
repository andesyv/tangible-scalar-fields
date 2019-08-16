function mark3(imageNumber, coordinates)

% coordinates = [col row; col row; col row];

radius = 20;
lineWidth = 1;
letters = char('A','B','C');

% Control Group - Heatmap
heatMap = imread(strcat(strcat('heatMap',int2str(imageNumber)),'.png'));
[X,cmap] = imread(strcat(strcat('discreteHeatMap',int2str(imageNumber)),'.png'));       % necessary because of Matlab :(
discreteHeatMap = im2uint8(ind2rgb(X,cmap));

% Control Group - Scatter Plots
scatterPlot = imread(strcat(strcat('scatterPlot',int2str(imageNumber)),'.png'));
scatterPlotColored = imread(strcat(strcat('scatterPlotColored',int2str(imageNumber)),'.png'));

% Experimental Group
ippContinuous = imread(strcat(strcat('IPPcontinuous',int2str(imageNumber)),'.png'));
ippDiscrete = imread(strcat(strcat('IPPdiscrete',int2str(imageNumber)),'.png'));

% -----------------------------------------------------------------------------------------------------

heatMapMarked = heatMap;
discreteHeatMapMarked = discreteHeatMap;

scatterPlotMarked = scatterPlot;
scatterPlotColoredMarked = scatterPlotColored;

ippContinuousMarked = ippContinuous;
ippDiscreteMarked = ippDiscrete;


for i = 1:3
    
    % image
    heatMapMarked = insertShape(heatMapMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
    heatMapMarked = insertShape(heatMapMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');
    
    % letter
    heatMapMarked = insertText(heatMapMarked, [coordinates(i,1) coordinates(i,2)-43], letters(i,1), 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
    
    if i == 1  % A
        heatMapMarked = insertText(heatMapMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 2  % B
        heatMapMarked = insertText(heatMapMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 3  % C
        heatMapMarked = insertText(heatMapMarked, [coordinates(i,1)+0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');   
    end
    
    % write image
    imwrite(heatMapMarked,strcat(strcat('heatMap',int2str(imageNumber)),'_marked3.png'))
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    % image
    discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
    discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');
    
    % letter
    discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(i,1) coordinates(i,2)-43], letters(i,1), 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
    
    if i == 1  % A
        discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 2  % B
        discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 3  % C
        discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(i,1)+0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');   
    end
    
    % write image
    imwrite(discreteHeatMapMarked,strcat(strcat('discreteHeatMap',int2str(imageNumber)),'_marked3.png'))
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    % image
    scatterPlotMarked = insertShape(scatterPlotMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
    scatterPlotMarked = insertShape(scatterPlotMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');
    
    % letter
    scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(i,1) coordinates(i,2)-43], letters(i,1), 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
    
    if i == 1  % A
        scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 2  % B
        scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 3  % C
        scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(i,1)+0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');   
    end
    
    % write image
    imwrite(scatterPlotMarked,strcat(strcat('scatterPlot',int2str(imageNumber)),'_marked3.png'))
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    % image
    scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
    scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');
    
    % letter
    scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(i,1) coordinates(i,2)-43], letters(i,1), 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
    
    if i == 1  % A
        scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 2  % B
        scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 3  % C
        scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(i,1)+0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');   
    end
    
    % write image
    imwrite(scatterPlotColoredMarked,strcat(strcat('scatterPlotColored',int2str(imageNumber)),'_marked3.png'))
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    % image
    ippContinuousMarked = insertShape(ippContinuousMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
    ippContinuousMarked = insertShape(ippContinuousMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');
    
    % letter
    ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(i,1) coordinates(i,2)-43], letters(i,1), 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
    
    if i == 1  % A
        ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 2  % B
        ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 3  % C
        ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(i,1)+0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');   
    end
    
    % write image
    imwrite(ippContinuousMarked,strcat(strcat('IPPcontinuous',int2str(imageNumber)),'_marked3.png'))
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    
    % image
    ippDiscreteMarked = insertShape(ippDiscreteMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
    ippDiscreteMarked = insertShape(ippDiscreteMarked, 'circle', [coordinates(i,1) coordinates(i,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');
    
    % letter
    ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(i,1) coordinates(i,2)-43], letters(i,1), 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
    
    if i == 1  % A
        ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 2  % B
        ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(i,1)-0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');
    end
    
    if i == 3  % C
        ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(i,1)+0.5 coordinates(i,2)-43], letters(i,1), 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');   
    end
    
    % write image
    imwrite(ippDiscreteMarked,strcat(strcat('IPPdiscrete',int2str(imageNumber)),'_marked3.png'))
    
    %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
end

end

