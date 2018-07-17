/*====================================================================*/
/* +ListBox with double linked list and selection menu in C.
   +Scroll function added.
   Last modified : 17/7/2018
   Coded by Velorek.
   Target OS: DOS. NO ANSI                                            */
/*====================================================================*/

/*====================================================================*/
/* COMPILER DIRECTIVES AND INCLUDES */
/*====================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*====================================================================*/
/* CONSTANTS */
/*====================================================================*/
/*Scroll Control values.*/
#define SCROLL_ACTIVE 1
#define SCROLL_INACTIVE 0
#define CONTINUE_SCROLL -1
#define DOWN_SCROLL 1
#define UP_SCROLL 0
#define SELECT_ITEM 1
#define UNSELECT_ITEM 0
/* Colors used.   */
#define B_BLACK 0x7
#define B_BLUE 0x1F
#define F_BLACK 0x7
#define F_WHITE 0x7F
#define FH_WHITE 0x7F
/*Keys used.*/
#define K_ENTER 28
#define K_ESCAPE 27
#define K_UP_ARROW 72		/* K_ESCAPE + 'A' -> UP_ARROW*/
#define K_DOWN_ARROW 80	/* K_ESCAPE + 'B' -> DOWN_ARROW*/

/*====================================================================*/
/* TYPEDEF STRUCTS DEFINITIONS */
/*====================================================================*/

typedef struct _listchoice {
  unsigned index;		/* Item number*/
  char   *item;			/* Item string*/
  struct _listchoice *next;	/* Pointer to next item*/
  struct _listchoice *back;	/* Pointer to previous item*/
} LISTCHOICE;

typedef struct _scrolldata {
  unsigned scrollActive;	/*To know whether scroll is active or not.*/
  unsigned scrollLimit;		/*Last index for scroll.*/
  unsigned listLength;		/*Total no. of items in the list */
  unsigned currentListIndex;	/*Pointer to new sublist of items when scrolling.*/
  unsigned displayLimit;	/*No. of elements to be displayed.*/
  unsigned scrollDirection;	/*To keep track of scrolling Direction.*/
  unsigned wherex;
  unsigned wherey;
  unsigned selector;		/*Y++*/
  unsigned backColor0;		/*0 unselected; 1 selected*/
  unsigned foreColor0;
  unsigned backColor1;
  unsigned foreColor1;
  char   *item;
  unsigned itemIndex;
  LISTCHOICE *head;		/*store head of the list*/
} SCROLLDATA;

/*====================================================================*/
/* GLOBAL VARIABLES */
/*====================================================================*/

LISTCHOICE *listBox1 = NULL;	/*Head pointer.*/

/*====================================================================*/
/* PROTOTYPES OF FUNCTIONS                                            */
/*====================================================================*/

/*CONSOLE DISPLAY FUNCTIONS*/
void    writechar(char ch, char color);
void    gotoxy(char x, char y);
void    outputcolor(int foreground, int background, char *str);
char    getch();
void    cls();
/*DYNAMIC LINKED LIST FUNCTIONS*/
void    deleteList(LISTCHOICE ** head);
LISTCHOICE *addend(LISTCHOICE * head, LISTCHOICE * newp);
LISTCHOICE *newelement(char *text);

/*LISTBOX FUNCTIONS*/
void    addItems(LISTCHOICE ** listBox1);
char    listBox(LISTCHOICE * selector, unsigned whereX, unsigned whereY,
		SCROLLDATA * scrollData, unsigned bColor0,
		unsigned fColor0, unsigned bColor1, unsigned fColor1,
		unsigned displayLimit);
void    loadlist(LISTCHOICE * head, SCROLLDATA * scrollData,
		 unsigned indexAt);

void    gotoIndex(LISTCHOICE ** aux, SCROLLDATA * scrollData,
		  unsigned indexAt);
int     query_length(LISTCHOICE ** head);
int     move_selector(LISTCHOICE ** head, SCROLLDATA * scrollData);
char    selectorMenu(LISTCHOICE * aux, SCROLLDATA * scrollData);
void    displayItem(LISTCHOICE * aux, SCROLLDATA * scrollData,
		       int select);

/*====================================================================*/
/* CODE */
/*====================================================================*/

/* ------------------------------ */
/* Terminal manipulation routines */
/* ------------------------------ */

