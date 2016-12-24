#include <fastcgi2/component.h>
#include <fastcgi2/component_factory.h>
#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/data_buffer.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <bsoncxx/array/view.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <bsoncxx/types/value.hpp>
#include <bsoncxx/stdx/string_view.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <iterator>
#include <chrono>

const std::string POST_METHOD = "POST";
const std::string PUT_METHOD = "PUT";
const std::string GET_METHOD = "GET";
const std::string DELETE_METHOD = "DELETE";

const std::string NOT_FOUND = "NF";
const std::string NO_CONTENT = "NC";
const std::string NOT_MODIFIED = "NM";
const std::string ALREADY_EXISTS = "AE";

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::array;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

struct Relation {
	int cost;
	int time;
	std::string destination;
	std::string source;
};

struct Point {
	std::string name;
	std::vector<Relation> relations;
};

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

	static std::vector<std::string> split(const std::string &str, char delim) {
    		std::vector<std::string> elems;
    		std::stringstream ss;
    		ss.str(str);
    		std::string item;
   		while (std::getline(ss, item, delim)) {
        		elems.push_back(item);
    		}
    		return elems;
	}

	static long long currentMillis() {
		long long millis = std::chrono::duration_cast<std::chrono::milliseconds>
			(std::chrono::system_clock::now().time_since_epoch()).count();
		return millis;
	}

	static std::string convertToValidName(std::string name) {
		size_t start_pos = 0;
		const std::string space = " ", under = "_";
   		while((start_pos = name.find(space, start_pos)) != std::string::npos) {
        		name.replace(start_pos, space.length(), under);
        		start_pos += under.length();
    		}
		return name;
	}

	static void convertRelationNameToPointNames(const std::string& name, std::string& source, std::string& destination) {
		std::vector<std::string> names = split(name, '-');
		source = names[0];
		destination = names[1];
	}

	static std::string convertPointNamesToRelationName(const std::string& source, const std::string& dest) {
		return source + "-" + dest;
	}

	static document buildSearchPointDocumentByRelation(const Relation& rel) {
		document doc{};
		doc << "name" << rel.source << "relations" << open_document
			<< "$elemMatch" << open_document
			<< "destination" << rel.destination
			<< close_document << close_document;
		return doc;
	}

	static document buildUpdatePointDocumentByName(const std::string& name) {
		document doc{};
		doc << "$set" << open_document <<
                        "name" << convertToValidName(name) << close_document;
		return doc;
	}

	static document buildPointDocumentByName(const std::string& name) {
		document doc{};
		doc << "name" << convertToValidName(name);
		return doc;
	}


	static document buildDeleteRelationPointDocument(const Relation& rel) {
		document doc{};
		doc << "$pull" << open_document << "relations" << open_document
			<< "destination" << rel.destination << close_document << close_document;
		return doc;
	}

	static document buildInsertRelationPointDocument(const Relation& rel) {
		document doc{};
		doc << "$push" << open_document << "relations" << open_document
			<< "destination" << convertToValidName(rel.destination) 
			<< "cost" << rel.cost 
			<< "time" << rel.time
			<< close_document << close_document;
		return doc;
	}

	static document buildInsertLM(const std::string& name) {
		document doc{};
		doc << "$push" << open_document << "lm" << open_document
			<< "name" << convertToValidName(name)
			<< "val" << currentMillis()
			<< close_document << close_document;
		return doc;
	}

	static document buildDeleteLM(const std::string& name) {
		document doc{};
		doc << "$pull" << open_document << "lm" << open_document
			<< "name" << convertToValidName(name) << close_document << close_document;
		return doc;
	}

	static document buildUpdateSetLM(const std::string& name) {
		document doc{};
		doc << "$set" << open_document << "lm.$" << open_document
			<< "name" << convertToValidName(name)
			<< "val" << currentMillis() << close_document << close_document;
		return doc;
	}

	static document buildUpdateSearchLM(const std::string& name) {
		document doc{};
		doc << "lm.name" << convertToValidName(name);
		return doc;
	}

	static document buildUpdateSearchRelationsAfterPointUpdate(const std::string& name) {
		document doc{};
		doc << "relations.destination" << convertToValidName(name);
		return doc;
	}

	static document buildUpdateSetRelationsAfterPointUpdate(const std::string& name) {
		document doc{};
		doc << "$set" << open_document 
			<< "relations.$.destination" << convertToValidName(name) << close_document;
		return doc;
	}

	static document buildPointDocumentByBody(const fastcgi::DataBuffer& body) {
		std::string request_body = "";
		body.toString(request_body);
		rapidjson::Document doc_body;
		doc_body.Parse(request_body.c_str());
		std::string name = doc_body["name"].GetString();
		return buildPointDocumentByName(name);		
	}

	static document buildUpdateDocumentByBody(const fastcgi::DataBuffer& body) {
		std::string request_body = "";
		body.toString(request_body);
		rapidjson::Document doc_body;
		doc_body.Parse(request_body.c_str());
		std::string name = doc_body["name"].GetString();
		return buildUpdatePointDocumentByName(name);		
	}

	static document buildUpdateSetRelationsAfterPointUpdateByBody(const fastcgi::DataBuffer& body) {
		std::string request_body = "";
		body.toString(request_body);
		rapidjson::Document doc_body;
		doc_body.Parse(request_body.c_str());
		std::string name = doc_body["name"].GetString();
		return buildUpdateSetRelationsAfterPointUpdate(name);		
	}

	static Relation getRelationFromRelationBody(const fastcgi::DataBuffer& body) {
		std::string request_body = "";
		body.toString(request_body);
		rapidjson::Document doc_body;
		doc_body.Parse(request_body.c_str());
		Relation rel;
		rel.source = doc_body["source"].GetString();
		rel.destination = doc_body["destination"].GetString();
		rel.cost = doc_body["cost"].GetInt();
		rel.time = doc_body["time"].GetInt();
		return rel;		
	}	

	static document buildSearchPointDocumentByRelationName(const std::string& name) {
		Relation rel;
		convertRelationNameToPointNames(name, rel.source, rel.destination);		
		return buildSearchPointDocumentByRelation(rel);
	}

	static document buildUpdatePointDocumentByRelationName(const std::string& name) {
		Relation rel;
		convertRelationNameToPointNames(name, rel.source, rel.destination);		
		return buildDeleteRelationPointDocument(rel);
	}

	static document buildDeleteRelationPointDocumentByPointName(const std::string& name) {
		Relation rel;
		rel.destination = name;
		return buildDeleteRelationPointDocument(rel);
	}



	static Relation extractRelationFromPointDocument(const bsoncxx::document::value& point, const std::string& relation_id) {
		Relation rel;
		convertRelationNameToPointNames(relation_id, rel.source, rel.destination);
		rapidjson::Document doc_point;
		doc_point.Parse(bsoncxx::to_json(point).c_str());
		for (auto& r : doc_point["relations"].GetArray()) {
    			std::string d = r["destination"].GetString();
			if (rel.destination.compare(d) == 0) {
				rel.cost = r["cost"].GetInt();
				rel.time = r["time"].GetInt();
				break;
			}
		}
		return rel;
	}

	static std::string buildJsonRelation(const Relation& rel) {
		rapidjson::StringBuffer s;
    		rapidjson::Writer<rapidjson::StringBuffer> writer(s);
    
    		writer.StartObject();            
    		writer.Key("source");              
    		writer.String(rel.source.c_str());            
    		writer.Key("destination");
    		writer.String(rel.destination.c_str());
    		writer.Key("cost");
   		writer.Int(rel.cost);
    		writer.Key("time");
    		writer.Int(rel.time);
		writer.EndObject();
		
		return s.GetString();
	}

};

