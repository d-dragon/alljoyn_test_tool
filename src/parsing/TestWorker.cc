/*testworker class*/
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <string.h>
#include "TestWorker.h"
#include "OnboardingTest.h"
#include "JsonParser.h"
#define TIME_OUT 100

using namespace std;
int TestWorker::signalRespFlag;
string TestWorker::reportFile;
string *TestWorker::mRespMsg = NULL;
vector<struct DeviceInfo> TestWorker::mDeviceList;

string ReplaceAll(string str, const string& from, const string& to) {
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}


struct TestWorker::TestCaseInfo TestWorker::mTestCaseInfo;
struct TestWorker::ResponseMessageInfo TestWorker::respmsg;

TestWorker::TestWorker(const char *interface){

	serviceInterface.assign(interface);
	ajClient = NULL;
	signalRespFlag = 0;
	mRespMsg = NULL;
	mDeviceList.clear(); 
	generateReportFileName();
}
TestWorker::~TestWorker(){

}

int TestWorker::startAlljoynClient(const char *serviceId){

	QStatus status;
	char *targetDevId;
	ajClient = new	AlljoynClient();

	ajClient->SetRefTargetDeviceID(serviceId);
	cout << "connect to service provider: " << serviceId << endl;
	status = ajClient->InitAlljoynClient(serviceInterface.c_str());
	if(ER_OK != status){

		printf("init alljoyn client failed\n");
		return ERROR;
	}
	sleep(10);

	status = ajClient->GetTargetDeviceID(&targetDevId);
	if(ER_OK != status){

		printf("Get targetDevId failed\n");
		return ERROR;
	}
	LOGCXX("Compare DEV ID : "<< targetDevId);
	if (0 != strcmp(serviceId, targetDevId)){
		//deallocate ajClient will be handled by caller (call StopAlljoynClient) 
		return ERROR;
	}

	status = ajClient->ConnectServiceProvider(serviceInterface.c_str());
	if(ER_OK != status){

		printf("connect to service provider failed\n");
		return ERROR;
	}
	ajClient->RegisterCB(TIRespMonitor);
	LOGCXX("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
	return OK;
}

void TestWorker::StopAlljoynClient(){

	delete ajClient;
	mTestCaseInfo.Signal = "";
}

int TestWorker::executeTestItem(string testItem, size_t numArg, string tiArg[]){

	
	QStatus status;
	int timeout = 0;
	OnboardingTest *onboardingTestApp;
	
	cout << "execute test item: " << testItem << endl;
	//Save infor of Test Case
	mTestCaseInfo.Signal.assign(testItem.c_str());
	mTestCaseInfo.Type.assign(tiArg[0]);
	for(size_t i = 0; i < numArg; i++){

		cout << "argument " << i << ": " << tiArg[i] << endl;
	}

	if(!testItem.compare("onboarding")){

		onboardingTestApp = new OnboardingTest(tiArg);
		status = onboardingTestApp->CreateBusAttachment();
		if ( ER_OK != status )
		{
			onboardingTestApp->FinishBusAttachment();
			return status;
		}
		while (timeout++ < TIME_OUT) {
		   usleep(100 * 1000);
		}
		onboardingTestApp->FinishBusAttachment();
	 
	}else if(!testItem.compare("listen_notification")){
	
		if(!tiArg[0].compare("upnp")){

			int time;
			time = stoi(tiArg[1]);

			cout << "listen notification in: " <<  time << "s" << endl;
			exportStuffToFile("<tr><th>Result</th>");
			sleep(time);
			exportStuffToFile("</tr></table><br>");
		}
	}else{
		if ( numArg > 1 ){
			UpdateDevIDOfTC(tiArg);
		}

		ajClient->SendRequestSignal(testItem.c_str(), numArg, tiArg);

		cout << "call ResponseAnalyst" << endl;
		ResponseAnalyst();
		ParseRespondedMsg();

		signalRespFlag = 0;
		
	
	}

	mTestCaseInfo.Signal.clear();
	mTestCaseInfo.Type.clear();
	respmsg.message.clear();
	respmsg.srcpath.clear();
	respmsg.signalname.clear();
	return status;
}

void TestWorker::ResponseAnalyst(){

	int timeout = 0;

	while((signalRespFlag != 1) && (timeout < 300)){
		usleep(100000);
		timeout++;
		if(timeout == 300){
			cout << "Test item execution timeout!!!" << endl;
			//TODO - need implement action for this case
			exportStuffToFile("<tr><th>Result</th><td colspan=\"2\">");
			exportStuffToFile("Test item execution timeout!!!");
			exportStuffToFile("</td></tr></table><br>");

		}

	}
	if(timeout < 300){
		//received response message from callback
		//parse and analyse json message then export 
		
		if((mTestCaseInfo.Type.compare("zwave") == 0) && (respmsg.signalname.compare("notify") != 0 )){
			string status;
			string reason;
			JsonParser parser(NULL, NULL, NULL);

			cout << "Received Message: " << respmsg.message.c_str() << endl;
			parser.JSONGetObjectValue(&respmsg.message,"status", &status);
			exportStuffToFile("<tr><th>Result</th><td colspan=\"2\">");
			if(status.length() > 0){

				cout << "status " << status << endl;
				exportStuffToFile(status.c_str());
				if(status.compare("failed") == 0){
					parser.JSONGetObjectValue(&respmsg.message, "reason", &reason);
					if(reason.length() > 0){
						exportStuffToFile(" | Reason: ");
						exportStuffToFile(reason.c_str());
					}
				}
			}else{

				exportStuffToFile("Response message has no execution status");
			}

			exportStuffToFile("</td></tr>");
		}else if(mTestCaseInfo.Type.compare("upnp") == 0){

			cout << "Received Message: " << respmsg.message.c_str() << endl;

			respmsg.message = ReplaceAll(respmsg.message, "\n", "<br><br>");
			exportStuffToFile("<tr><th>Result</th><td colspan=\"2\">");

			exportStuffToFile(respmsg.message.c_str());
			exportStuffToFile("</td></tr>");
		}

		exportStuffToFile("<tr><th>Response Message</th><td colspan=\"2\">");
		exportStuffToFile(respmsg.message.c_str());
		exportStuffToFile("</td></tr></table><br>");
	}

}
void TestWorker::TIRespMonitor(int respFlag, const char *respMsg, const char *srcPath, const char *member){

	string tmpStr;
	tmpStr.assign(respMsg);
	respmsg.message.assign(respMsg);
	respmsg.srcpath.assign(srcPath);
	respmsg.signalname.assign(member);
	// TO DO
	if (0 != mTestCaseInfo.Signal.compare(member))
	{
		if((0 == mTestCaseInfo.Signal.compare("listen_notification")) && (strcmp(member, "notify") == 0)){
			//export the result to report file	
			tmpStr = ReplaceAll(tmpStr, "\n", "<br><br>");
			exportStuffToFile("<td colspan=\"2\">");
			exportStuffToFile(tmpStr.c_str());
			exportStuffToFile("</td>");
		}
		return;
	}
	//TODO - export the result to file
	/*
	tmpStr = ReplaceAll(tmpStr, "\n", "<br><br>");
	exportStuffToFile("<tr><th>Result</th><td colspan=\"2\" rowspan=\"2\">");

	exportStuffToFile(tmpStr.c_str());
	exportStuffToFile("</td></tr></table><br>");
	*/
	signalRespFlag = respFlag;
}


int TestWorker::exportStuffToFile(const char* content){

	//TODO - auto generate file name
	fstream outFile;
	outFile.open(reportFile.c_str(), fstream::out | fstream::app);
	if(!outFile.is_open()){
		cout << "can not open file" << endl;
		return -1;
	}

	outFile << content;
	outFile.close();
	return 0;

}

void TestWorker::generateReportFileName(){

	time_t rawTime;
	struct tm *timeInfo;
	char timeBuff[128];
	char reportName[256];

	time(&rawTime);
	timeInfo = localtime(&rawTime);

	strftime(timeBuff, 128, "%d-%m-%Y_%I:%M:%S", timeInfo);
	sprintf(reportName, "%s%s%s%s", "output/", "report-", timeBuff, ".html");
	reportFile.assign(reportName);

}

int TestWorker::ParseRespondedMsg(){
	// TODO: make a structure of signal name 
	int ret = 0;
	LOGCXX("TestWorker::ParseRespondedMsg");
	ret =  mTestCaseInfo.Signal.compare("list_devices");
	if ((0 == ret) && (0 == mTestCaseInfo.Type.compare("zwave")))
	{
		// Clear list dev iD before add it again
		LOGCXX("TestWorker::ParseRespondedMsg condition");
		mDeviceList.clear();
		if (respmsg.message.length() > 0){
			GetDevIDFromList();	
		}
	}


	return ret;
}
	/*
	Function: 	support user update device ID of test case, base on index of test case
	Input: 		String from test case
	Output: 	arg with ID of device will be changed corresponding with index
				if wrong index: the first device will be fill instead.
	*/
void TestWorker::UpdateDevIDOfTC(string arg[]){
	LOGCXX("TestWorker::UpdateDevIDOfTC: "<< mTestCaseInfo.Signal.c_str());
	char *cstring, *saveptr;
	int cnt = 0;
	int pos;
	string tempdevId, actions, newIDValue, stringsuffix, *saveActionInput;

	saveActionInput = NULL;

	if (mDeviceList.size() == 0){
		LOGCXX("There is no Dev ID was recorded");
		return;
	}

	/*
		Handle normal signals
	*/
	if ((mTestCaseInfo.Signal.compare("set_binary") 	== 0) ||
		(mTestCaseInfo.Signal.compare("get_binary") 	== 0) ||
		(mTestCaseInfo.Signal.compare("read_spec") 		== 0) ||
		(mTestCaseInfo.Signal.compare("write_spec") 	== 0) ||
		(mTestCaseInfo.Signal.compare("read_s_spec") 	== 0) ||
		(mTestCaseInfo.Signal.compare("remove_device") 	== 0) ||
		(mTestCaseInfo.Signal.compare("write_s_spec") 	== 0)
		)
	{
		if(arg[1].find("ID") == std::string::npos){
			// Keep the input from user
			return;
		}
		newIDValue 		= mDeviceList.at(GetIndexByCondition(arg[1])).ID;
		arg[1].assign(newIDValue);
		LOGCXX("Update new Dev ID: "<< arg[1].c_str()); 

	}

	/*
		Handle actions 
	*/
	if(mTestCaseInfo.Signal.compare("set_rule") 		== 0) {
		saveActionInput = &arg[3];
	}
	if(mTestCaseInfo.Signal.compare("rule_actions") 	== 0){
		saveActionInput = &arg[5];
	}
	if (saveActionInput != NULL){
		if(saveActionInput->find("ID") == std::string::npos){
			// Keep the input from user
			return;
		}
		LOGCXX("Action input "<< saveActionInput->c_str());
		actions.assign(*saveActionInput);
		for ( cstring 	= strtok_r((char*)actions.c_str(), ";",&saveptr); cstring; cstring = strtok_r(NULL, ";", &saveptr)) {
			if (cnt++ == 1){
				LOGCXX(cstring);
				tempdevId.assign(cstring);
				break;
			}
		}
		// Save the first occurent
		pos 			= actions.find(tempdevId);
		LOGCXX("Temporary dev id: " << tempdevId.c_str());
		//Replace 
		newIDValue 		= mDeviceList.at(GetIndexByCondition(tempdevId)).ID;
		stringsuffix	= saveActionInput->substr(pos+tempdevId.length(), saveActionInput->length()- (pos+tempdevId.length()));
		actions 		= saveActionInput->substr(0,pos) + ";" + newIDValue + ";" + stringsuffix;
		saveActionInput->assign(actions);
		LOGCXX("Update new Dev ID -> Updated actions: "<< *saveActionInput);
	}
	/*
		Handle other
	*/
}
	/*
	Function: 	Get device IDs from response of list_devices signal on Zwave
	Input: 		responded msg from list_devices
	Output: 	save all device order in this list: mTestCaseInfo.DeviceList
	*/

void TestWorker::GetDevIDFromList(){ 
#if 0
	char *devInfo, *info,*devID,  *tmp, *tmp1, *tmp2;
	int cnt = 0;
	char *listDevs =(char *) mRespMsg->c_str();
	for ( devInfo = strtok_r(listDevs, "\n", &tmp); devInfo; devInfo = strtok_r( NULL, "\n", &tmp ) ){
		cnt = 0;
		// devInfo store Responded Msg from CRESS of only 1 dev zwave
		for ( info = strtok_r(devInfo, "|", &tmp1 ); info; info = strtok_r(NULL, "|", &tmp1) ){
			if ( cnt == 3 ) // Position of Dev ID in response message
			{
				devID = strtok_r(info, " ", &tmp2);				
				mTestCaseInfo.DeviceList.push_back(std::string(devID));
				LOGCXX("Found device ID: ["<<mTestCaseInfo.DeviceList.back().c_str()<<"]");
				break;
			}
			cnt++;
		}
	}
#endif
	JsonParser::GetDevIDInJSMsg(&respmsg.message, &mDeviceList);
	LOGCXX("number of devices "<< mDeviceList.size());

}
	/*
	Function: 		Get index of Dev by parsing input
	Input format: 	"ID index"
					index is an integer value
	Return:			valid index.
	*/

int TestWorker::GetIndexByCondition(string condition){
	int ret = 0;
	int found = -1, cnt;
	char *index, *saveptr;
	/*
		Condition format: "ID index"
		ID: string
		index: integer
	*/
	LOGCXX("TestWorker::GetIndexByCondition condition: " << condition);
	found = condition.find("ID");
	if (found != std::string::npos){
		cnt = 0;
		for ( index = strtok_r((char*)condition.c_str(), " ", &saveptr); index; index = strtok_r(NULL, " ", &saveptr)){
			if (cnt++ == 1){
				ret = atoi(index);
				LOGCXX("Found index [" <<ret<<"]");
				break;
			}
		}
	}

	/*Cover out of range*/

	if ((ret < 0) || (ret >= mDeviceList.size())){
		LOGCXX("Invalid index (out of range), set it to first dev index with ID: "<< mDeviceList.front().ID);
		ret = 0;
	}
	return ret;
}
