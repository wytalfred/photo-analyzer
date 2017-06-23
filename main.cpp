#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/time.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_STATIC
#include "stb/stb_image_resize.h"

using namespace std;
typedef unsigned char uchar;



// if you are using Windows or linux, make sure to commit the next line!
//#define MACOS_X

// #define DEBUG




struct timeval time_val;
struct timezone tz;
inline long getTime()
{
    gettimeofday(&time_val, &tz);
    return (time_val.tv_sec%1000)*1000000 + time_val.tv_usec;
}

inline void RGB2HSL(uchar R, uchar G, uchar B, float& H, float& S, float& L)
{
    float r, g, b, cmax, cmin, delta;
    r = R / 255.0;
    g = G / 255.0;
    b = B / 255.0;
    cmax = fmax(fmax(r,g),b);
    cmin = fmin(fmin(r,g),b);
    delta = cmax - cmin;
    L = (cmax + cmin) / 2;
    if (delta == 0)
    {
        H = 0;
        S = 0;
    }
    else
    {
        if(L <= 0.5)
            S = delta / (cmax + cmin);
        else
            S = delta / (2 - (cmax + cmin));

        if(cmax == r)
        {
            if (g >= b)
                H = int( 60 * (g - b) / delta + 0.5);
            else
                H = int( 60 * (g - b) / delta + 360 + 0.5);
        }
        else if(cmax == g)
            H = int( 60 * (b - r) / delta + 120 + 0.5);
        else
            H = int( 60 * (r - g) / delta + 240 + 0.5);
    }
}

void integral(unsigned char* in, unsigned long* out, int w, int h)
{
    unsigned long *colsum = new unsigned long[w]; // sum of each column
    // calculate integral of the first line
    for(int i=0;i<w;i++)
    {
        colsum[i]=in[i];
        out[i] = in[i];
        if(i>0) out[i] += out[i-1];
    }
    for (int i=1;i<h;i++)
    {
        int offset = i*w;
        // first column of each line
        colsum[0] +=in[offset];
        out[offset] = colsum[0];
         // other columns
        for(int j=1;j<w;j++)
        {
            colsum[j] += in[offset+j];
            out[offset+j] = out[offset+j-1] + colsum[j];
        }
    }
}

// A pixel's local contrast is measured by the fabs() of the pixel's value and the average of pixel values around it.
void getLocalContrast(unsigned char* img, unsigned long* sum, unsigned char* out, int w, int h, int s)
{
    long ss = s*s;
    for(int i=0;i<h;i++)
    {
        for(int j=0;j<w;j++)
        {
            int x = min(max(j-s/2-1,-1),w-s-1);
            int y = min(max(i-s/2-1,-1),h-s-1);
            long avg;
            if(y == -1 && x != -1)
                avg = (sum[(y+s)*w+(x+s)] - sum[(y+s)*w+x]) / ss;
            else if(x == -1 && y != -1)
                avg = (sum[(y+s)*w+(x+s)] - sum[y*w+(x+s)]) / ss;
            else if(x == -1 && y == -1)
                avg = sum[(y+s)*w+(x+s)] / ss;
            else
                avg = (sum[(y+s)*w+(x+s)] - sum[y*w+(x+s)] - sum[(y+s)*w+x] + sum[y*w+x]) / ss;
            out[i*w+j] = abs(img[i*w+j]-avg)*255/(avg>128?avg:(255-avg));
        }
    }
}


// Don't know what for, but must have it.
class stbir_context {
public:
    stbir_context()
    {
        size = 1000000;
        memory = malloc(size);
    }

    ~stbir_context()
    {
        free(memory);
    }

    size_t size;
    void* memory;
} g_context;


