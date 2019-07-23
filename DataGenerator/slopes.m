function z = slopes(x, y)
    z = 0;
    
    %% Upper Right Point Linear Slope
    cent_x = 1.5;
    cent_y = 1.5;
    dist = sqrt(abs((x-cent_x)*(x-cent_x) + (y-cent_y)*(y-cent_y)));
    
    if dist < 1
        z = 0.5 + (1-dist)/2;
    end
    
    %% Upper Left Point Quadratic Slope
    cent_x = -1.5;
    cent_y = 1.5;
    dist = sqrt(abs((x-cent_x)*(x-cent_x) + (y-cent_y)*(y-cent_y)));
    
    if dist < 1
        z = 0.5 + ((1-dist)^2)/2;
    end
    %% Lower Right Point Qubic Slope
    cent_x = 1.5;
    cent_y = -1.5;
    dist = sqrt(abs((x-cent_x)*(x-cent_x) + (y-cent_y)*(y-cent_y)));
    
    if dist < 1
        z = 0.5 + ((1-dist)^3)/2;
    end

    %% Lower Left Point Qubic Slope
    cent_x = -1.5;
    cent_y = -1.5;
    dist = sqrt(abs((x-cent_x)*(x-cent_x) + (y-cent_y)*(y-cent_y)));
    
    if dist < 1
        z = 0.5 + ((1-dist)^0.5)/2;
    end
end
