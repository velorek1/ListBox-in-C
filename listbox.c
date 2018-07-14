/*
===============================================================   
   +ListBox with double linked list and selection menu in C.
   +Scroll function added.
   Last modified : 14/7/2018
   Coded by Velorek.
   Target OS: Linux.
===============================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* CONSTANTS */

//scrollControl values
#define SCROLL_ACTIVE 1
#define SCROLL_INACTIVE 0
#define CONTINUE_SCROLL -1
#define DOWN_SCROLL 1
#define UP_SCROLL 0

// Colors used.                                                                         
#define B_BLACK 40
#define B_BLUE 44
#define F_BLACK 30
#define F_WHITE 37
#define FH_WHITE 97

//KEYS USED

#define K_ENTER 10
#define K_ESCAPE 27
#define K_UP_ARROW 'A'		// K_ESCAPE + 'A' -> UP_ARROW
#define K_DOWN_ARROW 'B'	// K_ESCAPE + 'B' -> DOWN_ARROW

typedef struct _listchoice {
  unsigned index;		// Item number
  char   *item;			// Item string
  struct _listchoice *next;	// Pointer to next item
  struct _listchoice *back;	// Pointer to previous item
} LISTCHOICE;

typedef struct _scrolldata {
  unsigned scrollActive;	//To know whether scroll is active or not.
  unsigned scrollLimit;		//Last index for scroll.
  unsigned listLength;		//Total no. of items in the list
  unsigned currentListIndex;	//Pointer to new sublist of items when scrolling.
  unsigned displayLimit;	//No. of elements to be displayed.
  unsigned scrollDirection;	//To keep track of scrolling Direction.
  unsigned wherex;
  unsigned wherey;
  unsigned selector;		//Y++
  unsigned backColor0;		//0 unselected; 1 selected
  unsigned foreColor0;
  unsigned backColor1;
  unsigned foreColor1;
  char   *item;
  unsigned itemIndex;
  LISTCHOICE *head;		//store head of the list
} SCROLLDATA;

static struct termios old, new;

LISTCHOICE *listBox1 = NULL;	//Head pointer.

/* PROTOTYPES */

//CONSOLE DISPLAY FUNCTIONS 

void    gotoxy(int x, int y);
void    outputcolor(int foreground, int background);
void    initTermios(int echo);
void    resetTermios(void);
char    getch();

//LIST FUNCTIONS

void    deleteList(LISTCHOICE ** head);
LISTCHOICE *addend(LISTCHOICE * head, LISTCHOICE * newp);
LISTCHOICE *newelement(char *text);
LISTCHOICE *addfront(LISTCHOICE * head, LISTCHOICE * newp);
void    addItems(LISTCHOICE ** listBox1);
char    listBox(LISTCHOICE * head, unsigned whereX, unsigned whereY,
		SCROLLDATA * scrollData, unsigned bColor0,
		unsigned fColor0, unsigned bColor1, unsigned fColor1,
		unsigned displayLimit);
void    loadlist(LISTCHOICE * head, SCROLLDATA * scrollData,
		 unsigned indexAt);

void    gotoIndex(LISTCHOICE ** aux, SCROLLDATA * scrollData,
		  unsigned indexAt);

int     query_length(LISTCHOICE ** head);
int     move_up(LISTCHOICE ** head, SCROLLDATA * scrollData);
int     move_down(LISTCHOICE ** head, SCROLLDATA * scrollData);

char    selectorMenu(LISTCHOICE * aux, SCROLLDATA * scrollData);

  /* Terminal manipulation routines */

void gotoxy(int x, int y)
//Sets the cursor at the desired position.
{
  printf("%c[%d;%df", 0x1B, y, x);
}

void outputcolor(int foreground, int background)
//Changes format foreground and background colors of display.
{
  printf("%c[%d;%dm", 0x1b, foreground, background);
}

