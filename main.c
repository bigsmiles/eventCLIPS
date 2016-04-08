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

#define DEBUGTIME 1
int main(int, char *[]);
void UserFunctions(void);
void EnvUserFunctions(void *);

/****************************************/
/* main: Starts execution of the expert */
/*   system development environment.    */
/****************************************/
void *betaEnv;

#if THREAD 
CRITICAL_SECTION g_cs;
//CRITICAL_SECTION g_debug; //解决同步问题?
CRITICAL_SECTION g_csDebug, g_csDebug1, g_csDebug2,g_runDebug;
HANDLE g_debug;

HANDLE g_hSemaphoreBuffer,g_hSemaphoreBufferOfThread1, g_hSemaphoreBufferOfThread2;
#endif

int main(
	int argc,
	char *argv[])
{
	void *theEnv;
	//add by xuchao
	
	void *thirdEnv;
#if THREAD
	InitializeCriticalSection(&g_cs);
	//InitializeCriticalSection(&g_csDebug);
	InitializeCriticalSection(&g_runDebug);
	//InitializeCriticalSection(&g_csDebug1);
	//InitializeCriticalSection(&g_csDebug2);
	//g_hSemaphoreBufferEmpty = CreateSemaphore(NULL, 100, 100, NULL);
	//g_hSemaphoreBufferFull = CreateSemaphore(NULL, 0, 100, NULL);
	//g_hSemaphoreBuffer = CreateSemaphore(NULL, 0, 20000, NULL);
	g_hSemaphoreBufferOfThread1 = CreateSemaphore(NULL, 0, 20000, NULL);
	g_hSemaphoreBufferOfThread2 = CreateSemaphore(NULL, 0, 20000, NULL);
	//g_debug = CreateSemaphore(NULL, 0, 1, NULL);
	HANDLE hThread,hThread1;
#endif

	
	theEnv = CreateEnvironment();
	betaEnv = CreateEnvironment();
	thirdEnv = CreateEnvironment();
	//
	
	
	//*EvaluationData(betaEnv) = *EvaluationData(theEnv);
	
	//RerouteStdin(theEnv, argc, argv);

	EnvLoad(theEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");
	EnvLoad(betaEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");
	EnvLoad(thirdEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");


	//EnvReset(theEnv);
	//EnvReset(betaEnv);
	//struct environmentData *env1 = (struct environmentData*) theEnv;
	//struct environmentData *env2 = (struct environmentData*) betaEnv;
	//*env1->theData = *env2->theData;

	struct ThreadNode *env1 = (struct ThreadNode*)malloc(sizeof(struct ThreadNode));
	env1->theEnv = betaEnv; env1->threadTag = 0;
	struct ThreadNode *env2 = (struct ThreadNode*)malloc(sizeof(struct ThreadNode));
	env2->theEnv = thirdEnv; env2->threadTag = 1;
	
	
	
#if THREAD
	//add by xuchao,start this execute thread
#if MUTILTHREAD
	hThread = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, env1, 0, NULL);
	SetThreadAffinityMask(hThread, 1 << 1);//线程指定在某个cpu运行
	hThread1 = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, env2, 0, NULL);
	SetThreadAffinityMask(hThread1, 1 << 2);//线程指定在某个cpu运行
#else if
	hThread = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, betaEnv, 0, NULL);
#endif
	//hThread1 = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, thirdEnv, 0, NULL);
	//SetThreadAffinityMask(hThread1, 1 << 2);//线程指定在某个cpu运行
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
	char tmpBuffer[200];
	//printf("****************\n");
	total = 0;
	int tmpBufferLenth = 0;
	char timeStr[100];
	__int64 counterOfTimer;
	LARGE_INTEGER large_time,start,end,finish,freq;
	int cnt = 0;
	QueryPerformanceCounter(&large_time);
	QueryPerformanceFrequency(&freq);
	//DWORD start = GetTickCount();
	QueryPerformanceCounter(&start);
	while (fgets(tmpBuffer, 100, pFile))
	{
		//if (cnt++ >= 20)break;
#if DEBUGTIME
		tmpBufferLenth = strlen(tmpBuffer) - 1;
		
		if (tmpBuffer[tmpBufferLenth - 1] == ')' && tmpBuffer[tmpBufferLenth - 2] != ')'){
			//tmpBuffer[tmpBufferLenth - 1] = '\0';
			//QueryPerformanceCounter(&large_time);
			//counterOfTimer = large_time.QuadPart;
			
			//sprintf(timeStr, " %I64d)", counterOfTimer);
			timeStr[0] = '\0';
		}
		else if (tmpBuffer[tmpBufferLenth - 1] == ')' && tmpBuffer[tmpBufferLenth - 2] == ')'){
			tmpBuffer[tmpBufferLenth - 1] = '\0';
			//QueryPerformanceCounter(&large_time);
			sprintf(timeStr, "(time %I64d))", large_time.QuadPart);
		}
		strcat(tmpBuffer, timeStr);
#else if
		
#endif
		//printf("%s\n", tmpBuffer);
		//Sleep(200);
		EnvAssertString(theEnv, tmpBuffer);
		
		//EnvRun(betaEnv, -1);
	}
	QueryPerformanceCounter(&end);
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
	//WaitForSingleObject(hThread, INFINITE);
	//hThread1 = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, env2, 0, NULL);
	//hThread = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, env1, 0, NULL);
	Sleep(20000);
	//CommandLoop(theEnv);
	//CommandLoop(betaEnv);
	//CommandLoop(thirdEnv);
	//DWORD end = GetTickCount();
	QueryPerformanceCounter(&finish);
	printf("input time: %lf\n", 1.0 * (end.QuadPart - start.QuadPart) / freq.QuadPart);
	printf("time:%d\n", (finish.QuadPart - start.QuadPart) / freq.QuadPart);


#if THREAD
	//CloseHandle(g_hSemaphoreBufferEmpty);
	//CloseHandle(g_hSemaphoreBufferFull);
	//CloseHandle(g_debug);
	//DeleteCriticalSection(&g_cs);
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
