/*
 * LinkList.cpp
 *
 *  Created on: 22-May-2009
 *      Author: loyola
 */

#include "LinkList.h"
#include "UaDebug.h"

LinkList::LinkList() {
	head.data = NULL;
	head.next = NULL;
	head.prev = NULL;
	tail = &head;
}

LinkList::~LinkList() {
	while (head.next != NULL) {
		remove(head.next);
	}
}

LinkListNode * LinkList::createNode(void * data) {
	LinkListNode * node = new LinkListNode;

	node->data = data;
	node->next = node->prev = NULL;
	return node;
}

LinkListNode * LinkList::append(void * data) {
	LinkListNode * node = createNode(data);

	tail->next = node;
	node->prev = tail;
	tail = node;
	return node;
}

LinkListNode * LinkList::insertAfter(void * data, LinkListNode * node) {
	UA_ASSERT_NOT_NULL(node);

	LinkListNode * newNode = createNode(data);

	newNode->next = node->next;
	newNode->prev = node->prev;
	node->next = newNode;

	if (node->next != NULL) {
		node->next->prev = newNode;
	}
	return node;
}

LinkListNode * LinkList::insertBeginning(void * data) {
	return insertAfter(data, &head);
}

LinkListNode * LinkList::findNode(void * data) {
	LinkListNode * node = head.next;
	while (node != NULL) {
		if (node->data == data) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

void LinkList::remove(void * data) {
	LinkListNode * node = findNode(data);
	if (node == NULL) {
		UA_WARN("node " << data << " not found");
		return;
	}
	node->next->prev = node->prev;
	node->prev->next = node->next;
	delete node;

}

void LinkList::remove(LinkListNode * node) {
	UA_ASSERT_NOT_NULL(node);
	if (node->next != NULL) {
		node->next->prev = node->prev;
	}
	node->prev->next = node->next;
	delete node;

}

unsigned LinkList::size() {
	unsigned count = 0;
	LinkListNode * node = head.next;
	while (node != NULL) {
		++count;
		node = node->next;
	}
	return count;
}

void * LinkList::getItem(unsigned count) {
	LinkListNode * node = head.next;
	while ((count > 0) && (node != NULL)) {
		--count;
		node = node->next;
	}
	return node->data;
}
