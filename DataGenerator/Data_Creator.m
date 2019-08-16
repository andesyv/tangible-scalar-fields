clear all
close all

num_smaples = 2000000;
area_size = 3;

X = zeros(num_smaples,1);
Y = zeros(num_smaples,1);
Z = zeros(num_smaples,1);


 num_random_points = 8;
 rand_points = zeros(num_random_points,2);
for k= 1:num_random_points           
	rand_x = ((rand()-0.5)*2)*area_size;
    rand_y = ((rand()-0.5)*2)*area_size;
    
    rand_points(k,1) = rand_x;
    rand_points(k,2) = rand_y;
    
    
end

num_samples = 1;
while num_samples < (num_smaples + 1)
   
    x = ((rand()-0.5)*2)*area_size;
    y = ((rand()-0.5)*2)*area_size;
    
    z = waves(x,y);
    %z = slopes(x,y);
    %outliers(x, y, rand_points);
    if z >= rand()
        X(num_samples) = x;
        Y(num_samples) = y;
        Z(num_samples) = z;
        num_samples = num_samples + 1;
    end
end

figure(1)
scatter(X,Y,'filled');
xlim([-area_size area_size])
ylim([-area_size area_size])


figure(2)
%[Xs,Ys] = meshgrid(-area_size:0.1:area_size,-area_size:0.1:area_size);
%Zs = prob(Xs,Ys)
%surf(Xs,Ys,Zs)
scatter3(X,Y,Z);%,'filled');
R = ones(num_smaples,1);

filename = 'testdata1.xlsx';
xlswrite(filename,[X*1000,Y*1000,Z,R])











