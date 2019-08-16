function markSlope(imageNumber, coordinates)

% coordinates = [col row];

size = 59;
halfSize = floor(size/2);

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
heatMapMarked = insertShape(heatMapMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*7, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*3, 'Color', 'white');

% line
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+3 coordinates(1,2) coordinates(1,1)+halfSize-1 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize coordinates(1,2)-1 coordinates(1,1)+halfSize coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% dotted line
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize coordinates(1,2) coordinates(1,1)-halfSize+6 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+1 coordinates(1,2)-1 coordinates(1,1)-halfSize+3 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+9 coordinates(1,2) coordinates(1,1)-halfSize+15 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+10 coordinates(1,2)-1 coordinates(1,1)-halfSize+12 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+18 coordinates(1,2) coordinates(1,1)-halfSize+24 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+19 coordinates(1,2)-1 coordinates(1,1)-halfSize+21 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+27 coordinates(1,2) coordinates(1,1)-halfSize+33 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+28 coordinates(1,2)-1 coordinates(1,1)-halfSize+30 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+36 coordinates(1,2) coordinates(1,1)-halfSize+42 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+37 coordinates(1,2)-1 coordinates(1,1)-halfSize+39 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+45 coordinates(1,2) coordinates(1,1)-halfSize+51 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+46 coordinates(1,2)-1 coordinates(1,1)-halfSize+48 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+54 coordinates(1,2) coordinates(1,1)-halfSize+60 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+55 coordinates(1,2)-1 coordinates(1,1)-halfSize+57 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% letter
heatMapMarked = insertText(heatMapMarked, [coordinates(1,1) coordinates(1,2)-57], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
heatMapMarked = insertText(heatMapMarked, [coordinates(1,1)-0.5 coordinates(1,2)-57], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(heatMapMarked,strcat(strcat('heatMap',int2str(imageNumber)),'_slope.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*7, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*3, 'Color', 'white');

% line
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+3 coordinates(1,2) coordinates(1,1)+halfSize-1 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize coordinates(1,2)-1 coordinates(1,1)+halfSize coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% dotted line
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+1 coordinates(1,2) coordinates(1,1)-halfSize+7 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+2 coordinates(1,2)-1 coordinates(1,1)-halfSize+4 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+10 coordinates(1,2) coordinates(1,1)-halfSize+16 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+11 coordinates(1,2)-1 coordinates(1,1)-halfSize+13 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+19 coordinates(1,2) coordinates(1,1)-halfSize+25 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+20 coordinates(1,2)-1 coordinates(1,1)-halfSize+22 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+28 coordinates(1,2) coordinates(1,1)-halfSize+34 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+29 coordinates(1,2)-1 coordinates(1,1)-halfSize+31 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+37 coordinates(1,2) coordinates(1,1)-halfSize+43 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+38 coordinates(1,2)-1 coordinates(1,1)-halfSize+40 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+46 coordinates(1,2) coordinates(1,1)-halfSize+52 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+47 coordinates(1,2)-1 coordinates(1,1)-halfSize+49 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+55 coordinates(1,2) coordinates(1,1)-halfSize+61 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
discreteHeatMapMarked = insertShape(discreteHeatMapMarked, 'Line', [coordinates(1,1)-halfSize+56 coordinates(1,2)-1 coordinates(1,1)-halfSize+58 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% letter
discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(1,1) coordinates(1,2)-57], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
discreteHeatMapMarked = insertText(discreteHeatMapMarked, [coordinates(1,1)-0.5 coordinates(1,2)-57], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(discreteHeatMapMarked,strcat(strcat('discreteHeatMap',int2str(imageNumber)),'_slope.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
scatterPlotMarked = insertShape(scatterPlotMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*7, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*3, 'Color', 'white');

% line
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+3 coordinates(1,2) coordinates(1,1)+halfSize-1 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize coordinates(1,2)-1 coordinates(1,1)+halfSize coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% dotted line
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+1 coordinates(1,2) coordinates(1,1)-halfSize+7 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+2 coordinates(1,2)-1 coordinates(1,1)-halfSize+4 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+10 coordinates(1,2) coordinates(1,1)-halfSize+16 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+11 coordinates(1,2)-1 coordinates(1,1)-halfSize+13 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+19 coordinates(1,2) coordinates(1,1)-halfSize+25 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+20 coordinates(1,2)-1 coordinates(1,1)-halfSize+22 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+28 coordinates(1,2) coordinates(1,1)-halfSize+34 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+29 coordinates(1,2)-1 coordinates(1,1)-halfSize+31 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+37 coordinates(1,2) coordinates(1,1)-halfSize+43 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+38 coordinates(1,2)-1 coordinates(1,1)-halfSize+40 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+46 coordinates(1,2) coordinates(1,1)-halfSize+52 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+47 coordinates(1,2)-1 coordinates(1,1)-halfSize+49 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+55 coordinates(1,2) coordinates(1,1)-halfSize+61 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotMarked = insertShape(scatterPlotMarked, 'Line', [coordinates(1,1)-halfSize+56 coordinates(1,2)-1 coordinates(1,1)-halfSize+58 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% letter
scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(1,1) coordinates(1,2)-57], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
scatterPlotMarked = insertText(scatterPlotMarked, [coordinates(1,1)-0.5 coordinates(1,2)-57], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(scatterPlotMarked,strcat(strcat('scatterPlot',int2str(imageNumber)),'_slope.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*7, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*3, 'Color', 'white');

% line
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+3 coordinates(1,2) coordinates(1,1)+halfSize-1 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize coordinates(1,2)-1 coordinates(1,1)+halfSize coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% dotted line
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+1 coordinates(1,2) coordinates(1,1)-halfSize+7 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+2 coordinates(1,2)-1 coordinates(1,1)-halfSize+4 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+10 coordinates(1,2) coordinates(1,1)-halfSize+16 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+11 coordinates(1,2)-1 coordinates(1,1)-halfSize+13 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+19 coordinates(1,2) coordinates(1,1)-halfSize+25 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+20 coordinates(1,2)-1 coordinates(1,1)-halfSize+22 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+28 coordinates(1,2) coordinates(1,1)-halfSize+34 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+29 coordinates(1,2)-1 coordinates(1,1)-halfSize+31 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+37 coordinates(1,2) coordinates(1,1)-halfSize+43 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+38 coordinates(1,2)-1 coordinates(1,1)-halfSize+40 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+46 coordinates(1,2) coordinates(1,1)-halfSize+52 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+47 coordinates(1,2)-1 coordinates(1,1)-halfSize+49 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+55 coordinates(1,2) coordinates(1,1)-halfSize+61 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
scatterPlotColoredMarked = insertShape(scatterPlotColoredMarked, 'Line', [coordinates(1,1)-halfSize+56 coordinates(1,2)-1 coordinates(1,1)-halfSize+58 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% letter
scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(1,1) coordinates(1,2)-57], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
scatterPlotColoredMarked = insertText(scatterPlotColoredMarked, [coordinates(1,1)-0.5 coordinates(1,2)-57], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(scatterPlotColoredMarked,strcat(strcat('scatterPlotColored',int2str(imageNumber)),'_slope.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
ippContinuousMarked = insertShape(ippContinuousMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*7, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*3, 'Color', 'white');

% line
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+3 coordinates(1,2) coordinates(1,1)+halfSize-1 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize coordinates(1,2)-1 coordinates(1,1)+halfSize coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% dotted line
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+1 coordinates(1,2) coordinates(1,1)-halfSize+7 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+2 coordinates(1,2)-1 coordinates(1,1)-halfSize+4 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+10 coordinates(1,2) coordinates(1,1)-halfSize+16 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+11 coordinates(1,2)-1 coordinates(1,1)-halfSize+13 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+19 coordinates(1,2) coordinates(1,1)-halfSize+25 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+20 coordinates(1,2)-1 coordinates(1,1)-halfSize+22 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+28 coordinates(1,2) coordinates(1,1)-halfSize+34 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+29 coordinates(1,2)-1 coordinates(1,1)-halfSize+31 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+37 coordinates(1,2) coordinates(1,1)-halfSize+43 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+38 coordinates(1,2)-1 coordinates(1,1)-halfSize+40 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+46 coordinates(1,2) coordinates(1,1)-halfSize+52 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+47 coordinates(1,2)-1 coordinates(1,1)-halfSize+49 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+55 coordinates(1,2) coordinates(1,1)-halfSize+61 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippContinuousMarked = insertShape(ippContinuousMarked, 'Line', [coordinates(1,1)-halfSize+56 coordinates(1,2)-1 coordinates(1,1)-halfSize+58 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% letter
ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(1,1) coordinates(1,2)-57], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
ippContinuousMarked = insertText(ippContinuousMarked, [coordinates(1,1)-0.5 coordinates(1,2)-57], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(ippContinuousMarked,strcat(strcat('IPPcontinuous',int2str(imageNumber)),'_slope.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% image
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*7, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Rectangle', [coordinates(1,1)-halfSize coordinates(1,2)-halfSize size size], 'LineWidth', lineWidth*3, 'Color', 'white');

% line
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize+3 coordinates(1,2) coordinates(1,1)+halfSize-1 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
%heatMapMarked = insertShape(heatMapMarked, 'Line', [coordinates(1,1)-halfSize coordinates(1,2)-1 coordinates(1,1)+halfSize coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% dotted line
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+1 coordinates(1,2) coordinates(1,1)-halfSize+7 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+2 coordinates(1,2)-1 coordinates(1,1)-halfSize+4 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+10 coordinates(1,2) coordinates(1,1)-halfSize+16 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+11 coordinates(1,2)-1 coordinates(1,1)-halfSize+13 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+19 coordinates(1,2) coordinates(1,1)-halfSize+25 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+20 coordinates(1,2)-1 coordinates(1,1)-halfSize+22 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+28 coordinates(1,2) coordinates(1,1)-halfSize+34 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+29 coordinates(1,2)-1 coordinates(1,1)-halfSize+31 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+37 coordinates(1,2) coordinates(1,1)-halfSize+43 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+38 coordinates(1,2)-1 coordinates(1,1)-halfSize+40 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+46 coordinates(1,2) coordinates(1,1)-halfSize+52 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+47 coordinates(1,2)-1 coordinates(1,1)-halfSize+49 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+55 coordinates(1,2) coordinates(1,1)-halfSize+61 coordinates(1,2)], 'LineWidth', lineWidth*5, 'Color', [27 74 89]);
ippDiscreteMarked = insertShape(ippDiscreteMarked, 'Line', [coordinates(1,1)-halfSize+56 coordinates(1,2)-1 coordinates(1,1)-halfSize+58 coordinates(1,2)-1], 'LineWidth', lineWidth, 'Color', 'white');

% letter
ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(1,1) coordinates(1,2)-57], 'A', 'FontSize', 50, 'TextColor', [27 74 89], 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Bold');
ippDiscreteMarked = insertText(ippDiscreteMarked, [coordinates(1,1)-0.5 coordinates(1,2)-57], 'A', 'FontSize', 46, 'TextColor', 'white', 'BoxOpacity', 0, 'AnchorPoint', 'Center', 'Font', 'Calibri Light');

% write image
imwrite(ippDiscreteMarked,strcat(strcat('IPPdiscrete',int2str(imageNumber)),'_slope.png'))

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

end

