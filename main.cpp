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

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;


void RGB2HSL(uchar R, uchar G, uchar B, float& H, float& S, float& L)
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

// Not in use. OpenCV has offered a better one.
Mat fastfloatIntegral(Mat& img)
{
    Mat out = Mat::zeros(img.size(), CV_32FC1);
    vector<float> colsum(img.cols, 0);

    out.at<float>(0,0) = img.at<float>(0,0);
    colsum[0] = img.at<float>(0,0);
    for(int j=1;j<img.cols;j++)
    {
        out.at<float>(0,j) = out.at<float>(0,j-1) + img.at<float>(0,j);
        colsum[j] = img.at<float>(0,j);
    }

    for(int i=1;i<img.rows;i++)
    {
        out.at<float>(i,0) = out.at<float>(i-1,0) + img.at<float>(i,0);
        float* data = out.ptr<float>(i);
        for(int j=1;j<img.cols;j++)
        {
            colsum[j] += img.at<float>(i,j);
            data[j] = data[j-1] + colsum[j];
        }
    }
    return out;
}

// A pixel's local contrast is measured by the fabs() of the pixel's value and the average of pixel values around it.
Mat getLocalContrast(Mat& img, Mat& sum, int w)
{
    Mat localContrast = Mat::zeros(img.size(), CV_32FC1);
    for(int i=0;i<img.rows;i++)
    {
        float* lc = localContrast.ptr<float>(i);
        float* ll = img.ptr<float>(i);
        for(int j=0;j<img.cols;j++)
        {
            int x = min(max(j-w/2,0),img.cols-w-1);
            int y = min(max(i-w/2,0),img.rows-w-1);
            Mat patch = img(Rect(x, y, w, w));
            double avg = (sum.at<float>(y+w,x+w) - sum.at<float>(y,x+w) - sum.at<float>(y+w,x) + sum.at<float>(y,x)) / (w*w);
            lc[j] = fabs(ll[j]-avg)/(avg>0.5?avg:(1-avg));
        }
    }
    return localContrast;
}

void caculate(Mat& img)
{
    Mat R = Mat::zeros(img.size(), CV_32FC1);
    Mat G = Mat::zeros(img.size(), CV_32FC1);
    Mat B = Mat::zeros(img.size(), CV_32FC1);
    Mat H = Mat::zeros(img.size(), CV_32FC1);
    Mat S = Mat::zeros(img.size(), CV_32FC1);
    Mat L = Mat::zeros(img.size(), CV_32FC1);
    Mat RContrast = Mat::zeros(img.size(), CV_32FC1);
    Mat GContrast = Mat::zeros(img.size(), CV_32FC1);
    Mat BContrast = Mat::zeros(img.size(), CV_32FC1);
    Mat lightnessContrast = Mat::zeros(img.size(), CV_32FC1);
    Mat colorContrast = Mat::zeros(img.size(), CV_32FC1);
    Mat mv[3];
    split(img, mv); // get RGB channels


    for(int i=0;i<img.cols;i++)
    {
        for(int j=0;j<img.rows;j++)
        {
            uchar r0 = mv[0].at<uchar>(j,i);
            uchar g0 = mv[1].at<uchar>(j,i);
            uchar b0 = mv[2].at<uchar>(j,i);
            float h0,s0,l0;
            RGB2HSL(r0, g0, b0, h0, s0, l0);
            H.at<float>(j,i) = h0;
            L.at<float>(j,i) = l0;
            S.at<float>(j,i) = s0;
            R.at<float>(j,i) = r0/255.0;
            G.at<float>(j,i) = g0/255.0;
            B.at<float>(j,i) = b0/255.0;
        }
    }


    Mat Lsum, Rsum, Gsum, Bsum;
    integral(L, Lsum, CV_32F);
    integral(R, Rsum, CV_32F);
    integral(G, Gsum, CV_32F);
    integral(B, Bsum, CV_32F);

    // Get local contrast in different scales(aka template size). Then add them up.
    int ratio = min(img.cols, img.rows)/10-1;
    for(int i=1;i<=5;i++)
    {
        lightnessContrast += getLocalContrast(L, Lsum, i*ratio) / 5;
        RContrast += getLocalContrast(R, Rsum, i*ratio) / 5;
        GContrast += getLocalContrast(G, Gsum, i*ratio) / 5;
        BContrast += getLocalContrast(B, Bsum, i*ratio) / 5;
    }


    for(int i=0;i<img.cols;i++)
    {
        for(int j=0;j<img.rows;j++)
        {
            colorContrast.at<float>(j,i) = fmax(fmax(RContrast.at<float>(j,i), GContrast.at<float>(j,i)), BContrast.at<float>(j,i));
        }
    }


    imshow("Original", img);
    H *= 255.0/360; H.convertTo(H, CV_8UC1); imshow("Hue",H);
    S *= 255; S.convertTo(S, CV_8UC1); imshow("Lightness",L);
    L *= 255; L.convertTo(L, CV_8UC1); imshow("Saturation",S);
    // R *= 255; R.convertTo(R, CV_8UC1); imshow("R",R);
    // G *= 255; G.convertTo(G, CV_8UC1); imshow("G",G);
    // B *= 255; B.convertTo(B, CV_8UC1); imshow("B",B);
    lightnessContrast *= 255; lightnessContrast.convertTo(lightnessContrast, CV_8UC1); imshow("lightnessContrast",lightnessContrast);
    RContrast *= 255; RContrast.convertTo(RContrast, CV_8UC1); imshow("RContrast",RContrast);
    GContrast *= 255; GContrast.convertTo(GContrast, CV_8UC1); imshow("GContrast",GContrast);
    BContrast *= 255; BContrast.convertTo(BContrast, CV_8UC1); imshow("BContrast",BContrast);
    colorContrast *= 255; colorContrast.convertTo(colorContrast, CV_8UC1); imshow("colorContrast",colorContrast);
}


int main(int argc, char const *argv[])
{
    string location;
    if(argc == 1)
    {
        cout << "Input picture location, or drag the image here." << endl;
        getline(cin, location);
        location[location.length()-1] = 0;
        while(1) // delete '\'
        {
            int pos = location.find("\\");
            if(pos == -1) break;
            location.erase(pos, 1);
        }
    }
    else if(argc == 2)
        location = argv[1];
    // cout << location << endl;


    Mat img = imread(location);
    double ratio = (double)img.rows/img.cols; // longer edge => 800px
    if(img.cols > img.rows)
        resize(img, img, Size(800,800*ratio));
    else
        resize(img, img, Size(800/ratio,800));

    caculate(img);

    waitKey(0);
	return 0;
}