void gotoxy(char x, char y)
/*Sets the cursor at the desired position.*/
{

 /*  printf("%c[%d;%df", 0x1B, y, x);  */
 /*inline assembler or ansi */
 asm {
    mov ah,02h
    mov dh,y
    mov dl,x
    mov bh,0
    int 10h
  }
}
void cls()
{
asm {
  mov ax, 3h
  mov bh, 00h
  mov bl, 0h
  int 10h
}
}
void writechar(char ch, char color)
{
//printf("%d:%c-",ch,ch);
char chaux,coloraux;
/*
Note:
these need to be local variables
so that they can be accessed by
assembler compiler */
chaux=ch;
coloraux=color;
  asm{
    mov ah,02h   //wherex++;
    inc dl
    mov bh,0
    int 10h
    mov ah,9  //print char with cholor
    mov bh,0
    mov al, chaux
    mov bl, coloraux
    mov cx, 1
    int 10h
  }
}
void outputcolor(int foreground, int background, char *str)
/*Changes format foreground and background colors of display.*/
{
  /* printf("%c[%d;%dm", 0x1b, foreground, background); */
  char color;
  char cha,length,i;
  color = background & foreground;
  length = strlen(str);
  for (i=0;i<length;i++){
      cha = str[i];
      writechar(cha,color);
  }
}


/* Read 1 character - no echo */
char getch() {
  char    passkey;
  asm {
     mov ah, 00h
     int 16h
     mov passkey,ah
  }
  return passkey;
}

/* --------------------- */
/* Dynamic List routines */
/* --------------------- */

/* create new list element of type LISTCHOICE from the supplied text string*/
LISTCHOICE *newelement(char *text) {
  LISTCHOICE *newp;
  newp = (LISTCHOICE *) malloc(sizeof(LISTCHOICE));
  newp->item = (char *)malloc(strlen(text) + 1);
  strcpy(newp->item, text);
  newp->next = NULL;
  newp->back = NULL;
  return newp;
}