/* Initialize new terminal i/o settings */
void initTermios(int echo) {
  tcgetattr(0, &old);		/* grab old terminal i/o settings */
  new = old;			/* make new settings same as old settings */
  new.c_lflag &= ~ICANON;	/* disable buffered i/o */
  new.c_lflag &= echo ? ECHO : ~ECHO;	/* set echo mode */
  tcsetattr(0, TCSANOW, &new);	/* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) {
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - no echo */
char getch() {
  char    ch;
  initTermios(0);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Dynamic List routines */

// create new list element of type LISTCHOICE from the supplied text string
LISTCHOICE *newelement(char *text) {
  LISTCHOICE *newp;
  newp = (LISTCHOICE *) malloc(sizeof(LISTCHOICE));
  newp->item = (char *)malloc(strlen(text) + 1);
  strcpy(newp->item, text);
  newp->next = NULL;
  newp->back = NULL;
  return newp;
}

// deleleteList: remove list from memory
void deleteList(LISTCHOICE ** head) {
  LISTCHOICE *p, *aux;
  aux = *head;
  while(aux->next != NULL) {
    p = aux;
    aux = aux->next;
    free(p->item);
    free(p);			//remove item
  }
}

/* addend: add new LISTCHOICE to the end of a list  */
/* usage example: listBox1 = (addend(listBox1, newelement("Item")); */
LISTCHOICE *addend(LISTCHOICE * head, LISTCHOICE * newp) {
  LISTCHOICE *p2;
  if(head == NULL) {
    newp->index = 0;
    newp->back = NULL;
    return newp;
  }
// now find the end of list
  for(p2 = head; p2->next != NULL; p2 = p2->next) ;
  p2->next = newp;
  newp->back = p2;
  newp->index = newp->back->index + 1;
  return head;
}

void gotoIndex(LISTCHOICE ** aux, SCROLLDATA * scrollData,
	       unsigned indexAt)
//Go to a specific location on the list.
{
  LISTCHOICE *aux2;
  unsigned counter = 0;
  *aux = listBox1;
  aux2 = *aux;
  while(counter != indexAt) {
    aux2 = aux2->next;
    counter++;
  }
  //Highlight current item
  gotoxy(scrollData->wherex, scrollData->selector);
  outputcolor(scrollData->foreColor1, scrollData->backColor1);
  printf("%s\n", aux2->item);
  *aux = aux2;
}

void loadlist(LISTCHOICE * head, SCROLLDATA * scrollData, unsigned indexAt) {
/*
Displays the items contained in the list with the properties specified
in scrollData.
*/

  LISTCHOICE *aux;
  unsigned wherex, wherey, counter = 0;

  aux = head;
  gotoIndex(&aux, scrollData, indexAt);
  wherex = scrollData->wherex;
  wherey = scrollData->wherey;
  do {
    gotoxy(wherex, wherey);
    outputcolor(scrollData->foreColor0, scrollData->backColor0);
    printf("%s\n", aux->item);
    aux = aux->next;
    counter++;
    wherey++;
  } while(counter != scrollData->displayLimit);
}

int query_length(LISTCHOICE ** head) {
//Measure no. items in a list.
  {
    LISTCHOICE *aux;

    unsigned itemCount = 0;
    aux = *head;
    while(aux->next != NULL) {
      aux = aux->next;
      itemCount++;
    }
    return itemCount;
  }

}

int move_down(LISTCHOICE ** head, SCROLLDATA * scrollData) {
  LISTCHOICE *aux;
  unsigned scrollControl = 0, continueScroll = 0;

  //Check if we are at the end of the list
  aux = *head;

  if(aux->next != NULL) {

    //Unselect previous item
    gotoxy(scrollData->wherex, scrollData->selector);
    outputcolor(scrollData->foreColor0, scrollData->backColor0);
    printf("%s\n", aux->item);

    //Calculate bottom index limit if scroll is ACTIVE
    //Otherwise it defaults to scrollData->ListLength-1

    if(scrollData->scrollActive == SCROLL_ACTIVE)
      scrollControl =
	  scrollData->currentListIndex + (scrollData->displayLimit - 1);
    else
      scrollControl = scrollData->listLength - 1;

    //Move selector
    if(aux->next->index <= scrollControl) {
      aux = aux->next;		//Go to next item
      scrollData->selector++;	//whereY++;
    } else {
      if(scrollData->scrollActive == SCROLL_ACTIVE) {
	continueScroll = 1;
      } else {
	continueScroll = 0;
      }
    }

    //Metrics
    gotoxy(6, 5);
    printf("Length:%d|Index:%d|Memory addr:%p",
	   scrollData->listLength, aux->index, aux);
    gotoxy(6, 6);
    printf("Scroll Limit: %d|IsScActive?:%d|ContinueScroll: %d",
	   scrollControl, scrollData->scrollActive, continueScroll);

    //Highlight current item
    gotoxy(scrollData->wherex, scrollData->selector);
    outputcolor(scrollData->foreColor1, scrollData->backColor1);
    printf("%s\n", aux->item);

    *head = aux;
  }
  return continueScroll;
}

int move_up(LISTCHOICE ** head, SCROLLDATA * scrollData) {
  LISTCHOICE *aux;
  unsigned scrollControl = 0, continueScroll = 0;

  //Check if we are at the beginning of the list.
  aux = *head;
  if(aux->back != NULL) {

    //Unselect previous item
    gotoxy(scrollData->wherex, scrollData->selector);
    outputcolor(scrollData->foreColor0, scrollData->backColor0);
    printf("%s\n", aux->item);

    //Calculate new top index if scroll is active 
    //otherwise it defaults to 0 (top)
    if(scrollData->scrollActive == SCROLL_ACTIVE)
      scrollControl = scrollData->currentListIndex;
    else
      scrollControl = 0;

    //Move selector
    if(aux->back->index >= scrollControl) {
      scrollData->selector--;	//whereY--
      aux = aux->back;		//Go to previous item
    } else {
      if(scrollData->scrollActive == SCROLL_ACTIVE) {
	continueScroll = 1;
      } else {
	continueScroll = 0;
      }
    }

    //Metrics
    gotoxy(6, 5);
    printf("Length:%d|Index:%d|Memory addr:%p",
	   scrollData->listLength, aux->index, aux);
    gotoxy(6, 6);
    printf("Scroll Limit: %d|IsScActive?:%d|ContinueScroll: %d",
	   scrollControl, scrollData->scrollActive, continueScroll);

    //Highlight current item
    gotoxy(scrollData->wherex, scrollData->selector);
    outputcolor(scrollData->foreColor1, scrollData->backColor1);
    printf("%s\n", aux->item);

    *head = aux;
  }
  return continueScroll;
}

char selectorMenu(LISTCHOICE * aux, SCROLLDATA * scrollData) {
  char    ch;
  unsigned control = 0;
  unsigned continueScroll;
  unsigned counter = 0;
  //Go to and select expected item at the beginning

  gotoIndex(&aux, scrollData, scrollData->currentListIndex);

  if(scrollData->scrollDirection == DOWN_SCROLL
     && scrollData->currentListIndex != 0) {
    //If we are going down we'll select the last item 
    //to create a better scrolling transition (animation)
    for(counter = 0; counter < scrollData->displayLimit; counter++)
      move_down(&aux, scrollData);

  } else {
    //Do nothing.
  }
  while(control != CONTINUE_SCROLL)	// enter key
  {
    ch = getch();
    if(ch == 10)
      control = CONTINUE_SCROLL;
    if(ch == K_ESCAPE)		// escape key
    {
      getch();			// read key again for arrow key combinations
      switch (getch()) {
	case K_UP_ARROW:	// escape key + A => arrow key up
	  continueScroll = move_up(&aux, scrollData);
	  if(scrollData->scrollActive == SCROLL_ACTIVE
	     && continueScroll == 1) {
	    control = CONTINUE_SCROLL;
	    scrollData->currentListIndex =
		scrollData->currentListIndex - 1;
	    scrollData->selector = scrollData->wherey;
	    scrollData->item = aux->item;
	    scrollData->itemIndex = aux->index;
	    scrollData->scrollDirection = UP_SCROLL;
	    ch = control;
	  }
	  break;
	case K_DOWN_ARROW:	// escape key + B => arrow key down      
	  continueScroll = move_down(&aux, scrollData);
	  if(scrollData->scrollActive == SCROLL_ACTIVE
	     && continueScroll == 1) {
	    control = CONTINUE_SCROLL;
	    scrollData->currentListIndex =
		scrollData->currentListIndex + 1;
	    scrollData->selector = scrollData->wherey;
	    scrollData->item = aux->item;
	    scrollData->itemIndex = aux->index;
	    scrollData->scrollDirection = DOWN_SCROLL;
	    ch = control;
	  }
	  break;
      }
    }
  }
  if(ch == K_ENTER)		// enter key
  {
    //Pass data of last item selected.
    scrollData->item = aux->item;
    scrollData->itemIndex = aux->index;
  }
  return ch;
}

char listBox(LISTCHOICE * head,
	     unsigned whereX, unsigned whereY, SCROLLDATA * scrollData,
	     unsigned bColor0, unsigned fColor0, unsigned bColor1,
	     unsigned fColor1, unsigned displayLimit) {

  unsigned list_length = 0;
  //unsigned currentIndex = 0;
  int     scrollLimit = 0;
  unsigned currentListIndex = 0;
  char    ch;
  LISTCHOICE *aux;

  // Query size of the list
  list_length = query_length(&head) + 1;

  //Save calculations for SCROLL and store DATA
  scrollData->displayLimit = displayLimit;
  scrollLimit = list_length - scrollData->displayLimit;	//Careful with negative integers

  if(scrollLimit < 0)
    scrollData->displayLimit = list_length;	//Failsafe for overboard values

  scrollData->scrollLimit = scrollLimit;
  scrollData->listLength = list_length;
  scrollData->wherex = whereX;
  scrollData->wherey = whereY;
  scrollData->selector = whereY;
  scrollData->backColor0 = bColor0;
  scrollData->backColor1 = bColor1;
  scrollData->foreColor0 = fColor0;
  scrollData->foreColor1 = fColor1;

  //Check whether we have to activate scroll or not
  if(list_length > scrollData->displayLimit && scrollLimit > 0) {
    //Scroll is possible  

    scrollData->scrollActive = SCROLL_ACTIVE;
    aux = head;

    currentListIndex = 0;	//We listBox1 the scroll at the top index.
    scrollData->currentListIndex = currentListIndex;

    do {
      //Scroll loop
      currentListIndex = scrollData->currentListIndex;
      loadlist(aux, scrollData, currentListIndex);
      gotoIndex(&aux, scrollData, currentListIndex);
      gotoxy(6, 4);
      printf("Current List Index: %d:%d\n", scrollData->currentListIndex,
	     aux->index);

      ch = selectorMenu(aux, scrollData);
    } while(ch != 10);

  } else {
    //Scroll is not possible
    scrollData->scrollActive = SCROLL_INACTIVE;
    scrollData->currentListIndex = 0;
    loadlist(head, scrollData, 0);
    ch = selectorMenu(head, scrollData);
  }
  return ch;
}

void addItems(LISTCHOICE ** listBox1) {
  if(*listBox1 != NULL)
    deleteList(listBox1);
  *listBox1 = addend(*listBox1, newelement("Item 1"));
  *listBox1 = addend(*listBox1, newelement("Item 2"));
  *listBox1 = addend(*listBox1, newelement("Item 3"));
  *listBox1 = addend(*listBox1, newelement("Item 4"));
  *listBox1 = addend(*listBox1, newelement("Item 5"));
  *listBox1 = addend(*listBox1, newelement("Item 6"));
  *listBox1 = addend(*listBox1, newelement("Item 7"));
  *listBox1 = addend(*listBox1, newelement("Item 8"));
}

int main() {
  SCROLLDATA scrollData;
  char    ch;

  system("clear");
  addItems(&listBox1);

  /* 

     ListBox with Scroll: 
     ____________________

     Usage:
     listBox(headpointer, whereX, whereY, scrollData, backColor0, foreColor0,
     backcolor1, forecolor1, displayLimit);

   */

  ch = listBox(listBox1, 10, 8, &scrollData, B_BLACK, F_WHITE, B_BLUE,
	       FH_WHITE, 3);

  gotoxy(1, 14);
  outputcolor(FH_WHITE, B_BLUE);
  printf("Item selected: %s | Index: %d | Key : %d\n", scrollData.item,
	 scrollData.itemIndex, ch);
  deleteList(&listBox1);
  outputcolor(F_WHITE, B_BLACK);
  printf("\n");
}
