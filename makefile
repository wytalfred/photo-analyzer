Photo_analyzer:main.cpp
	g++ -std=c++11 main.cpp -o Photo_analyzer -I./stb -DLINUX
clean:
	rm -rf Photo_analyzer