class RouteHandler {
public:
	RouteHandler() {}
	~RouteHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		std::string result = "{\"cost\": \"For free!\", \"time\":\"In one moment!\"}";
		return result;
	}

};

class PointHandler {
private:

	std::string handlePut(const std::string& point_id, const fastcgi::DataBuffer& body) {
		if (body.empty()) {
			return NO_CONTENT;
		}

		const auto& searchOldPoint = UtilHelper::buildPointDocumentByName(point_id) << finalize;
		const auto& searchNewPoint = UtilHelper::buildPointDocumentByBody(body) << finalize;
		const auto& updatePoint = UtilHelper::buildUpdateDocumentByBody(body) << finalize;

		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];

		const auto& check_exist = collection.find_one(searchNewPoint.view());
		if (check_exist) {
			return ALREADY_EXISTS;
		}

		const auto& result = collection.update_one(searchOldPoint.view(), updatePoint.view());

		if (result && (*result).matched_count() > 0) {
			const auto& result_check = collection.find_one(searchNewPoint.view());
			if (result_check) {
				const auto& searchUpdRels = UtilHelper::buildUpdateSearchRelationsAfterPointUpdate(point_id) << finalize;
				const auto& setUpdRels = UtilHelper::buildUpdateSetRelationsAfterPointUpdateByBody(body) << finalize;
				collection.update_many(searchUpdRels.view(), setUpdRels.view());
				return bsoncxx::to_json(*result_check);
			}
			return NOT_MODIFIED;
		}

