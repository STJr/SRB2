/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/

#ifndef _U_LIST_H_
#define _U_LIST_H_

typedef struct listitem_s
{
	struct listitem_s *next;
	struct listitem_s *prev;
} listitem_t;

void ListAdd(void *pItem, listitem_t **itemHead);
void ListAddFront(void *pItem, listitem_t **itemHead);
void ListAddBefore(void *pItem, void *pSpot, listitem_t **itemHead);
void ListAddAfter(void *pItem, void *pSpot, listitem_t **itemHead);
void ListRemove(void *pItem, listitem_t **itemHead);
void ListRemoveAll(listitem_t **itemHead);
void ListRemoveNoFree(void *pItem, listitem_t **itemHead);
unsigned int ListGetCount(void *itemHead);
listitem_t *ListGetByIndex(void *itemHead, unsigned int index);

#endif
