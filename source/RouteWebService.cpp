#include <fastcgi2/component.h>
#include <fastcgi2/component_factory.h>
#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/data_buffer.h>

#include "rapidjson/document.h"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

const std::string POST_METHOD = "POST";
const std::string PUT_METHOD = "PUT";
const std::string GET_METHOD = "GET";
const std::string DELETE_METHOD = "DELETE";

const std::string NOT_FOUND = "NF";

class RouteHandler {
public:
	RouteHandler() {}
	~RouteHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		std::string result = "Hello from route\n";
		return result;
	}
};

class PointHandler {
private:

public:
	PointHandler() {}
	~PointHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		std::string result = "Hello from point\n";
		return result;
	}
};

class RelationHandler {
public:
	RelationHandler() {}
	~RelationHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		std::string result = "Hello from relation\n";
		return result;
	}
};

class PointsHandler {
private:
	std::string handlePost(const fastcgi::DataBuffer& body) {
		std::string result = "";
		if (body.empty()) {
			result = "Empty\n";
			return result;
		}
		body.toString(result);
		
		rapidjson::Document doc_body;
		doc_body.Parse(result.c_str());
		
		mongocxx::client conn{mongocxx::uri{}};
    		bsoncxx::builder::stream::document point{};
    		auto collection = conn["route_service"]["points"];
    		point << "name" << doc_body["name"].GetString();
    		collection.insert_one(point.view());

		//result = doc_body["name"].GetString();
		return result;
	}

	std::string handleGet() {
		std::string result = "Hello from points get\n";
		return result;
	}

	std::string handleMethod(const std::string& method, const fastcgi::DataBuffer& body) {
		if (method.compare(POST_METHOD) == 0) {
			return handlePost(body);
		}
		if (method.compare(GET_METHOD) == 0) {
			return handleGet();
		}
		return NOT_FOUND;
	}

public:
	PointsHandler() {}
	~PointsHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		if (path.size() == 1) {
			return handleMethod(method, body);
		}
		PointHandler h;
		return h.handle(method, path, body);		
	}
};

class RelationsHandler {
private:
	std::string handlePost(const fastcgi::DataBuffer& body) {
		std::string result = "Hello from relations\n";
		return result;
	}


	std::string handleMethod(const std::string& method, const fastcgi::DataBuffer& body) {
		if (method.compare(POST_METHOD) == 0) {
			return handlePost(body);
		}
		return NOT_FOUND;
	}

public:
	RelationsHandler() {}
	~RelationsHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		if (path.size() == 1) {
			return handleMethod(method, body);
		}
		RelationHandler h;
		return h.handle(method, path, body);	
	}
};

class RouteWebServiceClass : virtual public fastcgi::Component, virtual public fastcgi::Handler {

public:
        RouteWebServiceClass(fastcgi::ComponentContext *context) :
                fastcgi::Component(context) {
        }
        virtual ~RouteWebServiceClass() {
        }
	std::vector<std::string> split(const std::string &path) {
    		std::vector<std::string> elems;
    		std::stringstream ss;
    		ss.str(path);
    		std::string item;
   		while (std::getline(ss, item, '/')) {
        		elems.push_back(item);
    		}
    		return elems;
	}
	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		if (path.size() == 0) {
			std::string res = "Hello from root\n";
			return res;
		}
		std::string points = "points", relations = "relations", route = "route";
		if (path[0].compare(points) == 0) {
			PointsHandler h;
			return h.handle(method, path, body);
		}
		if (path[0].compare(relations) == 0) {
			RelationsHandler h;
			return h.handle(method, path, body);
		}
		if (path[0].compare(route) == 0) {
			RouteHandler h;
			return h.handle(method, path, body);
		}
		return NOT_FOUND;
	}

public:
        virtual void onLoad() {
        }
        virtual void onUnload() {
        }
        virtual void handleRequest(fastcgi::Request *request, fastcgi::HandlerContext *context) {
		
		std::string meth = request->getRequestMethod();
		std::string uri = request->getURI();
		fastcgi::DataBuffer body = request->requestBody();
		std::vector<std::string> parts = split(uri.substr(1));
		std::string result = handle(meth, parts, body);
		
		if (result.compare(NOT_FOUND) == 0) {
			request->setStatus(404);
		}
		else {
			//request->setContentType("application/json");
			request->write(result.c_str(), result.length());
		}		
                //request->setHeader("Route-Web-Service-Header", "Reply from HelloWorld");
        }

};

FCGIDAEMON_REGISTER_FACTORIES_BEGIN()
FCGIDAEMON_ADD_DEFAULT_FACTORY("simple_factory", RouteWebServiceClass)
FCGIDAEMON_REGISTER_FACTORIES_END()