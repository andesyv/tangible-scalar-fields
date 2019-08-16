clear all
close all
numpoints = 10000;
x=linspace(-10,10,numpoints);
pos = 0;
radius = 2;
y = zeros(1,numpoints);
for k=1:numpoints
    y(k) = ((radius - (min(radius,(x(k)-pos)^2)))/radius)^2;


end

plot(x,y)