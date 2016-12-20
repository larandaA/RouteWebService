all: RouteWebService.cpp
	g++ -std=c++11 RouteWebService.cpp -fPIC -lfastcgi-daemon2 -shared -o libroutews.so $(pkg-config --cflags --libs libmongocxx) -Xlinker -rpath /usr/local/lib

clean:
	rm -f *.so
