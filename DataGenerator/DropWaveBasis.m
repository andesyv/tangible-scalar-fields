clear all
close all
sample_size = 20000;

bounds = [-1,1];
noise_freq = 12;
k = 1;
i = 1;
while k<=sample_size
%for k = 1:sample_size
    
x = rand*(bounds(2)-bounds(1)) + bounds(1);
y = rand*(bounds(2)-bounds(1)) + bounds(1);


frac1 = 1 + cos(noise_freq * sqrt(x^2+y^2));
frac2 = 0.5*(x^2+y^2) + 2;
frac3 = sin(x*10);

z = (frac1/frac2)/2 + cos(2*sqrt(x^2+y^2))/2 + (frac3+1)/15;

if(z > rand)
X(k) = x;
Y(k) = y;
Z(k) = z;
r(k) = 0.01;
c(k) = 1;
k = k + 1;
end
i = i + 1;
end
figure(1)
%scatter(X,Y,'.')
%scatter.MarkerFaceAlpha = .2;

hold on
scatter(X,Y,'filled','SizeData',10)
%scatter(10*rand(10,1),10*rand(10,1),'filled','SizeData',200)
hold off
alpha(.1)

figure(2)
pcshow([X(:),Y(:),Z(:)]);


%Saving to excel format
A = {'X','Y', 'Z', 'radius', 'colour'};
AllData = cell(sample_size,5);
for j = 1:sample_size
AllData(j + 1,1) = num2cell(X(j));
AllData(j + 1,2) = num2cell(Y(j));
AllData(j + 1,3) = num2cell(Z(j));
AllData(j + 1,4) = num2cell(r(j));
AllData(j + 1,5) = num2cell(c(j));
end


[pdfx xi]= ksdensity(X);
[pdfy yi]= ksdensity(Y);
% Create 2-d grid of coordinates and function values, suitable for 3-d plotting
[xxi,yyi]     = meshgrid(xi,yi);
[pdfxx,pdfyy] = meshgrid(pdfx,pdfy);
% Calculate combined pdf, under assumption of independence
pdfxy = pdfxx.*pdfyy; 
% Plot the results
figure(3)
mesh(xxi,yyi,pdfxy)
set(gca,'XLim',[min(xi) max(xi)])
set(gca,'YLim',[min(yi) max(yi)])





xlswrite('testdata.xlsx', AllData)

