#ifndef _CodeCoverage_h_
#define _CodeCoverage_h_

#include "BPatch.h"
#include "BPatch_Vector.h"
#include "BPatch_image.h"
#include "BPatch_module.h"
#include "BPatch_thread.h"

class FunctionCoverage;
class FileLineInformation;

/** class to run code coverage on a mutatee program.
  * This class is responsible for selecting the functions
  * to be instrumented ( source code line information availability)
  * and then instruments each function either using all basic blocks 
  * or using dominator information.
  * This class contains array of functions that will be instrumented
  */
class CodeCoverage {
protected:
	/** bpatch object of the mutatee */
	BPatch bPatch;

	/** thread of the mutatee */
	BPatch_thread *appThread;

	/** image of the mutatee */
	BPatch_image *appImage;

	/** output file name where the coverage results are
	  * written as a binary file 
	  */
	char* coverageFileName;

	/** output file descriptor */
	ofstream coverageFile;

	/** the time between intervals in seconds */
	unsigned short deletionInterval;

	/** modules in the mutatee program */
	BPatch_Vector<BPatch_module*>* appModules;

	/** array of function records that will be instrumented
	  * or possible to instrument
	  */
	FunctionCoverage** instrumentedFunctions;

	/** number of the functions that might be instrumented*/
	int instrumentedFunctionCount;

	/** flag to identify whether dominator information
	  * will be used for instrumentation or not
	  */
	bool useDominator;

	/** the output file name is generated by adding a suffix
	  * to the name of the executable. this field stores
	  * the extension. The default value is ".dyncov"
	  */
	char* FILE_EXTENSION;

	/** pointer to the global object used for interval callback */
	static CodeCoverage* globalObject;

	/** array to represent files in the executable 
	  * sorted by fileName. Each entry in the array
	  * stores index to the entry in array of CodeCoverage records
	  * Note that CodeCoverage records are sorted according to
	  * file name and then function name
	  */
	static unsigned short fileCount;
	static unsigned short* fileStartIndex;
	static unsigned short* fileLineCount;

	Tcl_Interp* globalInterp;
	const char* statusBarName;

	unsigned short whichInterval;
	unsigned short totalDeletions;
	unsigned short totalCoveredLines;

	pthread_mutex_t statusUpdateLock;
	bool tclStatusChanged;
	char tclStatusBuffer[1024];

protected:
	/** interval callback function is defined to be friend*/
	friend void intervalCallback(int signalNo);
	friend class FunctionCoverage;

	/** method to validate whether function has its 
	  * source code line information available or not
	  * @param funcN name of the function
	  * @param min the minimum address of the line info records
	  *        of the function
	  */
	BPatch_function* validateFunction(const char* funcN,
					  unsigned long min);

	/** creates a new object of FunctionCoverage for the
	  * given function
	  * @param f function that is assigned to FunctionCoverage object
	  * @param funcN name of the function
	  * @param fileN name of the source file function is defined in
	  */
	FunctionCoverage* newFunctionCoverage(BPatch_function* f,
				const char* funcN, const char* fileN);

	/** method to print error message of this class */
	static int errorPrint(int code,char* text=NULL);

	/** method to print coverage results to the output file */
	int printCoverageInformation();

	/** method to update execution counts kept in FunctionCoverage
	  * objects for executed functions and basic blocks.
	  */
        int updateFCObjectInfo();

	/** interval callback */
	virtual int deletionIntervalCallback();

	/** method that returns whether a function is executed/instrumented
	  * during the execution of the mutatee. 
	  * @param i identifier of the function in the array
	  */
	virtual bool isInstrumented(int i);

	/** method that creates an array of files in the executable*/
	void createFileStructure();

public:
	pthread_mutex_t updateLock;

	/** constructor of the class */
	CodeCoverage();

	/** method that initialize the field of the class
	  * @param mutatee program to run
	  * @param interval time between intervals
	  * @param dominatorInfo flag whether dominator info will be used
	  * @param suffix the exetension to the exec file name to print results
	  */
	int initialize(char* mutatee[],unsigned short interval=0,
		       bool dominatorInfo=false,const char* suffix=".dyncov");

	/** method to select functions that will be possibly instrumented
	  * during the execution of mutatee (source code line information).
	  */
	int selectFunctions();

	/** method to instrument break point to the exit point of mutatee*/
	int instrumentExitHandle();

	/** method to initially instrument functions before the mutatee
	  * starts running. For preInstrumentation all functions are
	  * instrumented and for ondemand instrumentation only break
	  * points are inserted to the entry points
	  */
	virtual int instrumentInitial(); 

	/** method to execute the mutatee and checking the coverage
	  * results in fixed time intervals
	  */
	virtual int run();

	/** method to clean data structures in case of error */
	void terminate();

	/** method to register error call back function */
	BPatchErrorCallback registerErrorCallback(BPatchErrorCallback f);

	/** method to read the coverage results from the given file 
	  * and print in ascii to the standard output
	  * @param fN file name of the binary output file
	  */
	static int viewCodeCoverageInfo(char* fN);

	/** method that prints the tcl/tk command to create
	  * global data structure to be used for menu/list creation
	  */
	void getTclTkMenuListCreation(ofstream& file);
 
	/** method that retrieves all executed line information
	  */
	void getTclTkExecutedLines(ofstream& file);

	/** method that prints deletions and executed lines to a file
	  */
	void addTclTkFrequency();

	/** method to set tcl/tk related things to CodeCoverage */
	void setTclTkSupport(Tcl_Interp* interp,const char* statusBar);

	/** method that prepares the file for menu creation for view only */
	static int getTclTkMenuListForView(char* fN,ofstream& file);

	bool getTclStatusUpdateString(char* buffer,int length);

	/** destructor of the class */
	virtual ~CodeCoverage();
};

#endif /* _CodeCoverage_h_ */