		return NOT_FOUND;
	}

	std::string handleGet(const std::string& point_id) {
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		const auto& searchPoint = UtilHelper::buildPointDocumentByName(point_id) << finalize;
		const auto& result = collection.find_one(searchPoint.view());
    		if (result) {
			return bsoncxx::to_json(*result);
		}
		return NOT_FOUND;
	}

	std::string handleDelete(const std::string& point_id) {
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		const auto& searchPoint = UtilHelper::buildPointDocumentByName(point_id) << finalize;
		const auto& result = collection.delete_one(searchPoint.view());
    		if (result && (*result).deleted_count() > 0) {
			const auto& deleteRels = UtilHelper::buildDeleteRelationPointDocumentByPointName(point_id) << finalize;
			collection.update_many(document{} << finalize, deleteRels.view());
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
private:

	std::string handlePut(const std::string& relation_id, const fastcgi::DataBuffer& body) {
		return NOT_FOUND;
	}

	std::string handleDelete(const std::string& relation_id) {
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		const auto& searchPoint = UtilHelper::buildSearchPointDocumentByRelationName(relation_id) << finalize;
		const auto& updatePoint = UtilHelper::buildUpdatePointDocumentByRelationName(relation_id) << finalize;

		const auto& result = collection.update_one(searchPoint.view(), updatePoint.view());
    		if (result && (*result).matched_count() > 0) {
			return "";
		}
		return NOT_FOUND;

	}

	std::string handleGet(const std::string& relation_id) {
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];
		const auto& searchPoint = UtilHelper::buildSearchPointDocumentByRelationName(relation_id) << finalize;
		const auto& result = collection.find_one(searchPoint.view());
    		if (result) {
			Relation rel = UtilHelper::extractRelationFromPointDocument((*result), relation_id);
			return UtilHelper::buildJsonRelation(rel);
		}
		return NOT_FOUND;

	}

	std::string handleMethod(const std::string& method, const std::string& relation_id, const fastcgi::DataBuffer& body) {
		if (method.compare(GET_METHOD) == 0) {
			return handleGet(relation_id);
		}
		if (method.compare(PUT_METHOD) == 0) {
			return handlePut(relation_id, body);
		}
		if (method.compare(DELETE_METHOD) == 0) {
			return handleDelete(relation_id);
		}
		return NOT_FOUND;
	}

public:
	RelationHandler() {}
	~RelationHandler() {}

	std::string handle(const std::string& method, const std::vector<std::string> &path, const fastcgi::DataBuffer& body) {
		if (path.size() != 2) {
			return NOT_FOUND;
		}
		return handleMethod(method, path[1], body);
	}
};

class PointsHandler {
private:
	std::string handlePost(const fastcgi::DataBuffer& body) {
		if (body.empty()) {
			return NO_CONTENT;
		}
		const auto& point = UtilHelper::buildPointDocumentByBody(body) << finalize;
		
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];

		const auto& check_exist = collection.find_one(point.view());
    		if (check_exist) {
			return bsoncxx::to_json(*check_exist);
		}

    		collection.insert_one(point.view());
		const auto& check_result = collection.find_one(point.view());
		if (check_result) {
			return bsoncxx::to_json(*check_result);
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
		if (body.empty()) {
			return NO_CONTENT;
		}
		const auto& rel = UtilHelper::getRelationFromRelationBody(body);
		
		
		mongocxx::client conn{mongocxx::uri{}};
    		auto collection = conn["route_service"]["points"];

		const auto& searchPoint = UtilHelper::buildPointDocumentByName(rel.source) << finalize;
		const auto& check_source_exist = collection.find_one(searchPoint.view());
    		if (!check_source_exist) {
			return NOT_FOUND;
		}

		const auto& searchDestPoint = UtilHelper::buildPointDocumentByName(rel.destination) << finalize;
		const auto& check_dest_exist = collection.find_one(searchDestPoint.view());
    		if (!check_dest_exist) {
			return NOT_FOUND;
		}


		const auto& searchExistingPoint = UtilHelper::buildSearchPointDocumentByRelation(rel) << finalize;
		const auto& check_exist = collection.find_one(searchExistingPoint.view());
    		if (check_exist) {
			return ALREADY_EXISTS;
		}
		
		const auto& updatePoint = UtilHelper::buildInsertRelationPointDocument(rel) << finalize;

    		const auto& upd_result = collection.update_one(searchPoint.view(), updatePoint.view());
		if (upd_result && (*upd_result).matched_count() > 0) {
			const auto& check_result = collection.find_one(searchExistingPoint.view());
			if (check_result) {
				Relation res = UtilHelper::extractRelationFromPointDocument((*check_result), UtilHelper::convertPointNamesToRelationName(rel.source, rel.destination));
				return UtilHelper::buildJsonRelation(rel);
			}
		}

		return NOT_MODIFIED;
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
		std::vector<std::string> parts = UtilHelper::split(uri.substr(1), '/');
		std::string result = handle(meth, parts, body);
		
		if (result.compare(NOT_FOUND) == 0) {
			request->setStatus(404);
		} else if (result.compare(NO_CONTENT) == 0) {
			request->setStatus(204);
		} else if (result.compare(NOT_MODIFIED) == 0) {
			request->setStatus(304);
		} else if (result.compare(ALREADY_EXISTS) == 0) {
			request->setStatus(409);
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