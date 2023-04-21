
// for Json::value
#include <json/json.h>
#include <json/reader.h>
#include <json/writer.h>
#include <json/value.h>
#include <string>

// for JsonRPCCPP
#include <iostream>
#include "hw6server.h"
#include <jsonrpccpp/server/connectors/httpserver.h>
#include "hw6client.h"
#include <jsonrpccpp/client/connectors/httpclient.h>

// ecs36b
#include "ecs36b_Common.h"
#include "JvTime.h"

using namespace jsonrpc;
using namespace std;

class Myhw6Server : public hw6Server
{
public:
  Myhw6Server(AbstractServerConnector &connector, serverVersion_t type);
  virtual Json::Value set_name(const std::string& action, const Json::Value& student_list, const std::string& team_name);
};

Myhw6Server::Myhw6Server(AbstractServerConnector &connector, serverVersion_t type)
  : hw6Server(connector, type)
{
  std::cout << "Myhw6Server Object created" << std::endl;
}

// member functions

Json::Value
Myhw6Server::set_name
(const std::string& action, const Json::Value& student_list,
 const std::string& team_name)
{
  Json::Value lv_log;
  lv_log["arg action"] = action;
  lv_log["arg student_list"] = student_list;
  lv_log["arg team_name"] = team_name;
  myPrintLog(lv_log.toStyledString(), "./config/hw6server.log");
  
  int i;
  int error_code = 0;
  int rc = 0;
  Json::Value result;
  result["status"] = "successful";

  char buf_fname1[256];
  bzero(buf_fname1, 256);
  snprintf(buf_fname1, 255, "./config/ecs36b_teams.json");

  char buf_fname2[256];
  bzero(buf_fname2, 256);
  snprintf(buf_fname2, 255, "./config/ecs36b_students.json");

  Json::Value lv_teams;
  Json::Value lv_students;

  try {
    if (action != "set_name")
      {
	error_code = -9;
	throw ecs36b_Exception { ("action " + action + " mismatched") };
      }

    rc = myFile2JSON(buf_fname1, &lv_teams);
    if (rc != 0)
      {
	error_code = rc;
	throw ecs36b_Exception
	  { ("myFile2JSON error " + std::string { buf_fname1 }) };
      }

    if((lv_teams.isNull() == true) ||
       (lv_teams.isObject() == false))
      {
	error_code = -1;
	throw ecs36b_Exception
	  { ("teams JSON format incorrect " + std::string { buf_fname1 }) };
      }

    rc = myFile2JSON(buf_fname2, &lv_students);
    if (rc != 0)
      {
	error_code = rc;
	throw ecs36b_Exception
	  { ("myFile2JSON error " + std::string { buf_fname2 }) };
      }

    if ((lv_students.isNull() == true) ||
	(lv_students.isObject() == false))
      {
	error_code = -2;
	throw ecs36b_Exception
	  { ("students JSON format incorrect " + std::string { buf_fname2 }) };
      }

    if (team_name.empty())
      {
	error_code = -3;
	throw ecs36b_Exception { ("team name empty") };
      }

    if ((student_list.isNull() == true) ||
	(student_list.isArray() == false) ||
	(student_list.size() == 0))
      {
	error_code = -4;
	throw ecs36b_Exception {("student list JSON array format incorrect")};
      }

    if ((lv_teams[team_name]).isNull() == false)
      {
	error_code = -5;
	throw ecs36b_Exception
	  { ("team name already taken: " + team_name) };
      }

    // set up a new team
    // check if the team members have been registered with other team
    Json::Value lv_jv;
    for (i = 0; i < student_list.size(); i++)
      {
	if ((student_list[i]).isString() == false)
	  {
	    error_code = -6;
	    throw ecs36b_Exception
	      { ("student name is not a string. index = " + std::to_string(i)) };
	  }
	else
	  {
	    lv_jv = lv_students[(student_list[i]).asString()];
	    if ((lv_jv.isNull() == true) ||
		(lv_jv.isObject() == false))
	      {
		error_code = -7;
		throw ecs36b_Exception
		  { ("student format incorrect. index = " + std::to_string(i)) };
	      }

	    if ((lv_jv["team name"].isNull() == true) ||
		(lv_jv["team name"].isString() == false) ||
		(lv_jv["team name"]).asString() != "unassigned")
	      {
		error_code = -8;
		throw ecs36b_Exception
		  { ("team_name not unassigned. index = " + std::to_string(i)) };
	      }
	  }
      }

    for (i = 0; i < student_list.size(); i++)
      {
	lv_students[(student_list[i]).asString()]["team name"]
	  = team_name; 
	lv_students[(student_list[i]).asString()]["score"]
	  = 0; 
      }
    lv_teams[team_name] = student_list;

    // save to both files
    rc = myJSON2File(buf_fname1, &lv_teams);
    if (rc != 0)
      {
	error_code = rc;
	throw ecs36b_Exception
	  { ("myJSON2File error " + std::string { buf_fname1 }) };
      }

    rc = myJSON2File(buf_fname2, &lv_students);
    if (rc != 0)
      {
	error_code = rc;
	throw ecs36b_Exception
	  { ("myJSON2File error " + std::string { buf_fname2 }) };
      }
  } catch (ecs36b_Exception& e) {
    std::cerr << e.what() << std::endl;
    result["reason"] = e.what();
    result["error_code"] = error_code;
    result["status"] = "failed";
  }

  lv_log["result"] = result;
  myPrintLog(lv_log.toStyledString(), "./config/hw6server.log");
  
  return result;
}

int
main(void)
{
  JvTime * my_ptr = getNowJvTime();

  srand((my_ptr->second) * (my_ptr->minute) * (my_ptr->year));
  HttpServer httpserver(8384);
  Myhw6Server s(httpserver,
		JSONRPC_SERVER_V1V2); // hybrid server (json-rpc 1.0 & 2.0)
  s.StartListening();
  std::cout << "Hit enter to stop the server" << endl;
  getchar();

  s.StopListening();
  return 0;
}
