#include <iostream>
#include <fstream>

int main(){
    int width=600, height=400;
    std::ofstream out("post_1_gradient.ppm");
    out << "P3\n" << width << " " << height << "\n255\n";

    for (int j = height-1; j >= 0; j--) {
        for (int i = 0; i < width; i++) {
            float r = float(i)/(width-1);    
            float g = float(j)/(height-1);   
            float b = 0.25f;                 
            out << int(255.99f*r) << " " << int(255.99f*g) << " " << int(255.99f*b) << "\n";
        }
    }
    std::cout << "Wrote post_1_gradient.ppm\n";
    return 0;
}
