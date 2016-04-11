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



#if THREAD 
//void *betaEnv;
CRITICAL_SECTION g_cs;
//CRITICAL_SECTION g_debug; //解决同步问题?
CRITICAL_SECTION g_csDebug, g_runDebug;//g_csDebug1, g_csDebug2,
HANDLE g_debug;
HANDLE g_hSemaphoreBuffer,g_hSemaphoreBufferOfThread1, g_hSemaphoreBufferOfThread2;
#endif

int main(
	int argc,
	char *argv[])
{
	void *theEnv;
#if THREAD
	void *betaEnv;
	void *thirdEnv;

	InitializeCriticalSection(&g_cs);
	InitializeCriticalSection(&g_runDebug);
#if !MUTILTHREAD
	g_hSemaphoreBuffer = CreateSemaphore(NULL, 0, 20000, NULL);
#else
	g_hSemaphoreBufferOfThread1 = CreateSemaphore(NULL, 0, 20000, NULL);
	g_hSemaphoreBufferOfThread2 = CreateSemaphore(NULL, 0, 20000, NULL);
#endif
	//g_debug = CreateSemaphore(NULL, 0, 1, NULL);
	HANDLE hThread,hThread1;
#endif

	
	theEnv = CreateEnvironment();
#if THREAD
	betaEnv = CreateEnvironment();
	thirdEnv = CreateEnvironment();
#endif
	

	EnvLoad(theEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");
#if THREAD
	EnvLoad(betaEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");
	EnvLoad(thirdEnv, "D:\\VS\\testCLPS\\testCLIPS\\Debug\\debug.clp");

	struct ThreadNode *env1 = (struct ThreadNode*)malloc(sizeof(struct ThreadNode));
	env1->theEnv = betaEnv; env1->threadTag = 0;
	struct ThreadNode *env2 = (struct ThreadNode*)malloc(sizeof(struct ThreadNode));
	env2->theEnv = thirdEnv; env2->threadTag = 1;
#endif	
	
	
#if THREAD
	//add by xuchao,start this execute thread
#if MUTILTHREAD
	hThread = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, env1, 0, NULL);
	SetThreadAffinityMask(hThread, 1 << 1);//线程指定在某个cpu运行
	hThread1 = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, env2, 0, NULL);
	SetThreadAffinityMask(hThread1, 1 << 2);//线程指定在某个cpu运行
#else if
	hThread = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, env1, 0, NULL);
#endif
	//hThread1 = (HANDLE)_beginthreadex(NULL, 0, MoveOnJoinNetworkThread, thirdEnv, 0, NULL);
	//SetThreadAffinityMask(hThread1, 1 << 2);//线程指定在某个cpu运行
#endif

#if AUTOTEST
	int total =150;
	char fields[4][8] = { "data", "bar", "foo", "room" };
	char names[10][8] = { "tom", "tim", "jack", "mike", "sam", "mary", "lily", "bob", "alice", "lucy" };
	srand((unsigned int)time(0));
	//EnvAssertString(theEnv, "(student (id 100)(name \"tom\"))");
	

	FILE *pFile = fopen("D:\\VS\\stdCLIPS\\Debug\\facts.txt", "r");
	
	if (pFile == NULL)
		printf("error!\n");
	char tmpBuffer[200];
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
		
#if DEBUGTIME
		tmpBufferLenth = strlen(tmpBuffer) - 1;
		
		if (tmpBuffer[tmpBufferLenth - 1] == ')' && tmpBuffer[tmpBufferLenth - 2] != ')'){
			timeStr[0] = '\0';
		}
		else if (tmpBuffer[tmpBufferLenth - 1] == ')' && tmpBuffer[tmpBufferLenth - 2] == ')'){
			tmpBuffer[tmpBufferLenth - 1] = '\0';
			sprintf(timeStr, "(time %I64d))", large_time.QuadPart);
		}
		strcat(tmpBuffer, timeStr);
#else if
		
#endif
		EnvAssertString(theEnv, tmpBuffer);
	}
	QueryPerformanceCounter(&end);
	Sleep(50000);
#if !THREAD
	CommandLoop(theEnv);
#endif
	QueryPerformanceCounter(&finish);
	printf("input time: %lf\n", 1.0 * (end.QuadPart - start.QuadPart) / freq.QuadPart);
	printf("time:%d\n", (finish.QuadPart - start.QuadPart) / freq.QuadPart);
#else 
	//CommandLoop(theEnv);
	//CommandLoop(betaEnv);
	//CommandLoop(thirdEnv);
	
#endif 
#if THREAD
	CloseHandle(g_hSemaphoreBufferOfThread1);
	CloseHandle(g_hSemaphoreBufferOfThread2);
	CloseHandle(g_debug);
	//DeleteCriticalSection(&g_cs);
	DeleteCriticalSection(&g_runDebug);
	CloseHandle(hThread);
#if MUTILTHREAD
	CloseHandle(hThread1);
#endif
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
