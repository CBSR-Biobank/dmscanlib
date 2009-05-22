#ifndef LINKLIST_H_
#define LINKLIST_H_

/*
 * LinkList.h
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

struct LinkListNode {
	void * data;
	LinkListNode * next;
	LinkListNode * prev;
};

class LinkList {
public:
	LinkList();
	virtual ~LinkList();

	LinkListNode * append(void * data);
	LinkListNode * insertAfter(void * data, LinkListNode * node);
	LinkListNode * insertBeginning(void * data);
	LinkListNode * findNode(void * data);
	void remove(void * data);
	void remove(LinkListNode * node);
	unsigned size();
	void * getItem(unsigned count);

private:
	LinkListNode head;
	LinkListNode * tail;

	LinkListNode * createNode(void * data);
};

#endif /* LINKLIST_H_ */
