%Create probability functions
function z = waves(x, y)
    dist = sqrt(abs(x*x + y*y));
    
    if dist < 3
        z = (cos(dist)+1)/2;
        z = z + (cos(10*dist)+1)/6;
        
        
    else
        z = 0;
    end
    
    z = z + (y+3)/6;
    
    z = z + (cos(x*1.5)+1)/2;
    
    z = z/3.0;
    
    
   
    %z = (cos(2*x )+1)/2 + (cos(1*y)+1)/2;

    %z = (z + (cos(2*x)+1)/2 + (sin(1*y)+1)/2)/2;
    % adding additional cluster points
end