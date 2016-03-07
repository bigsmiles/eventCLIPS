/*******************************************************/
/*      "C" Language Integrated Production System      */
/*                                                     */
/*               CLIPS Version 6.24  07/01/05          */
/*                                                     */
/*                     MAIN MODULE                     */
/*******************************************************/

/*************************************************************/
/* Purpose:                                                  */
/*                                                           */
/* Principal Programmer(s):                                  */
/*      Gary D. Riley                                        */
/*                                                           */
/* Contributing Programmer(s):                               */
/*                                                           */
/* Revision History:                                         */
/*                                                           */
/*      6.24: Moved UserFunctions and EnvUserFunctions to    */
/*            the new userfunctions.c file.                  */
/*                                                           */
/*************************************************************/

/***************************************************************************/
/*                                                                         */
/* Permission is hereby granted, free of charge, to any person obtaining   */
/* a copy of this software and associated documentation files (the         */
/* "Software"), to deal in the Software without restriction, including     */
/* without limitation the rights to use, copy, modify, merge, publish,     */
/* distribute, and/or sell copies of the Software, and to permit persons   */
/* to whom the Software is furnished to do so.                             */
/*                                                                         */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS */
/* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT   */
/* OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY  */
/* CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES */
/* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN   */
/* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF */
/* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.          */
/*                                                                         */
/***************************************************************************/
#include <process.h>
#include <windows.h>

#include <stdio.h>
#include "setup.h"
#include "clips.h"

#include "move.h"
#include "input.h"


int main(int, char *[]);
void UserFunctions(void);
void EnvUserFunctions(void *);

/****************************************/
/* main: Starts execution of the expert */
/*   system development environment.    */
/****************************************/