int main(int argc, char const *argv[])
{
#ifdef DEBUG
long us = getTime();
#endif
#if defined ( MACOS_X)||(LINUX)
    string apppath = argv[0];
    while(apppath[apppath.size()-1] != '/')
        apppath.pop_back();
    apppath.pop_back();
#else
    string apppath = ".";
#endif

    string location;
    if(argc == 1)
    {
        cout << "Drag the image here." << endl;
        getline(cin, location);
#if defined ( MACOS_X)||(LINUX)
        location[location.length()-1] = 0;
        while(1) // delete '\'
        {
            int pos = location.find("\\");
            if(pos == -1) break;
            location.erase(pos, 1);
        }
#endif
    }
    else if(argc == 2)
        location = argv[1];

#ifdef DEBUG
cout << getTime() - us << endl;
us = getTime();
#endif
cout << "Loading image..." << endl;

    // read in and resize image
    int width, height, n;
    uchar* inputImage = stbi_load(location.c_str(), &width, &height, &n, 3);

#ifdef DEBUG
cout << getTime() - us << endl;
us = getTime();
#endif
cout << "resizing..." << endl;

    float ratio = max(width, height)/800.0;
    int w = (int)(width/ratio);
    int h = (int)(height/ratio);
    uchar* img = (uchar*)malloc(w * h * n);
    stbir_resize(inputImage, width, height, 0, img, w, h, 0, STBIR_TYPE_UINT8, n, STBIR_ALPHA_CHANNEL_NONE, 0, STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_FILTER_BOX, STBIR_COLORSPACE_SRGB, &g_context);

#ifdef DEBUG
cout << getTime() - us << endl;
us = getTime();
#endif
cout << "Calculating channels..." << endl;

    // get R, G, B, H, S, L channels
    uchar* Hq = new uchar[w*h*1];
    uchar* H = new uchar[w*h*1];
    uchar* S = new uchar[w*h*1];
    uchar* L = new uchar[w*h*1];
    uchar* R = new uchar[w*h*1];
    uchar* G = new uchar[w*h*1];
    uchar* B = new uchar[w*h*1];
    for(int i=0;i<h;i++)
    {
        for(int j=0;j<w;j++)
        {
            uchar r, g, b;
            r = img[(i*w+j)*n];
            g = img[(i*w+j)*n+1];
            b = img[(i*w+j)*n+2];
            float h, s, l;
            RGB2HSL(r, g, b, h, s, l);
            Hq[i*w+j] = (uchar)((30-abs((int)h%60-30))*8+15);
            H[i*w+j] = (uchar)(h*255/360);
            S[i*w+j] = (uchar)(s*255);
            L[i*w+j] = (uchar)(l*255);
            R[i*w+j] = r;
            G[i*w+j] = g;
            B[i*w+j] = b;
        }
    }

#ifdef DEBUG
cout << getTime() - us << endl;
us = getTime();
#endif
cout << "Integral..." << endl;

    unsigned long* Lsum = new unsigned long[w*h*1];
    unsigned long* Rsum = new unsigned long[w*h*1];
    unsigned long* Gsum = new unsigned long[w*h*1];
    unsigned long* Bsum = new unsigned long[w*h*1];
    uchar* Lcontrast1 = new uchar[w*h*1];
    uchar* Lcontrast2 = new uchar[w*h*1];
    uchar* Lcontrast3 = new uchar[w*h*1];
    uchar* Lcontrast4 = new uchar[w*h*1];
    uchar* Lcontrast5 = new uchar[w*h*1];
    uchar* Lcontrast = new uchar[w*h*1];
    uchar* Rcontrast = new uchar[w*h*1];
    uchar* Gcontrast = new uchar[w*h*1];
    uchar* Bcontrast = new uchar[w*h*1];
    uchar* ColorContrast = new uchar[w*h*1];

    integral(L, Lsum, w, h);
    integral(R, Rsum, w, h);
    integral(G, Gsum, w, h);
    integral(B, Bsum, w, h);

#ifdef DEBUG
cout << getTime() - us << endl;
us = getTime();
#endif
cout << "Calculating contrast..." << endl;
    // getLocalContrast(L, Lsum, Lcontrast1, w, h, min(w,h)/32);
    // getLocalContrast(L, Lsum, Lcontrast2, w, h, min(w,h)/16);
    // getLocalContrast(L, Lsum, Lcontrast3, w, h, min(w,h)/8);
    // getLocalContrast(L, Lsum, Lcontrast4, w, h, min(w,h)/4);
    // getLocalContrast(L, Lsum, Lcontrast5, w, h, min(w,h)/2);
    // for(int i=0;i<h;i++)
    //     for(int j=0;j<w;j++)
    //         Lcontrast[i*w+j] = (Lcontrast1[i*w+j] + Lcontrast2[i*w+j] + Lcontrast3[i*w+j] + Lcontrast4[i*w+j] + Lcontrast5[i*w+j]) / 5;

    getLocalContrast(L, Lsum, Lcontrast, w, h, min(w,h)/4);
    getLocalContrast(R, Rsum, Rcontrast, w, h, min(w,h)/4);
    getLocalContrast(G, Gsum, Gcontrast, w, h, min(w,h)/4);
    getLocalContrast(B, Bsum, Bcontrast, w, h, min(w,h)/4);
    for(int i=0;i<h;i++)
        for(int j=0;j<w;j++)
            ColorContrast[i*w+j] = max(max(Rcontrast[i*w+j], Gcontrast[i*w+j]), Bcontrast[i*w+j]);

#ifdef DEBUG
cout << getTime() - us << endl;
us = getTime();
#endif
cout << "Output..." << endl;

    stbi_write_bmp((apppath+"/01-Lightness.png").c_str(), w, h, 1, L);
    stbi_write_bmp((apppath+"/02-Saturation.png").c_str(), w, h, 1, S);
    stbi_write_bmp((apppath+"/03-Hue.png").c_str(), w, h, 1, H);
    stbi_write_bmp((apppath+"/04-RContrast.png").c_str(), w, h, 1, Rcontrast);
    stbi_write_bmp((apppath+"/05-GContrast.png").c_str(), w, h, 1, Gcontrast);
    stbi_write_bmp((apppath+"/06-BContrast.png").c_str(), w, h, 1, Bcontrast);
    stbi_write_bmp((apppath+"/07-ColorContrast.png").c_str(), w, h, 1, ColorContrast);
    stbi_write_bmp((apppath+"/08-LightnessContrast.png").c_str(), w, h, 1, Lcontrast);
    stbi_write_bmp((apppath+"/09-HueQuality.png").c_str(), w, h, 1, Hq);

#ifdef DEBUG
cout << getTime() - us << endl;
#endif
cout << "Done!" << endl;

    usleep(100000);
    stbi_image_free(img);
	return 0;
}



