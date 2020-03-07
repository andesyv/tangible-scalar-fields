

//maps value x from [a,b] --> [0,c]
int mapInterval(float x, float a, float b, int c){
    return int((x-a)*c/(b-a));
}

//maps value x from [a,b] --> [c,d]
float mapInterval_O(float x, int a, int b, float c, float d){
    return float((x-a)*(d-c)/float(b-a) + c);
}