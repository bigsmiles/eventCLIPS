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
#if THREAD

struct JoinNodeList *joinNodeListHead;
struct JoinNodeList *joinNodeListTail;
extern struct activeJoinNode *activeNodeHead;
extern struct activeJoinNode *activeNodeTail;
extern CRITICAL_SECTION g_cs;
extern HANDLE  g_hSemaphoreBuffer, g_hSemaphoreBufferOfThread1, g_hSemaphoreBufferOfThread2;


struct activeJoinNode* GetBestOneActiveNode(void *theEnv,int id);
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

	while ((currentActiveNode = GetBestOneActiveNode(theEnv,0)) != NULL)
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
globle struct activeJoinNode* GetBestOneActiveNode(void *theEnv,int threadID)
{
	struct activeJoinNode *rtnNode = NULL;
	struct activeJoinNode *pNode = NULL;
	struct JoinNodeList *oneListNode = NULL;
#if THREAD
	
#if !MUTILTHREAD
	WaitForSingleObject(g_hSemaphoreBuffer, INFINITE);
#else if
	if(rtnNode == NULL){
	if (threadID == 0){
		WaitForSingleObject(g_hSemaphoreBufferOfThread1, INFINITE);
		}
	else{
		WaitForSingleObject(g_hSemaphoreBufferOfThread2, INFINITE);
		}
	}
#endif
	EnterCriticalSection(&g_cs);
#endif
#if SPEEDUP

	if(joinNodeListHead->next == NULL)return NULL;
	oneListNode = joinNodeListHead->next;
	long long MinTime = 0x3fffffffffffffff;
	long long flag = 100000;
	struct joinNode *tmp = NULL;
	int depth = -1;
	while(oneListNode != NULL){
		
		if (oneListNode->join->numOfActiveNode > 0 ){
#if MUTILTHREAD
			if ((oneListNode->join->nodeMaxSalience % 2) == threadID)
#endif
			{

				long long curTime = (oneListNode->join->nodeMinSalience * flag);// +oneListNode->join->activeJoinNodeListHead->next->timeTag;
				//if(curTime < MinTime ||(curTime == MinTime && oneListNode->join->depth > depth)) //deadline相同的时候，越接近出口的越优先
				if ((int)oneListNode->join->depth > depth)
				{
					rtnNode = oneListNode->join->activeJoinNodeListHead->next;
					tmp = oneListNode->join;
					MinTime = curTime;
					depth = tmp->depth;
					//cnt += 1;
				}
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
		
		time = DOToLong(timeVal);
		printf("arg1 is integer: %lld\n", time);
	}
	
#if THREAD
	
	EnterCriticalSection(&g_cs);
#endif
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
#if !MUTILTHREAD
	ReleaseSemaphore(g_hSemaphoreBuffer, 1, NULL);
#else if
	if (curNode->threadTag == 0){
		ReleaseSemaphore(g_hSemaphoreBufferOfThread1, 1, NULL);
	}
	else{
		ReleaseSemaphore(g_hSemaphoreBufferOfThread2, 1, NULL);
	}
#endif

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

#if THREAD
	
	EnterCriticalSection(&g_cs);
#endif
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
#if !MUTILTHREAD 1
	ReleaseSemaphore(g_hSemaphoreBuffer, 1, NULL);
#else if
	if (curNode->threadTag == 0){
		ReleaseSemaphore(g_hSemaphoreBufferOfThread1, 1, NULL);
	}
	else{
		ReleaseSemaphore(g_hSemaphoreBufferOfThread2, 1, NULL);
	}
#endif

#endif

	return;
}
unsigned int __stdcall MoveOnJoinNetworkThread(void *pM)
{
#if THREAD
	void *theEnv = ((struct ThreadNode*)pM)->theEnv;
	int threadID = ((struct ThreadNode*)pM)->threadTag;
#endif
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
		currentActiveNode = GetBestOneActiveNode(theEnv,threadID);
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
#if THREAD 1
			struct factNotOnJoinNode *p = theFact->factNotOnNode;
			while (p->next != NULL){
				if (p->next->join == currentJoinNode){
					p->next = p->next->next;
					break;
				}
				p = p->next;
			}
#endif
		    ((struct patternMatch *)theFact->list)->theMatch = currentPartialMatch;
			NetworkAssertRight(theEnv, currentPartialMatch, currentJoinNode);
		}
		else {/*error*/}
	}

	return 0;
}
#endif // THREAD
globle double SlowDown()
{
	//Sleep(3000);
	int slow = 8000;
	int alpha = 123;
	double result = 1;
	while (slow-- > 0){
		result = sin(alpha * result);
	}
	return result;
}
