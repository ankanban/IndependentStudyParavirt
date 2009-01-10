/** @file test.c
 *
 *  Simple test variable queue macro package 
 *  
 *  Simple program that stress-tests the functionality of the c preprocessor
 *  queue macro set. If it executes properly, the expected output to the
 *  kernel debug console is:
 *  5 -> 
 *  4 -> 
 *  3 -> 
 *  2 -> 
 *  1 -> 
 *  End
 *
 *  1 <- 
 *  2 <- 
 *  3 <- 
 *  4 <- 
 *  5 <- 
 *  End
 *
 *  5 -> 
 *  4 -> 
 *  3 -> 
 *  2 -> 
 *  1 -> 
 *  1 -> 
 *  2 -> 
 *  3 -> 
 *  4 -> 
 *  5 -> 
 *  End
 *
 *  5 <- 
 *  4 <- 
 *  3 <- 
 *  2 <- 
 *  1 <- 
 *  1 <- 
 *  2 <- 
 *  3 <- 
 *  4 <- 
 *  5 <- 
 *  End
 *
 *  5 -> 
 *  4 -> 
 *  3 -> 
 *  2 -> 
 *  1 -> 
 *  1 -> 
 *  3 -> 
 *  4 -> 
 *  5 -> 
 *  End
 *
 *  5 <- 
 *  4 <- 
 *  3 <- 
 *  1 <- 
 *  1 <- 
 *  2 <- 
 *  3 <- 
 *  4 <- 
 *  5 <- 
 *  End
 *
 *  -1 -> 
 *  5 -> 
 *  4 -> 
 *  3 -> 
 *  2 -> 
 *  1 -> 
 *  1 -> 
 *  3 -> 
 *  4 -> 
 *  5 -> 
 *  End
 *
 *  5 <- 
 *  4 <- 
 *  3 <- 
 *  1 <- 
 *  1 <- 
 *  2 <- 
 *  3 <- 
 *  4 <- 
 *  5 <- 
 *  -1 <- 
 *  End
 *
 *  -1 -> 
 *  5 -> 
 *  4 -> 
 *  3 -> 
 *  2 -> 
 *  1 -> 
 *  1 -> 
 *  3 -> 
 *  4 -> 
 *  5 -> 
 *  -2 -> 
 *  End
 *  
 *  -2 <- 
 *  5 <- 
 *  4 <- 
 *  3 <- 
 *  1 <- 
 *  1 <- 
 *  2 <- 
 *  3 <- 
 *  4 <- 
 *  5 <- 
 *  -1 <- 
 *  End
 *  
 *  -1 -> 
 *  5 -> 
 *  4 -> 
 *  3 -> 
 *  2 -> 
 *  1 -> 
 *  1 -> 
 *  3 -> 
 *  4 -> 
 *  5 -> 
 *  End
 *  
 *  5 <- 
 *  4 <- 
 *  3 <- 
 *  1 <- 
 *  1 <- 
 *  2 <- 
 *  3 <- 
 *  4 <- 
 *  5 <- 
 *  -1 <- 
 *  End
 *  
 *  5 -> 
 *  4 -> 
 *  3 -> 
 *  2 -> 
 *  1 -> 
 *  1 -> 
 *  3 -> 
 *  4 -> 
 *  5 -> 
 *  End
 *  
 *  5 <- 
 *  4 <- 
 *  3 <- 
 *  1 <- 
 *  1 <- 
 *  2 <- 
 *  3 <- 
 *  4 <- 
 *  5 <- 
 *  End
 *  
 *
 *  You may wish to look through this source to get a feel for how the
 *  variable queue macro package works in practice.
 *
 *  @author Mark T. Tomczak (mtomczak)
 **/

#include <variable_queue.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <thread.h>

/** @brief number of elements in the queue */
#define NUM_ELEMENTS 5

/** @brief thread library stack size */
#define STACK_SIZE (1<<10)

Q_NEW_HEAD(int_list_head_t, int_list_t);

/** @brief a simple list of integers */
typedef struct int_list_t{
  Q_NEW_LINK(int_list_t) myLink;
  int data;
} int_list_t;


void output_list(int_list_head_t *myHead)
{
  int_list_t *celem;
  Q_FOREACH(celem,myHead,myLink)
  {
    lprintf_kern("%d -> ",celem->data);
  }
  lprintf_kern("End\n");
  
  for(celem=Q_GET_TAIL(myHead);celem != NULL; celem = Q_GET_PREV(celem,myLink))
  {
    lprintf_kern("%d <- ", celem->data);
  }
  lprintf_kern("End\n");
}

int_list_t *new_int_list(int newData)
{
  int_list_t *nl;
  
  nl = (int_list_t *)malloc(sizeof(int_list_t));
  Q_INIT_ELEM(nl,myLink);
  nl->data=newData;
  
  return nl;
}

int main(int argc, char **argv)
{
  int i;
  int_list_head_t *head;  
  int_list_t *curList;
  int_list_t *insertPos;

  thr_init(STACK_SIZE);
  
  head=(int_list_head_t *)malloc(sizeof(int_list_head_t));
  Q_INIT_HEAD(head);
  
  for(i=1; i<=NUM_ELEMENTS; i++)
  {
    curList = new_int_list(i);
    Q_INSERT_FRONT(head,curList,myLink);
  }
  
  output_list(head); /** should be 5 -> 4 -> ... 2 -> 1 -> End **/
  
  for(i=1;i<=NUM_ELEMENTS;i++)
  {
    curList = new_int_list(i);
    Q_INSERT_TAIL(head,curList,myLink);
  }
  
  output_list(head); /** should be 5 -> 4 -> ... 2 -> 1 -> 1 -> 2 ... 5 -> End*/
  
  curList = Q_GET_FRONT(head);
  for(i=0; i<=NUM_ELEMENTS; i++)
  {
    curList = Q_GET_NEXT(curList,myLink);
  }
  
  Q_REMOVE(head,curList,myLink);
  
  output_list(head); /** should be 5 -> ... 1 -> 1 -> 3 -> ... 5 -> End */
  
  insertPos=Q_GET_FRONT(head);

  curList = new_int_list(-1);

  Q_INSERT_BEFORE(head,insertPos,curList,myLink);

  output_list(head); /* should be 5 -> -1 -> ... 1 -> 1 -> 3 -> ... 5 -> End */
  
  insertPos = Q_GET_TAIL(head);
    
  curList = new_int_list(-2);
  
  Q_INSERT_AFTER(head,insertPos,curList,myLink);
  
  output_list(head);
  
  Q_REMOVE(head,curList,myLink);
  
  output_list(head);
  
  curList = Q_GET_FRONT(head);
  
  Q_REMOVE(head,curList,myLink);
  
  output_list(head);
  
  return 0;
}
