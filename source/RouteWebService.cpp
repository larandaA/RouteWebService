#include <fastcgi2/component.h>
#include <fastcgi2/component_factory.h>
#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/data_buffer.h>

#include "rapidjson/document.h"

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <iterator>

const std::string POST_METHOD = "POST";
const std::string PUT_METHOD = "PUT";
const std::string GET_METHOD = "GET";
const std::string DELETE_METHOD = "DELETE";

const std::string NOT_FOUND = "NF";
const std::string NO_CONTENT = "NC";
const std::string NOT_MODIFIED = "NM";

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

class UtilHelper {
public:

	static std::string join(const std::vector<std::string>& elements, const char* const separator) {
    		switch (elements.size()) {
        		case 0:
            			return "";
        		case 1:
            			return elements[0];
        		default:
            			std::ostringstream os; 
            			std::copy(elements.begin(), elements.end()-1, std::ostream_iterator<std::string>(os, separator));
            			os << *elements.rbegin();
            			return os.str();
    		}
	}
};

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

	std::string handlePut(const std::string& point_id, const fastcgi::DataBuffer& body) {
		std::string request_body = "";
		if (body.empty()) {
			return NO_CONTENT;
		}
		
		body.toString(request_body);
		rapidjson::Document doc_body;
		doc_body.Parse(request_body.c_str());
		const std::string new_name = doc_body["name"].GetString();

		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		const auto& result = collection.update_one(document{} << "name" << point_id << finalize,
                      document{} << "$set" << open_document <<
                        "name" << new_name << close_document << finalize);

		if (result && (*result).matched_count() > 0) {
			mongocxx::stdx::optional<bsoncxx::document::value> result_check = collection.find_one(document{} << "name" << new_name << finalize);
    			if (result_check) {
				return bsoncxx::to_json(*result_check);
			}
			return NOT_MODIFIED;
		}

		return NOT_FOUND;
	}

	std::string handleGet(const std::string& point_id) {
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		mongocxx::stdx::optional<bsoncxx::document::value> result = collection.find_one(document{} << "name" << point_id << finalize);
    		if (result) {
			return bsoncxx::to_json(*result);
		}
		return NOT_FOUND;
	}

	std::string handleDelete(const std::string& point_id) {
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		const auto& result = collection.delete_one(document{} << "name" << point_id << finalize);
    		if (result && (*result).deleted_count() > 0) {
			return "";
		}
		return NOT_FOUND;
	}

	std::string handleMethod(const std::string& method, const std::string& point_id, const fastcgi::DataBuffer& body) {
		if (method.compare(GET_METHOD) == 0) {
			return handleGet(point_id);
		}
		if (method.compare(PUT_METHOD) == 0) {
			return handlePut(point_id, body);
		}
		if (method.compare(DELETE_METHOD) == 0) {
			return handleDelete(point_id);
		}
		return NOT_FOUND;
	}

public:
	PointHandler() {}
	~PointHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		if (path.size() != 2) {
			return NOT_FOUND;
		}
		return handleMethod(method, path[1], body);
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
			return NO_CONTENT;
		}
		body.toString(result);
		
		rapidjson::Document doc_body;
		doc_body.Parse(result.c_str());
		
		mongocxx::client conn{mongocxx::uri{}};
    		document point{};
    		auto collection = conn["route_service"]["points"];

		mongocxx::stdx::optional<bsoncxx::document::value> check_exist = collection.find_one(document{} << "name" << doc_body["name"].GetString() << finalize);
    		if (check_exist) {
			result = bsoncxx::to_json(*check_exist);
			return result;
		}

		point << "name" << doc_body["name"].GetString();
    		collection.insert_one(point.view());
		mongocxx::stdx::optional<bsoncxx::document::value> check_result = collection.find_one(document{} << "name" << doc_body["name"].GetString() << finalize);
		if (check_result) {
			result = bsoncxx::to_json(*check_result);
			return result;
		}

		return NOT_MODIFIED;
	}

	std::string handleGet() {
		std::string result = "{\"points\":[";

		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		mongocxx::cursor cursor = collection.find(document{} << finalize);
		std::vector<std::string> str_points;
		for(const auto& doc : cursor) {
  			str_points.push_back(bsoncxx::to_json(doc));
		}

		result.append(UtilHelper::join(str_points, ","));
		result.append("]}");
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
			return NOT_FOUND;
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
		} else if (result.compare(NO_CONTENT) == 0) {
			request->setStatus(204);
		} else if (result.compare(NOT_MODIFIED) == 0) {
			request->setStatus(304);
		} else {
			request->setContentType("application/json");
			request->write(result.c_str(), result.length());
		}		
                //request->setHeader("Route-Web-Service-Header", "Reply from HelloWorld");
        }

};

FCGIDAEMON_REGISTER_FACTORIES_BEGIN()
FCGIDAEMON_ADD_DEFAULT_FACTORY("simple_factory", RouteWebServiceClass)
FCGIDAEMON_REGISTER_FACTORIES_END()