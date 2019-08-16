%% Clean memory and close all figures
clear all
close all

%% Define the fields
%The probabiltity distribution is defined on a 0 to 1 field but is scaled
%to the bounds below
xBounds = [-1000, 10000];
yBounds = [-1000, 1000];

%Number of points that is to be generated
numPoints = 50000;

%% Generate the data
pointCounter = 0;
data = zeros(numPoints,3);
while pointCounter < numPoints
    rP = rand(3,1);
    z = evalFct(rP(1),rP(2));
    if rP(3) <= z
        pointCounter = pointCounter + 1;
        data(pointCounter,1) = (rP(1) * (xBounds(2) - xBounds(1))) + xBounds(1);
        data(pointCounter,2) = (rP(2) * (yBounds(2) - yBounds(1))) + yBounds(1);
        data(pointCounter,3) = z;
    end 
end

%% Show the scatter plot if wanted
figure(1)
scatter3(data(:,1),data(:,2),data(:,3),'filled');

xlswrite('scatterData.xlsx',data)
%% Define the evaluation function

function z = evalFct(x,y)
% Uncomment the function that is to be used above
    %Linear Slope
    %z = slope(x,y,1);
    %Convex Slope
    z = slope(x,y,0.5);
    %Concave Slope
    %z = slope(x,y,2);
    

    %Set flip to 1 if you want to flip the function
    flip = 1;
    if flip == 1 && z ~= 0
        z = 1 - z;
    end
end

function z = slope(x,y,pot)
    x = (x * 2) - 1;
    y = (y * 2) - 1;
    z = 1 - pdist([x,y; 0.0,0.0],'euclidean');
    % Consider only the points that are not further away than 1, this
    % removes the artifacts in the corners.
    if z > 0
        z = z^pot;
    else 
       z = 0; 
    end
end



