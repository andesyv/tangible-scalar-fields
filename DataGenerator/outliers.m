%Create probability functions
function z = outliers(x, y, rand_points)
    dist = sqrt(abs(x*x + y*y));
    
    if dist < 3
        z = (cos(dist)+1)/2;
        z = z + (cos(8*dist)+1)/10;
        
        
        
        
        % add random points

        for k= 1:size(rand_points(:,1))
            
            dist2 = sqrt(abs((x-rand_points(k,1))*(x-rand_points(k,1)) + (y-rand_points(k,2))*(y-rand_points(k,2))));
            val = 0.4 - dist2;
            if val>0
                z =  z + (val*val)*1.5;
            end
        end
        
        
    else
        z = 0;
    end
    
   
    %z = (cos(2*x )+1)/2 + (cos(1*y)+1)/2;

    %z = (z + (cos(2*x)+1)/2 + (sin(1*y)+1)/2)/2;
    % adding additional cluster points
end