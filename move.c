/*******************************************************/
/*      "C" Language Integrated Production System      */
/*                                                     */
/*                                                     */
/*                                                     */
/*                      add by xuchao                  */
/*******************************************************/
#define _MOVE_SOURCE_

#include <stdio.h>
#define _STDIO_INCLUDED_
#include <string.h>

#include "setup.h"

//#include "drive.h"
#include <windows.h>
#include <process.h>

#include "move.h"
#include "drive.h"
#include "reteutil.h"
#include "factmngr.h"

//add by xuchao
#include "envrnmnt.h"


struct JoinNodeList *joinNodeListHead;
struct JoinNodeList *joinNodeListTail;
extern struct activeJoinNode *activeNodeHead;
extern struct activeJoinNode *activeNodeTail;
extern CRITICAL_SECTION g_cs;
extern HANDLE g_hSemaphoreBufferEmpty, g_hSemaphoreBufferFull, g_hSemaphoreBuffer;
extern HANDLE g_debug;

struct activeJoinNode* GetBestOneActiveNode(void *theEnv);
int EstimateJoinNodeToEndTime(struct joinNode *curJoinNode);

globle void MoveOnJoinNetwork(void*theEnv)
{
	struct activeJoinNode *currentActiveNode;
	struct joinNode *currentJoinNode;
	struct partialMatch *currentPartialMatch;
	struct partialMatch *lhsBinds;
	struct partialMatch *rhsBinds;
	unsigned long hashValue;
	char enterDirection;
	struct fact *theFact;
	struct multifieldMarker *theMarks;
	struct patternNodeHeader *theHeader;
	unsigned long hashOffset;

	while ((currentActiveNode = GetBestOneActiveNode(theEnv)) != NULL)
	{
		currentJoinNode = currentActiveNode->currentJoinNode;
		currentPartialMatch = currentActiveNode->currentPartialMatch;
		lhsBinds = currentActiveNode->lhsBinds;
		rhsBinds = currentActiveNode->rhsBinds;
		hashValue = currentActiveNode->hashValue;
		enterDirection = currentActiveNode->curPMOnWhichSide;
		theFact =(struct fact *) currentActiveNode->theEntity;
		theMarks = currentActiveNode->markers;
		theHeader = currentActiveNode->theHeader;

		

		if (currentJoinNode->firstJoin)
		{
			// I have modified the CreateAlpahMatch()!! It should named be CreateAlphaMatch if not have else return the partialMatch
			currentPartialMatch = CreateAlphaMatch(theEnv, theFact, theMarks, theHeader, currentActiveNode->hashOffset);
			currentPartialMatch->owner = theHeader;

			((struct patternMatch *)theFact->list)->theMatch = currentPartialMatch;
			EmptyDrive(theEnv, currentJoinNode, currentPartialMatch);
		}
		else if (currentActiveNode->curPMOnWhichSide == LHS)
		{
			UpdateBetaPMLinks(theEnv, currentPartialMatch, lhsBinds, rhsBinds, currentJoinNode, hashValue, enterDirection);
			NetworkAssertLeft(theEnv, currentPartialMatch, currentJoinNode);
		}
		else if (currentActiveNode->curPMOnWhichSide == RHS)
		{
			//UpdateBetaPMLinks(theEnv, currentPartialMatch, lhsBinds, rhsBinds, currentJoinNode, hashValue, enterDirection);
			currentPartialMatch = CreateAlphaMatch(theEnv, theFact, theMarks, theHeader, currentActiveNode->hashOffset);
			currentPartialMatch->owner = theHeader;

			((struct patternMatch *)theFact->list)->theMatch = currentPartialMatch;
			NetworkAssertRight(theEnv, currentPartialMatch, currentJoinNode);
		}
		else
		{
			//error
		}

	}
	return;
}

