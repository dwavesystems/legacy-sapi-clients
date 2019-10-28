//Copyright © 2019 D-Wave Systems Inc.
//The software is licensed to authorized users only under the applicable license agreement.  See License.txt.

#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>
#include <types.hpp>
#include <sapi-service.hpp>
#include <wctype.h>
#include <boost/foreach.hpp>

using std::cerr;
using std::cout;
using std::string;
using std::vector;
using std::cout;
using std::make_shared;
using std::mutex;
using std::condition_variable;
using std::lock_guard;
using std::unique_lock;

using sapiremote::makeSapiService;
using sapiremote::RemoteProblemInfo;
using sapiremote::http::HttpService;

//connects remotely and retrieves status messages for input job ID
void PrintProblemStatus(const string uRL, string APIToken, const vector<string>& ids);

class Callback : public sapiremote::StatusSapiCallback
{
  bool done_;
  bool success_;
  mutable mutex mutex_;
  mutable std::condition_variable cv_;
  vector<RemoteProblemInfo> result_;
  std::exception_ptr exe_;

  virtual void completeImpl(std::vector<RemoteProblemInfo>& problemInfo) 
  {
    lock_guard<mutex> l(mutex_);
    result_=std::move(problemInfo);
    done_ = true;
    success_=true;
    cv_.notify_all();
  }

  virtual void errorImpl(std::exception_ptr e) 
  {
    lock_guard<mutex> l(mutex_);
    exe_=e;
    done_ = true;
    cv_.notify_all();
  }

public:
  Callback() : done_(false),success_(false)
  {}

  vector<RemoteProblemInfo> result() const
  {
    unique_lock<mutex> lock(mutex_);
    while (!done_) cv_.wait(lock);
    if(!success_)
    {
    	//handle throwing exception again
      std::cout<<"Job Status return failed. Rethrowing exception"<<std::endl;
      std::rethrow_exception(exe_); 
    }
    //else return the vector
    return std::move(result_);
  }
};//end of Callback

void PrintProblemStatus(const string url, string apitoken, const vector<string>& ids)
{
 
  sapiremote::http::Proxy p; 
  //create a httpservice obj and assign ptr to it
  auto httpService=sapiremote::http::makeHttpService(1);
  //make a CallBack obj here. 
  auto callback = make_shared<Callback>();
  auto sapiservice = makeSapiService(httpService, url,apitoken,p);
  sapiservice->multiProblemStatus(ids,callback);

  BOOST_FOREACH( auto& i, callback->result() )
  {
    //using Mapping function to print job status as a string to i/o stream
	//format
	//ID - status
	//submittedOn;
	//solvedOn;
	//type;
	//errorMessage;
	auto statusMessage = sapiremote::toString<sapiremote::remotestatuses::Type>(i.status);
	cout<<"******Start of Status Info********"<<std::endl;
	cout << i.id <<" - " <<statusMessage << '\n';
	cout << "Submitted Dated: " <<i.submittedOn<< '\n';
	//check if solved from status
	if( (statusMessage.compare("FAILED")!=0) || !(statusMessage.compare("CANCELED")!=0))
	{
		cout << "Solved Date: " <<i.solvedOn<< '\n';
	}
	cout << "Problem type: " <<i.type<< '\n';
	//check if returns some error msg from status
	if((statusMessage.compare("FAILED")==0) || (statusMessage.compare("CANCELED")==0))
	{
		cout << "ErrorMessage if any: " <<i.errorMessage<< '\n';
	}
	cout<<"******End of Info********"<<std::endl;
  }
  cout<<"\n\n";
}

int main(int argc, char* argv[]) 
{

  string url;
  string token;
  //check for 3 arguments from input
  if (argc < 3) 
  {
    cerr << "Missing URL and token arguments\n";
    return 0;
  }

  url=(string)argv[1];
  token=(string)argv[2];
  
  //E.g. JOB ID - "056e99dd-1d31-4edf-a4c4-f6b6202a7d4a","cb0e5f86-6949-4188-930b-dcbea8ffa397"
  //skip blank inputs
  vector<string> ID;  
  std::string line;
  while (std::getline(std::cin, line))
  {
	if(line=="")
		break;

    ID.push_back(line);
  }

  //call function ith following inputs
  //input 1 - url
  //input 2 - APIToken
  //input 3- array of job ids
  PrintProblemStatus(url,token,ID);

  return 0;
}
