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


extern struct activeJoinNode *activeNodeHead;
extern struct activeJoinNode *activeNodeTail;
extern CRITICAL_SECTION g_cs;
extern HANDLE g_hSemaphoreBufferEmpty, g_hSemaphoreBufferFull, g_hSemaphoreBuffer;
extern HANDLE g_debug;

struct activeJoinNode* GetBestOneActiveNode();

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

	while ((currentActiveNode = GetBestOneActiveNode()) != NULL)
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
globle struct activeJoinNode* GetBestOneActiveNode()
{
	struct activeJoinNode *rtnNode = NULL;
	struct activeJoinNode *pNode = NULL;
#if THREAD
	//WaitForSingleObject(g_hSemaphoreBufferFull, INFINITE);
	WaitForSingleObject(g_hSemaphoreBuffer, INFINITE);
	EnterCriticalSection(&g_cs);
#endif
	if (activeNodeHead->next == NULL)return NULL;
	pNode = activeNodeHead->next;
	rtnNode = pNode;
	
	while(pNode != NULL)
	{
		if (pNode->currentJoinNode->nodeMaxSalience > rtnNode->currentJoinNode->nodeMaxSalience)
		{
			rtnNode = pNode;
		}
		pNode = pNode->next;
	}
	rtnNode->pre->next = rtnNode->next;
	if(rtnNode->next != NULL)rtnNode->next->pre = rtnNode->pre;
	if (rtnNode == activeNodeTail) activeNodeTail = rtnNode->pre;
	rtnNode->next = NULL;
	rtnNode->pre = NULL;
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
	theFact->whichDeftemplate = EnvFindDeftemplate(theEnv, "debug1");
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
#if THREAD
	//WaitForSingleObject(g_hSemaphoreBufferEmpty, INFINITE);
	EnterCriticalSection(&g_cs);
#endif
	if (activeNodeHead->next == NULL){ activeNodeHead->next = oneNode; oneNode->pre = activeNodeHead; }
	else
	{
		activeNodeTail->next = oneNode;
		oneNode->pre = activeNodeTail;
	}
	activeNodeTail = oneNode;
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

#if THREAD
	//WaitForSingleObject(g_hSemaphoreBufferEmpty, INFINITE);
	EnterCriticalSection(&g_cs);
#endif

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

	while ((currentActiveNode = GetBestOneActiveNode()) != NULL)
	{
		currentJoinNode = currentActiveNode->currentJoinNode;
		currentPartialMatch = currentActiveNode->currentPartialMatch;
		lhsBinds = currentActiveNode->lhsBinds;
		rhsBinds = currentActiveNode->rhsBinds;
		hashValue = currentActiveNode->hashValue;
		enterDirection = currentActiveNode->curPMOnWhichSide;
		theFact = (struct fact *) currentActiveNode->theEntity;
		theMarks = currentActiveNode->markers;
		theHeader = currentActiveNode->theHeader;

		//WaitForSingleObject(g_debug, INFINITE);
		//EnvRun(theEnv, -1);
		if (currentJoinNode->firstJoin)
		{
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
	return 0;
}
globle double SlowDown()
{
	//Sleep(3000);
	int slow = 1000000;
	int alpha = 123;
	double result = 1;
	while (slow-- > 0){
		result = sin(alpha * result);
	}
	return result;
}