//min Time from curNode to endNode
globle int EstimateJoinNodeToEndTime(struct joinNode *curJoinNode){

	
	struct expr *firstTest = curJoinNode->networkTest;
	struct expr *p = firstTest;
	int numberOfTest = 0;
	struct joinLink * CurJoinLink = NULL;
	if (curJoinNode->ruleToActivate != NULL)return 0;

	while (p != NULL){
		numberOfTest += 1;
		p = p->nextArg;
	}
	int minTime = 999999999;
	for (CurJoinLink = curJoinNode->nextLinks; CurJoinLink != NULL; CurJoinLink = CurJoinLink->next){
		int time = EstimateJoinNodeToEndTime(CurJoinLink->join);
		if (time < minTime){
			time = minTime;
		}
	}
	//minTime = 0;
	return numberOfTest + minTime;
}
globle struct activeJoinNode* GetBestOneActiveNode(void *theEnv)
{
	struct activeJoinNode *rtnNode = NULL;
	struct activeJoinNode *pNode = NULL;
	struct JoinNodeList *oneListNode = NULL;
#if THREAD
	//WaitForSingleObject(g_hSemaphoreBufferFull, INFINITE);
	WaitForSingleObject(g_hSemaphoreBuffer, INFINITE);
	EnterCriticalSection(&g_cs);
#endif
#if SPEEDUP
	//int thread_flag = 0;
	//if (GetEnvironmentByIndex(1) == theEnv) thread_flag = 1;
	//else thread_flag = 0;

	if(joinNodeListHead->next == NULL)return NULL;
	oneListNode = joinNodeListHead->next;
	long long MinTime = 0x3fffffffffffffff;
	long long flag = 100000;
	struct joinNode *tmp = NULL;
	int cnt = 0;
	while(oneListNode != NULL){
		cnt += 1;


		//int join_flag = oneListNode->join->depth;
		//printf("flag: %d %d\n", join_flag % 2,thread_flag);
		if (oneListNode->join->numOfActiveNode > 0){// && (join_flag % 2) == thread_flag
			//printf("flag: %d %d\n", join_flag % 2,thread_flag);
			long long curTime = (oneListNode->join->nodeMinSalience * flag) + oneListNode->join->activeJoinNodeListHead->next->timeTag;
			if(curTime < MinTime)
			{
				rtnNode = oneListNode->join->activeJoinNodeListHead->next;
				tmp = oneListNode->join;
				MinTime = curTime;
				//cnt += 1;
			}
		}
		oneListNode = oneListNode->next;
	}
	if (rtnNode != NULL)
	{
		rtnNode->pre->next = rtnNode->next;
		if (rtnNode->next != NULL)rtnNode->next->pre = rtnNode->pre;
		if (rtnNode == tmp->activeJoinNodeListTail)tmp->activeJoinNodeListTail = rtnNode->pre;
		rtnNode->next = NULL;
		rtnNode->pre = NULL;
		tmp->numOfActiveNode -= 1;
	}
#else
	if (activeNodeHead->next == NULL)return NULL;
	pNode = activeNodeHead->next;
	rtnNode = pNode;
	long long MinTime = 0x3ffffffffffff;
	long long flag = 100000;

	//no any algorithm ,just take the head(first) one
	if (activeNodeHead->next != NULL) rtnNode = activeNodeHead->next;
	
	/*while(pNode != NULL)
	{
		if (pNode->currentJoinNode->nodeMaxSalience > rtnNode->currentJoinNode->nodeMaxSalience)
		//int curTime = EstimateJoinNodeToEndTime(pNode->currentJoinNode);  //can be optimized
		//long long curTime = (pNode->currentJoinNode->nodeMinSalience * flag) + pNode->timeTag;
		//if (curTime < MinTime)
		{
			rtnNode = pNode;
			//MinTime = curTime;
		}
		pNode = pNode->next;
	}*/
	rtnNode->pre->next = rtnNode->next;
	if(rtnNode->next != NULL)rtnNode->next->pre = rtnNode->pre;
	if (rtnNode == activeNodeTail) activeNodeTail = rtnNode->pre;
	rtnNode->next = NULL;
	rtnNode->pre = NULL;
#endif
#if THREAD
	LeaveCriticalSection(&g_cs);
	//ReleaseSemaphore(g_hSemaphoreBufferEmpty, 1, NULL);
#endif
	return rtnNode;
}
globle void AddNodeFromAlpha(
	void* theEnv,
	struct joinNode* curNode, //listOfJoins
	unsigned long hashValue,
	struct multifieldMarker *theMarks,
	struct fact* theFact,
	struct patternNodeHeader* header
	)
{
	
	DATA_OBJECT timeVal;
	long long time = 0;
	EnvDirectGetSlot(theEnv, (void*)theFact, "time", &timeVal);
	if (GetType(timeVal) == INTEGER){
		//printf("arg1 is integer: %d\n", DOToLong(arg1));
		time = DOToLong(timeVal);
	}
	struct activeJoinNode* oneNode = (struct activeJoinNode*) malloc(sizeof(struct activeJoinNode));
	oneNode->currentJoinNode = curNode;
	//theMatch = NULL;
	oneNode->currentPartialMatch = NULL; //null
	oneNode->curPMOnWhichSide = RHS;
	oneNode->markers = theMarks;
	oneNode->theEntity = theFact;
	oneNode->theHeader = header;// (struct patternNodeHeader *)&thePattern->header;
	oneNode->hashOffset = hashValue;
	oneNode->hashValue = hashValue;
	oneNode->next = NULL;
	oneNode->timeTag = time;

#if THREAD
	//WaitForSingleObject(g_hSemaphoreBufferEmpty, INFINITE);
	EnterCriticalSection(&g_cs);
#endif
#if SPEEDUP
	if (curNode->activeJoinNodeListHead->next == NULL){
		curNode->activeJoinNodeListHead->next = oneNode;
		oneNode->pre = curNode->activeJoinNodeListHead;
	}
	else{
		curNode->activeJoinNodeListTail->next = oneNode;
		oneNode->pre = curNode->activeJoinNodeListTail;
	}
	curNode->activeJoinNodeListTail = oneNode;
	curNode->numOfActiveNode += 1;

#else 
	if (activeNodeHead->next == NULL){ activeNodeHead->next = oneNode; oneNode->pre = activeNodeHead; }
	else
	{
		activeNodeTail->next = oneNode;
		oneNode->pre = activeNodeTail;
	}
	activeNodeTail = oneNode;
#endif
#if THREAD
	LeaveCriticalSection(&g_cs);
	//ReleaseSemaphore(g_hSemaphoreBufferFull, 1, NULL);
	ReleaseSemaphore(g_hSemaphoreBuffer, 1, NULL);
#endif
}
globle void AddOneActiveNode(
	void* theEnv, 
	struct partialMatch* partialMatch,
	struct partialMatch* lhsBinds,
	struct partialMatch* rhsBinds,
	struct joinNode* curNode, 
	unsigned long hashValue,
	char whichEntry)
{
	struct activeJoinNode *oneActiveNode = (struct activeJoinNode*) malloc(sizeof(struct activeJoinNode));
	oneActiveNode->curPMOnWhichSide = whichEntry;
	oneActiveNode->currentJoinNode = curNode;
	oneActiveNode->currentPartialMatch = partialMatch;
	oneActiveNode->lhsBinds = lhsBinds;
	oneActiveNode->rhsBinds = rhsBinds;
	oneActiveNode->hashValue = hashValue;
	oneActiveNode->hashOffset = hashValue;
	oneActiveNode->next = NULL;

	oneActiveNode->timeTag = partialMatch->timeTag;

#if THREAD
	//WaitForSingleObject(g_hSemaphoreBufferEmpty, INFINITE);
	EnterCriticalSection(&g_cs);
#endif
#if SPEEDUP
	if (curNode->activeJoinNodeListHead->next == NULL){
		curNode->activeJoinNodeListHead->next = oneActiveNode;
		oneActiveNode->pre = curNode->activeJoinNodeListHead;
	}
	else{
		curNode->activeJoinNodeListTail->next = oneActiveNode;
		oneActiveNode->pre = curNode->activeJoinNodeListTail;
	}
	curNode->activeJoinNodeListTail = oneActiveNode;
	curNode->numOfActiveNode += 1;
#else
	if (activeNodeHead->next == NULL)
	{
		activeNodeHead->next = oneActiveNode;
		oneActiveNode->pre = activeNodeHead;
		
	}
	else
	{
		activeNodeTail->next = oneActiveNode;
		oneActiveNode->pre = activeNodeTail;
		
	}
	activeNodeTail = oneActiveNode;
#endif
#if THREAD
	LeaveCriticalSection(&g_cs);
	//ReleaseSemaphore(g_hSemaphoreBufferFull, 1, NULL);
	ReleaseSemaphore(g_hSemaphoreBuffer, 1, NULL);
#endif

	return;
}
unsigned int __stdcall MoveOnJoinNetworkThread(void *pM)
{
	void *theEnv = (void *)pM;
	
	struct activeJoinNode *currentActiveNode;
	struct joinNode *currentJoinNode;
	struct partialMatch *currentPartialMatch;
	struct partialMatch *lhsBinds;
	struct partialMatch *rhsBinds;
	unsigned long hashValue;
	char enterDirection;
	struct fact *theFact;
	struct multifieldMarker *theMarks;
	struct patternNodeHeader *theHeader;
	unsigned long hashOffset;
	long long timeTag;


	while (1)
	{
		currentActiveNode = GetBestOneActiveNode(theEnv);
		if (currentActiveNode == NULL)continue;

		currentJoinNode = currentActiveNode->currentJoinNode;
		currentPartialMatch = currentActiveNode->currentPartialMatch;
		lhsBinds = currentActiveNode->lhsBinds;
		rhsBinds = currentActiveNode->rhsBinds;
		hashValue = currentActiveNode->hashValue;
		enterDirection = currentActiveNode->curPMOnWhichSide;
		theFact = (struct fact *) currentActiveNode->theEntity;
		theMarks = currentActiveNode->markers;
		theHeader = currentActiveNode->theHeader;
		timeTag = currentActiveNode->timeTag;

		//WaitForSingleObject(g_debug, INFINITE);
		//EnvRun(theEnv, -1);
		if (currentJoinNode->firstJoin)
		{
			currentPartialMatch = CreateAlphaMatch(theEnv, theFact, theMarks, theHeader, currentActiveNode->hashOffset);
			currentPartialMatch->owner = theHeader;
			currentPartialMatch->timeTag = timeTag;

			((struct patternMatch *)theFact->list)->theMatch = currentPartialMatch;
			EmptyDrive(theEnv, currentJoinNode, currentPartialMatch);
		}
		else if (currentActiveNode->curPMOnWhichSide == LHS)
		{
			UpdateBetaPMLinks(theEnv, currentPartialMatch, lhsBinds, rhsBinds, currentJoinNode, hashValue, enterDirection);
			NetworkAssertLeft(theEnv, currentPartialMatch, currentJoinNode);
		
		}
		else if (currentActiveNode->curPMOnWhichSide == RHS)
		{
			//UpdateBetaPMLinks(theEnv, currentPartialMatch, lhsBinds, rhsBinds, currentJoinNode, hashValue, enterDirection);
			currentPartialMatch = CreateAlphaMatch(theEnv, theFact, theMarks, theHeader, currentActiveNode->hashOffset);
			currentPartialMatch->owner = theHeader;

			((struct patternMatch *)theFact->list)->theMatch = currentPartialMatch;
			NetworkAssertRight(theEnv, currentPartialMatch, currentJoinNode);
		}
		else
		{
			//error
		}
		

	}

	return 0;
}
globle double SlowDown()
{
	//Sleep(3000);
	int slow = 5000;
	int alpha = 123;
	double result = 1;
	while (slow-- > 0){
		result = sin(alpha * result);
	}
	return result;
}
