all: RouteWebService.cpp
	g++ -std=c++0x RouteWebService.cpp -fPIC -lfastcgi-daemon2 -shared -o libroutews.so

clean:
	rm -f *.so