#if THREAD
CRITICAL_SECTION g_cs;
//CRITICAL_SECTION g_debug; //解决同步问题?
CRITICAL_SECTION g_csDebug, g_csDebug1, g_csDebug2;
HANDLE g_debug;
HANDLE g_hSemaphoreBufferEmpty, g_hSemaphoreBufferFull;
HANDLE g_hSemaphoreBuffer;
#endif
int main(
	int argc,
	char *argv[])
{
	void *theEnv;
	//add by xuchao
	void *betaEnv;
#if THREAD
	InitializeCriticalSection(&g_cs);
	InitializeCriticalSection(&g_csDebug);
	InitializeCriticalSection(&g_csDebug1);
	InitializeCriticalSection(&g_csDebug2);
	//g_hSemaphoreBufferEmpty = CreateSemaphore(NULL, 100, 100, NULL);
	//g_hSemaphoreBufferFull = CreateSemaphore(NULL, 0, 100, NULL);
	g_hSemaphoreBuffer = CreateSemaphore(NULL, 0, 20000, NULL);
	g_debug = CreateSemaphore(NULL, 0, 1, NULL);
	HANDLE hThread;
#endif

	
	theEnv = CreateEnvironment();
	betaEnv = CreateEnvironment();
	//
	
	
	//*EvaluationData(betaEnv) = *EvaluationData(theEnv);
	
	//RerouteStdin(theEnv, argc, argv);

	EnvLoad(theEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");
	EnvLoad(betaEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");

	//*DefmoduleData(betaEnv) = *DefmoduleData(theEnv);
	/*
	*PatternData(theEnv) = *PatternData(betaEnv);
	*DeftemplateData(theEnv) = *DeftemplateData(betaEnv);
	
	*AgendaData(theEnv) = *AgendaData(betaEnv);
	*EngineData(theEnv) = *EngineData(betaEnv);
	*DefruleData(theEnv) = *DefruleData(betaEnv);
	*FactData(theEnv) = *FactData(betaEnv);
	*/
	//EnvReset(theEnv);
	//EnvReset(betaEnv);
	struct environmentData *env1 = (struct environmentData*) theEnv;
	struct environmentData *env2 = (struct environmentData*) betaEnv;
	//*env1->theData = *env2->theData;
	
	
	
#if THREAD
	//add by xuchao,start this execute thread
	hThread = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, betaEnv, 0, NULL);
#endif

	
	//EnvAssertString(theEnv, "(data 10 20)");
	//EnvAssertString(theEnv, "(foo 10 30)");
	//VariableFactAssert(theEnv, "data", 10, 20);
	//VariableFactAssert(theEnv, "foo", 10, 30);
	int total =150;
	char fields[4][8] = { "data", "bar", "foo", "room" };
	char names[10][8] = { "tom", "tim", "jack", "mike", "sam", "mary", "lily", "bob", "alice", "lucy" };
	srand((unsigned int)time(0));
	//EnvAssertString(theEnv, "(student (id 100)(name \"tom\"))");
	

	FILE *pFile = fopen("D:\\VS\\stdCLIPS\\Debug\\facts.txt", "r");

	if (pFile == NULL)
		printf("error!\n");
	char tmpBuffer[100];
	printf("****************\n");
	total = 0;
	while (fgets(tmpBuffer, 100, pFile))
	{
		//printf("%s\n", tmpBuffer);
		//Sleep(15);
		//printf("total:%d\n", total++);
		EnvAssertString(theEnv, tmpBuffer);
		
		//EnvRun(theEnv, -1);
		//EnvRun(betaEnv, -1);
	}
	//EnvRun(theEnv, -1);
	/*while (total-- > 0)
	{
		//printf("total: %d\n", total);
		Sleep(10);
		EnvIncrementGCLocks(theEnv);
		switch (rand() % 5)
		{
		case 0:
			VariableFactAssert1(theEnv, fields[rand() % 4], rand() % 10, rand() % 10);
			break;
		case 1:
			VariableFactAssert2(theEnv, rand() % 10);
			break;
		case 2:
			//VariableFactAssert3(theEnv, rand() % 10, names[rand() % 3]);
			break;
		case 3:
			VariableFactAssert4(theEnv, rand() % 10, names[rand() % 3]);
			break;
		case 4:
			VariableFactAssert5(theEnv, rand() % 10, names[rand() % 3]);
			break;
		default:
			printf("error!\n");
		}
		//EnvDecrementGCLocks(theEnv);
	}
	
	
	*/
	Sleep(5000);
	//CommandLoop(theEnv);
	//CommandLoop(betaEnv);
	//char betaFact[10000];
	//EnvFacts(betaEnv, betaFact, NULL, -1, -1, -1);
	//printf("betaFact:\n%s\n", betaFact);


#if THREAD
	CloseHandle(g_hSemaphoreBufferEmpty);
	CloseHandle(g_hSemaphoreBufferFull);
	CloseHandle(g_debug);
	DeleteCriticalSection(&g_cs);
	//DeleteCriticalSection(&g_debug);
	//CloseHandle(hThread);
#endif

	/*==================================================================*/
	/* Control does not normally return from the CommandLoop function.  */
	/* However if you are embedding CLIPS, have replaced CommandLoop    */
	/* with your own embedded calls that will return to this point, and */
	/* are running software that helps detect memory leaks, you need to */
	/* add function calls here to deallocate memory still being used by */
	/* CLIPS. If you have a multi-threaded application, no environments */
	/* can be currently executing. If the ALLOW_ENVIRONMENT_GLOBALS     */
	/* flag in setup.h has been set to TRUE (the default value), you    */
	/* call the DeallocateEnvironmentData function which will call      */
	/* DestroyEnvironment for each existing environment and then        */
	/* deallocate the remaining data used to keep track of allocated    */
	/* environments. Otherwise, you must explicitly call                */
	/* DestroyEnvironment for each environment you create.              */
	/*==================================================================*/

	/* DeallocateEnvironmentData(); */
	/* DestroyEnvironment(theEnv); */
	
	return(-1);
}
