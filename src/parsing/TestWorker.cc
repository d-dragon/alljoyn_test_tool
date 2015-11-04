/*testworker class*/
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string.h>
#include "TestWorker.h"
#include "OnboardingTest.h"
#define TIME_OUT 100

using namespace std;
int TestWorker::signalRespFlag;
string TestWorker::reportFile;

TestWorker::TestWorker(char *interface){

	serviceInterface.assign(interface);
	ajClient = NULL;
	signalRespFlag = 0;
	generateReportFileName();
}
TestWorker::~TestWorker(){

}

QStatus TestWorker::startAlljoynClient(){

	QStatus status;
	ajClient = new	AlljoynClient();

	status = ajClient->InitAlljoynClient(serviceInterface.c_str());
	if(ER_OK != status){

		printf("init alljoyn client failed\n");
		return status;
	}
	sleep(2);
	status = ajClient->ConnectServiceProvider(serviceInterface.c_str());
	if(ER_OK != status){

		printf("connect to service provider failed\n");
	}
	ajClient->RegisterCB(TIRespMonitor);
	return status;
}

QStatus TestWorker::executeTestItem(string testItem, size_t numArg, string tiArg[]){

	
	QStatus status;
	OnboardingTest *onboardingTestApp;
	int timeout = 0;

	cout << "executeTestItem: " << testItem << endl;
	for(size_t i = 0; i < numArg; i++){

		cout << "argument " << i << ": " << tiArg[i] << endl;
	}

	if(!testItem.compare("onboarding")){

		std::cout<<"procesing onboarding test\n";
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
	 
	}else{

		printf("processing signal test\n");
		ajClient->SendRequestSignal(testItem.c_str(), numArg, tiArg);
		while((signalRespFlag != 1) && (timeout < 300)){
			usleep(100000);
			timeout++;
			if(timeout == 300){
				cout << "Test item execution timeout!!!" << endl;
				//TODO - need implement action for this case
			}

		}
		signalRespFlag = 0;
	
	}
}

void TestWorker::TIRespMonitor(int respFlag, const char *respMsg, const char *srcPath, const char *member){

//	TestWorker *twInstance = static_cast<TestWorker *>(respFlag, respMsg, srcPath, member);
	cout << "Received Message: " << respMsg << endl;
	//TODO - export the result to file
	
	exportStuffToFile("<tr><th>Result</th><td colspan=\"2\">");
	exportStuffToFile(respMsg);
	exportStuffToFile("</td></tr></table>");
	signalRespFlag = respFlag;
}


int TestWorker::exportStuffToFile(const char* content){

	cout << "export content to file" << endl;
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
	sprintf(reportName, "%s%s%s%s", "output/", "report-", timeBuff, ".txt");
	reportFile.assign(reportName);

}
