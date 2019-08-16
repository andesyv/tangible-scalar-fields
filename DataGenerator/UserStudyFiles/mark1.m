function mark1(imageNumber, coordinates)

% coordinates = [col row];

radius = 20;
lineWidth = 1;

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

% image
heatMapMarked = insertShape(heatMapMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');

% letter
heatMapMarked = insertText(heatMapMarked, [coordinates(1,1) coordinates(1,2)-43], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
heatMapMarked = insertText(heatMapMarked, [coordinates(1,1)-0.5 coordinates(1,2)-43], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(heatMapMarked,strcat(strcat('heatMap',int2str(imageNumber)),'_marked1.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% image
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');

% letter
discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(1,1) coordinates(1,2)-43], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(1,1)-0.5 coordinates(1,2)-43], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(discreteHeatMapMarked,strcat(strcat('discreteHeatMap',int2str(imageNumber)),'_marked1.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


% image
scatterPlotMarked = insertShape(scatterPlotMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');

% letter
scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(1,1) coordinates(1,2)-43], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(1,1)-0.5 coordinates(1,2)-43], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(scatterPlotMarked,strcat(strcat('scatterPlot',int2str(imageNumber)),'_marked1.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');

% letter
scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(1,1) coordinates(1,2)-43], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(1,1)-0.5 coordinates(1,2)-43], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(scatterPlotColoredMarked,strcat(strcat('scatterPlotColored',int2str(imageNumber)),'_marked1.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
ippContinuousMarked = insertShape(ippContinuousMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');

% letter
ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(1,1) coordinates(1,2)-43], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(1,1)-0.5 coordinates(1,2)-43], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(ippContinuousMarked,strcat(strcat('IPPcontinuous',int2str(imageNumber)),'_marked1.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*10, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'circle', [coordinates(1,1) coordinates(1,2) radius], 'LineWidth', lineWidth*4, 'Color', 'white');

% letter
ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(1,1) coordinates(1,2)-43], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(1,1)-0.5 coordinates(1,2)-43], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(ippDiscreteMarked,strcat(strcat('IPPdiscrete',int2str(imageNumber)),'_marked1.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

end

