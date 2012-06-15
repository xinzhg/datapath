//
//  Copyright 2012 Alin Dobra and Christopher Jermaine
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
#include "CodeGenerator.h"
#include "AttributeManager.h"
#include "QueryManager.h"
#include "Timer.h"
#include "ExternalCommands.h"

#include <iostream>
#include <iomanip>

/** This version of the code generator generates each waypoint code
	* separately and compiles using a templated Makefile. The templated
	* makefile expects a correct variable OBJECTS to be defined. This
	* variable is a list of .o files that are put together in the
	* Generated.so library
**/


// constructor for the implementation class
CodeGeneratorImp::CodeGeneratorImp(const char* _srcDir, EventProcessor& _coordinator):
	srcDir(_srcDir)
#ifdef  DEBUG_EVPROC
	,EventProcessorImp(true, "CodeGenerator")
#endif
{
	coordinator.copy(_coordinator);

	RegisterMessageProcessor(GenerateNewCodeMessage::type, &GenerateNewCode, 1
		/* lowest priority */);
}


// GenerateNewCode handler definition
MESSAGE_HANDLER_DEFINITION_BEGIN(CodeGeneratorImp, GenerateNewCode, GenerateNewCodeMessage)
{
	// declare our output structure
	WPConfDataContainer outConfigs;

	Timer clock;
	clock.Restart();

	// get code with the current request from the message
	evProc.Generate(msg.dirName, msg.configs, outConfigs);

	cout << "Code generation took " << setprecision(6) << clock.GetTime() <<
		" seconds" << endl;

	// return the output to the sender
	GeneratedCodeMessage_Factory(evProc.coordinator, outConfigs);
}
MESSAGE_HANDLER_DEFINITION_END

void CodeGeneratorImp::AddPreamble(ofstream &out) {

	// get the current time in a nice, ascii form
	time_t rawtime = time(NULL);
	tm *timeinfo = localtime(&rawtime);

	// add the preamble with some debugging comments
	out << "dnl # CODE GENERATED BY DATAPATH ON " << asctime(timeinfo) << endl;
	out << "include(Modules.m4)" << endl;
	out << "M4_CODE_GENERATION_PREAMBLE" << endl << endl;
}

void CodeGeneratorImp::Build(string dirPath, void *&module, string objects) {

	// let's make the call string for generate.sh, which takes the
	// directory itself as a parameter
	string call = "./generate.sh " + dirPath + " \"" + objects + '\"' + " 1>&2";
	cout << call << "\n";

	// do the system call, generate.sh must return 0 on success
	int sysret = external_command(call.c_str());
	FATALIF(sysret != 0, "Unable to do the code generation on directory %s!",
		dirPath.c_str());

	// make the file name for our module
	string modFname = dirPath + "/Generated.so";

	// open up the module, lazy mode
	module = dlopen(modFname.c_str(), RTLD_LAZY);

	if (module == NULL){
		FATAL("Unable to load generated code from file %s! \nThe error is %s\n",
			modFname.c_str(), dlerror());
	}
}

void CodeGeneratorImp::Generate(string dirName,
	SymbolicWPConfigContainer &configMessages, WPConfDataContainer &generatedCode) {


	// ask the attribute manager to drop its info in our directory
	AttributeManager &am = AttributeManager::GetAttributeManager();
	string amFname = dirName + "/Attributes.m4";
	am.GenerateM4Files(amFname);


	// ask the query manager to drop its info in our directory
	QueryManager &qm = QueryManager::GetQueryManager();
	string qmFname = dirName + "/Queries.m4";
	qm.GenerateM4Files(qmFname);


	// we accumulate the objects here
	string objects;

	// structure to keep the CGs
	vector<WayPointCG *> cgs;

	// now, we will add the code for all our waypoints!
	for (configMessages.MoveToStart();!configMessages.AtEnd();configMessages.Advance()) {
		WayPointID wpID=configMessages.Current().GetID();
		IDInfo info;
		wpID.getInfo(info);
		string wpName = info.getName();

		objects+=wpName+".o ";

		// create our Main.m4 file!
		string wpFname = dirName + "/"+wpName+".m4";
		ofstream mout;
		mout.open(wpFname.c_str());

		// insert the preamble
		AddPreamble(mout);

		// get a CG from the factory
		WayPointCG *cg = CGFactory(configMessages.Current());

		// make it write the code in Main.m4
		cg->GenerateM4Code(mout);

		// put it in the array -- we'll need it later on
		cgs.push_back(cg);

		// close our Waypoint.m4 file
		mout.close();
	}


	// build/compile/load it!
	void *mod;
	Build(dirName, mod, objects);

	// now, we will obtain the functions for all our waypoints
	for (int i=0;i<cgs.size();i++) {
		// get the code for the current CG as a WPConfig structure
		WayPointConfigureData wpc;
		cgs[i]->ExtractCode(mod, wpc);

		// add that structure to our output collection
		generatedCode.Insert(wpc);

		// kill the cg -- we don't need it anymore
		delete cgs[i];
	}

	// clear the array of CGs
	cgs.clear();

	// we are done!
}
