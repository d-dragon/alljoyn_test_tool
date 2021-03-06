/**
  @file is responsible for parsing json input file.
 */
#ifndef _JSON_PARSER_H_
#define _JSON_PARSER_H_

#include <iostream>
#include <string>
#include <jansson.h>
#include <typeinfo>
#include <vector>
#include "common_def.h"
#include "TestWorker.h"
#include "ResultVerdictHelper.h"
#include "Reporter.h"

#define CONFIG_FILE "configuration.json"
#define REFERENCE_FILE "references.json"
#define TEST_CASE_FILE "testcase.json"
using namespace std;

/**
 * %JsonParser is responsible for parsing json input file.
 */
class JsonParser {

	public:
		
		vector<string> mFileList;
		vector<string> mTestSuiteList;
		Reporter aReporter;

		JsonParser(const char *tsPath, const char *tcPath, const char *tiPath, const char *configPath);
		~JsonParser();
		int startParser(int reference_flag);

		//return string length
		
		void JSONGetObjectValue(string *inputString, string objectName, string *output);
		void JSONGetObjectValue(json_t *inputObj, string objectName, string *output);
		json_t *JSONGetObjectFromString(string *inputString, string objectName);


		template<class Type1> 
		void JSONGetStuff(Type1 *input,string objectName, int index, Type1 *output);
		static int GetDevIDInJSMsg(string *input, vector<DeviceInfo> *devList);
		int GetTestSuiteFileList(const char *dirPath);
		void PrintConfigurationInfo(const char *filePath);
		int UpdateConfiguration(const char *filePath);
		void ApplyPaths(const char *tsPath, const char *tcPath, const char *configPath, const char *ref_path);


	private:
		int mReferenceFlag;/*!< Point out the test suite run as a reference or not*/
		json_t *testSuitRoot;
		json_error_t err;
		TestWorker *worker;
		json_t *tcTemplateRoot;
		json_t *tiRoot;
		json_t *mRefJsonRoot;
		ResultVerdictHelper *mVerdictHelper;

		const char *dfTSPath;
		const char *dfTCPath;
		const char *dfTIPath;
		const char *dfConfigPath;
		const char *mReferencePath;

		int TestsuitParser(json_t *tsObj);
		int TestCaseCollector(json_t *tcRoot);
		int TestItemProcessor(json_t *inputArg, json_t *tiObj, TestItemInfo **ti_info, TestItem *t_test_item);
		json_t *getTestItemTemplateObj(const char *tiName);
		int GetWorkerConfiguration(TestWorker *worker, const char *dfConfigPath);

		/**
		 * Export test case info and result to html, cvs, test suite report.
		 * @param testCaseInfo struct store all test case info.
		 * @return void
		 */
		void ReportTestCaseInfo(TestCase testCaseInfo);
		void DeallocateTestCaseInfo(TestCase test_case_info);

		/**
		 * Get corresponding reference value from response message.
		 * Then update it into references.json 
		 * @param ti_info Test item result information.
		 * @param response_msg Matched test item response message.
		 * @return 
		 *		- #OK If update reference value successfully.
		 *		- #ERROR Update reference value failed.
		 */
		int UpdateReferenceValue(TestItemInfo *ti_info, string response_msg);

		/**
		 * This function is specific for SENSOR_MULTILEVEL type which responsible for 
		 * updating sensor value from response json object to reference json object 
		 */
		int ReplaceValueSensorMultilevel(json_t *resp_root, json_t *ref_root, const char *cmd_class, string sensor_type, const char *target_name);

};

template<class Type1>
void JsonParser::JSONGetStuff(Type1 *input, string objectName, int index, Type1 *output){
	json_t *rootObj;
	json_t *targetObj;
	const char *retString;
	int strLen;

	if(strcmp(typeid(input).name(), "PSs") == 0){
//	if(!(json_is_array(input)) && !(json_is_object(input))){


		rootObj = json_loads(input->c_str(), 0, &err);
		if(!json_is_object(rootObj)){
			cout << "json format invalid" << endl;
		}

	
		targetObj = json_object_get(rootObj, objectName.c_str());
		if (targetObj == NULL){
			cout << "found no status object" << endl;
		}
		retString = json_string_value(targetObj);
		if(retString != NULL){
			output->assign(retString);
		}
	}else{
/*		
		if(json_is_array(input)){
			output = json_array_get(input, index);
		}else{
			output = json_object_get(input, objectName.c_str());
		}
*/
	}

		
	json_decref(rootObj);
	json_decref(targetObj);	

}
#endif