/* deleleteList: remove list from memory*/
void deleteList(LISTCHOICE ** head) {
  LISTCHOICE *p, *aux;
  aux = *head;
  while(aux->next != NULL) {
    p = aux;
    aux = aux->next;
    free(p->item);
    free(p);			/*remove item*/
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
/* now find the end of list*/
  for(p2 = head; p2->next != NULL; p2 = p2->next) ;
  p2->next = newp;
  newp->back = p2;
  newp->index = newp->back->index + 1;
  return head;
}

/* ---------------- */
/* Listbox routines */
/* ---------------- */

void gotoIndex(LISTCHOICE ** aux, SCROLLDATA * scrollData,
	       unsigned indexAt)
/*Go to a specific location on the list.*/
{
  LISTCHOICE *aux2;
  unsigned counter = 0;
  *aux = listBox1;
  aux2 = *aux;
  while(counter != indexAt) {
    aux2 = aux2->next;
    counter++;
  }
  /*Highlight current item*/

  displayItem(aux2, scrollData, SELECT_ITEM);

  /*Update pointer*/
  *aux = aux2;
}

void loadlist(LISTCHOICE * head, SCROLLDATA * scrollData, unsigned indexAt) {
/*
Displays the items contained in the list with the properties specified
in scrollData.
*/

  LISTCHOICE *aux;
  unsigned wherey, counter = 0;

  aux = head;
  gotoIndex(&aux, scrollData, indexAt);
  /* save values for later*/
  /* wherex = scrollData->wherex; */
  wherey = scrollData->wherey;
  do {
    displayItem(aux, scrollData, UNSELECT_ITEM);
    aux = aux->next;
    counter++;
    scrollData->selector++;  /* y++ */
  } while(counter != scrollData->displayLimit);
  scrollData->selector=wherey; /* reset selector */
}

int query_length(LISTCHOICE ** head) {
/*Return no. items in a list.*/
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

void displayItem(LISTCHOICE * aux, SCROLLDATA * scrollData, int select)
/*Select or unselect item animation*/
{
  switch (select) {

    case SELECT_ITEM:
      gotoxy(scrollData->wherex, scrollData->selector);
      outputcolor(scrollData->foreColor1, scrollData->backColor1,aux->item);
      /*printf("%s\n", aux->item); */
      break;

    case UNSELECT_ITEM:
      gotoxy(scrollData->wherex, scrollData->selector);
      outputcolor(scrollData->foreColor0, scrollData->backColor0,aux->item);
      /*printf("%s\n", aux->item);*/
      break;
  }
}
int move_selector(LISTCHOICE ** selector, SCROLLDATA * scrollData) {
/*
Creates animation by moving a selector highlighting next item and
unselecting previous ite,
*/

  LISTCHOICE *aux;
  unsigned scrollControl = 0, continueScroll = 0;

  /*Auxiliary pointer points to selector.*/
  aux = *selector;

  /*Check if we are within boundaries.*/
  if((aux->next != NULL && scrollData->scrollDirection == DOWN_SCROLL)
     || (aux->back != NULL && scrollData->scrollDirection == UP_SCROLL)) {

    /*Unselect previous item*/
    displayItem(aux, scrollData, UNSELECT_ITEM);

    /*Check whether we move UP or Down*/
    switch (scrollData->scrollDirection) {

      case UP_SCROLL:
	/*Calculate new top index if scroll is active*/
	/*otherwise it defaults to 0 (top)*/
	if(scrollData->scrollActive == SCROLL_ACTIVE)
	  scrollControl = scrollData->currentListIndex;
	else
	  scrollControl = 0;

	/*Move selector*/
	if(aux->back->index >= scrollControl) {
	  scrollData->selector--;	/*whereY--*/
	  aux = aux->back;	/*Go to previous item*/
	} else {
	  if(scrollData->scrollActive == SCROLL_ACTIVE)
	    continueScroll = 1;
	  else
	    continueScroll = 0;
	}
	break;

      case DOWN_SCROLL:
	/*Calculate bottom index limit if scroll is ACTIVE*/
	/*Otherwise it defaults to scrollData->ListLength-1*/

	if(scrollData->scrollActive == SCROLL_ACTIVE)
	  scrollControl =
	      scrollData->currentListIndex + (scrollData->displayLimit -
					      1);
	else
	  scrollControl = scrollData->listLength - 1;

	/*Move selector*/
	if(aux->next->index <= scrollControl) {
	  aux = aux->next;	/*Go to next item*/
	  scrollData->selector++;	/*whereY++;*/
	} else {
	  if(scrollData->scrollActive == SCROLL_ACTIVE)
	    continueScroll = 1;
	  else
	    continueScroll = 0;
	}
	break;
    }

    /*Metrics*/
    gotoxy(6, 5);
    printf("Length:%d|Index:%d|Memory addr:%p",
	   scrollData->listLength, aux->index, aux);
    gotoxy(6, 6);
    printf("Scroll Limit: %d|IsScActive?:%d|ContinueScroll: %d",
	   scrollControl, scrollData->scrollActive, continueScroll);

    /*Highlight new item*/
    displayItem(aux, scrollData, SELECT_ITEM);

    /*Update selector pointer*/
    *selector = aux;
  }
  return continueScroll;
}

char selectorMenu(LISTCHOICE * aux, SCROLLDATA * scrollData) {
  char    ch;
  unsigned control = 0;
  unsigned continueScroll;
  unsigned counter = 0;

  /*Go to and select expected item at the beginning*/

  gotoIndex(&aux, scrollData, scrollData->currentListIndex);

  if(scrollData->scrollDirection == DOWN_SCROLL
     && scrollData->currentListIndex != 0) {
    /*If we are going down we'll select the last item*/
    /*to create a better scrolling transition (animation)*/
    for(counter = 0; counter < scrollData->displayLimit; counter++) {
      scrollData->scrollDirection = DOWN_SCROLL;
      move_selector(&aux, scrollData);
    }

  } else {
    /*Do nothing if we are going up. Selector is always at the top item.*/
  }

  /*It break the loop everytime the boundaries are reached.*/
  /*to reload a new list to show the scroll animation.*/

  while(control != CONTINUE_SCROLL) {
    ch = getch();

    /*if enter key pressed - break loop*/
    if(ch == K_ENTER)
      break;	/*Break the loop*/

    /*Check arrow keys*/
      /* read key again for arrow key combinations*/
      switch (ch) {
	case K_UP_ARROW:	/* escape key + A => arrow key up*/
	  /*Move selector up*/
	  scrollData->scrollDirection = UP_SCROLL;
	  continueScroll = move_selector(&aux, scrollData);
	  /*Break the loop if we are scrolling*/
	  if(scrollData->scrollActive == SCROLL_ACTIVE
	     && continueScroll == 1) {
	    control = CONTINUE_SCROLL;
	    /*Update data*/
	    scrollData->currentListIndex =
		scrollData->currentListIndex - 1;
	    scrollData->selector = scrollData->wherey;
	    scrollData->item = aux->item;
	    scrollData->itemIndex = aux->index;
	    /*Return value*/
	    ch = control;
	  }
	  break;
	case K_DOWN_ARROW:	/* escape key + B => arrow key down
	  /*Move selector down*/
	  scrollData->scrollDirection = DOWN_SCROLL;
	  continueScroll = move_selector(&aux, scrollData);
	  /*Break the loop if we are scrolling*/
	  if(scrollData->scrollActive == SCROLL_ACTIVE
	     && continueScroll == 1) {
	    control = CONTINUE_SCROLL;
	    /*Update data*/
	    scrollData->currentListIndex =
		scrollData->currentListIndex + 1;
	    scrollData->selector = scrollData->wherey;
	    scrollData->item = aux->item;
	    scrollData->itemIndex = aux->index;
	    scrollData->scrollDirection = DOWN_SCROLL;
	    /*Return value*/
	    ch = control;
	  }
	  break;
      }
  }
  if(ch == K_ENTER)		/* enter key*/
  {
    /*Pass data of last item selected.*/
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
  /*unsigned currentIndex = 0;*/
  int     scrollLimit = 0;
  unsigned currentListIndex = 0;
  char    ch;
  LISTCHOICE *aux;

  /* Query size of the list*/
  list_length = query_length(&head) + 1;

  /*Save calculations for SCROLL and store DATA*/
  scrollData->displayLimit = displayLimit;
  scrollLimit = list_length - scrollData->displayLimit;
  /*Careful with negative integers*/

  if(scrollLimit < 0)
    scrollData->displayLimit = list_length;
    /*Failsafe for overboard values*/

  scrollData->scrollLimit = scrollLimit;
  scrollData->listLength = list_length;
  scrollData->wherex = whereX;
  scrollData->wherey = whereY;
  scrollData->selector = whereY;
  scrollData->backColor0 = bColor0;
  scrollData->backColor1 = bColor1;
  scrollData->foreColor0 = fColor0;
  scrollData->foreColor1 = fColor1;

  /*Check whether we have to activate scroll or not*/
  /*and if we are within bounds. [1,list_length)*/

  if(list_length > scrollData->displayLimit && scrollLimit > 0
     && displayLimit > 0) {
    /*Scroll is possible*/

    scrollData->scrollActive = SCROLL_ACTIVE;
    aux = head;

    currentListIndex = 0;	/*We listBox1 the scroll at the top index.*/
    scrollData->currentListIndex = currentListIndex;

    /*Scroll loop animation. Finish with ENTER.*/
    do {
      currentListIndex = scrollData->currentListIndex;
      loadlist(aux, scrollData, currentListIndex);
      gotoIndex(&aux, scrollData, currentListIndex);
      gotoxy(6, 4);
      printf("Current List Index: %d:%d\n", scrollData->currentListIndex,
	     aux->index);
      ch = selectorMenu(aux, scrollData);
    } while(ch != K_ENTER);

  } else {
    /*Scroll is not possible.*/
    /*Display all the elements and create selector.*/
    scrollData->scrollActive = SCROLL_INACTIVE;
    scrollData->currentListIndex = 0;
    scrollData->displayLimit = list_length;	/*Default to list_length*/
    loadlist(head, scrollData, 0);
    ch = selectorMenu(head, scrollData);
  }
  return ch;
}

void addItems(LISTCHOICE ** listBox1) {
/*Load items into the list. */
  if(*listBox1 != NULL)
    deleteList(listBox1);
  *listBox1 = addend(*listBox1, newelement("Option 1"));
  *listBox1 = addend(*listBox1, newelement("Option 2"));
  *listBox1 = addend(*listBox1, newelement("Option 3"));
  *listBox1 = addend(*listBox1, newelement("Option 4"));
  *listBox1 = addend(*listBox1, newelement("Option 5"));
  *listBox1 = addend(*listBox1, newelement("Option 6"));
  *listBox1 = addend(*listBox1, newelement("Option 7"));
  *listBox1 = addend(*listBox1, newelement("Option 8"));
}

int main() {
  SCROLLDATA scrollData;
  char    ch;

  cls();
  addItems(&listBox1);

  /*========================================================================*/
  /*
     ListBox with Scroll:
     ____________________

     Usage:
     listBox(headpointer, whereX, whereY, scrollData, backColor0, foreColor0,
     backcolor1, forecolor1, displayLimit);
   */
   /*=======================================================================*/

  ch = listBox(listBox1, 10, 8, &scrollData, B_BLACK, F_WHITE, B_BLUE,
	       FH_WHITE, 3);

  /*Item selected. */
  gotoxy(1, 14);
  //outputcolor(FH_WHITE, B_BLUE, "Hello");
  printf("Item selected: %s | Index: %d | Key : %d\n", scrollData.item,
	 scrollData.itemIndex, ch);

  /*Free memory and restore colors. */
  deleteList(&listBox1);
  return 0;
}
