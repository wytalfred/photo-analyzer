# photo-analyzer
### Summary

A tool for photographers to analyze their photos. Right now it can mainly show you the  saturation and contrast of a photo. New features will be added in the future.

This program is tested on Windows and macOS. You can download the source file and compile it for your own. Or you can simply download the executable file and try it out.

### Usage

Double click on the app, it'll open a terminal window. Drag and drop your photo onto it and hit Enter on your keyboard, you'll get some output pictures in the app folder. Just like this (brighter means higher value):

![image](https://github.com/wytalfred/photo-analyzer/blob/master/sample_output.png)

On Linux type the following command
`./PhotoAnalyzer xxx.jpg`

### Build Guide

Building this program is super easy:

`g++ -std=c++11 -DMACOS_X -I./stb main.cpp -o PhotoAnalyzer`

For Linux
`g++ -std=c++11 -DLINUX -I./stb main.cpp -o PhotoAnalyzer`

Or just type 
`make`

That's it. Thanks to the open-source library stb. You can find more info of it [here](https://github.com/nothings/stb).



Good luck and enjoy!
