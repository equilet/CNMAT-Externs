// lmin file for Max object
// by Peter Elsea,  University of California, Santa Cruz
// copyright � 2004, Regents of the University of California
// This code and the object created by it may be modified and/or 
// distributed by any means, provided that this copyright and notice   
// appear in all subsequent copies and documentation.

#include "ext.h"
#include <string.h>
#include "ext_common.h"

#define MAXSIZE 256
#define ASSIST_INLET 1  // used in assist method
#define ASSIST_OUTLET 2


typedef struct lmin
{
	struct object ob;
	void *proxy;
	long inletNumber;  // needed for classic
	Atom theList[MAXSIZE];  // stored list
	int listsize;
	Atom outList[MAXSIZE];  // list that will be output
	int outsize;
	Atom inList[MAXSIZE]; 	// copy of input
	int insize;	
	void *out;  // just one outlet for these
} Lmin;


void 	*lmin_new();
void 	lmin_free();
void  	lmin_doLists();
void 	lmin_doBang();
void  	lmin_doInt();
void  	lmin_doFloat();
void 	lmin_doOutput();
void  	lmin_clearList();
void 	lmin_assist();

int 	check_inlet();

void *class;

int main(void)
{
	Symbol * s;
	Symbol * l;
	setup((t_messlist **)&class,(method)lmin_new,(method)lmin_free,(short)sizeof(struct lmin),0L,A_GIMME,0);
	addint((method)lmin_doInt);
	addfloat((method)lmin_doFloat);
	addbang((method)lmin_doBang);
	addmess((method)lmin_doLists, "list",A_GIMME,0);
	addmess((method)lmin_assist, "assist",A_CANT,0);
	addmess((method)lmin_clearList, "clear",0);
	finder_addclass("Lists","Lmin");
	s = gensym("goslugsgo");
	l = gensym("lobjectsbypqe");
	if(s->s_thing != (struct object *)l){
		post("Lobjects for Max � Peter Elsea and Regents of the University of California."); // first Lobject used
	s->s_thing = (struct object *)l;}
	return 0;
}

void lmin_doLists(x,s,argc,argv)
Lmin *x;
Symbol *s;
int argc;
Atom *argv;
{
	int i;
	if(argc > MAXSIZE) argc = MAXSIZE;
	if(check_inlet(x))  // list in right inlet
	{
	for(i=0;i<argc;++i)
		{
		x->theList[i] = argv[i];
		}
	for(;i<MAXSIZE;++i)
		{
		x->theList[i].a_type  = A_LONG;
		x->theList[i].a_w.w_long  = 0;
		}
	x->listsize = argc;
	}
	else		// list in left inlet
		{
	for(i=0;i<argc;++i)
		{
		
		x->inList[i] = argv[i];
		if((x->theList[i].a_type != A_LONG) && (x->theList[i].a_type != A_FLOAT))
			x->outList[i] = argv[i];
		else
		switch(argv[i].a_type) // promote types as necessary
		{
			case A_LONG:
			if(x->theList[i].a_type == A_LONG){
			x->outList[i].a_type = A_LONG;
			x->outList[i].a_w.w_long = argv[i].a_w.w_long < x->theList[i].a_w.w_long ?
				argv[i].a_w.w_long : x->theList[i].a_w.w_long;
			}else{
			x->outList[i].a_type = A_FLOAT;
			x->outList[i].a_w.w_float = (double)argv[i].a_w.w_long < x->theList[i].a_w.w_float ?
				 (double)argv[i].a_w.w_long : x->theList[i].a_w.w_float;
			}
			break;
			
			case A_FLOAT:
			x->outList[i].a_type = A_FLOAT;
			if(x->theList[i].a_type == A_FLOAT)
			
			x->outList[i].a_w.w_float = argv[i].a_w.w_float < x->theList[i].a_w.w_float ?
				argv[i].a_w.w_float : x->theList[i].a_w.w_float;
			else
			x->outList[i].a_w.w_float = argv[i].a_w.w_float < (double)x->theList[i].a_w.w_long ?
				argv[i].a_w.w_float : (double)x->theList[i].a_w.w_long;
			break;
			
			default:
			x->outList[i] = argv[i];
			}
		}
		x->outsize = argc;
		x->insize = argc;
		lmin_doOutput(x);
	}
}


void lmin_doBang(x)  // bang recalculates output based on new input, if any
Lmin *x;

{
		if(x->insize == 0 ) return;
		if(x->insize == 1)
			switch(x->inList[0].a_type){
				case A_LONG:
				lmin_doInt(x,x->inList[0].a_w.w_long);
				break;
				
				case A_FLOAT:
				lmin_doFloat(x,x->inList[0].a_w.w_float);
				break;
			
			}
		else
		lmin_doLists(x,gensym("bang"),x->insize,x->inList);
}

void  lmin_doInt(x,n)
Lmin *x;
long n;
{
	int i;
	
	if(check_inlet(x))		// constant is in right inlet
	{
		
		for(i=0;i<MAXSIZE;++i)
			{
			x->theList[i].a_type = A_LONG;
			x->theList[i].a_w.w_long = n;
			}
		x->listsize = 1;
	}
	else    // constant in left
	{
		for(i=0;i<x->listsize;++i)
			{
				if((x->theList[i].a_type != A_LONG) && (x->theList[i].a_type != A_FLOAT))
					SETLONG(&x->outList[i],n);
				else{
					x->outList[i].a_type = x->theList[i].a_type;
					
					if(x->theList[i].a_type == A_LONG)
						x->outList[i].a_w.w_long = n < x->theList[i].a_w.w_long ? 
														n : x->theList[i].a_w.w_long;
					else
						x->outList[i].a_w.w_float = (double)n < x->theList[i].a_w.w_float ? 
														(double)n : x->theList[i].a_w.w_float;
				}
			}
	x->outsize = x->listsize;
		x->inList[0].a_w.w_long = n;
		x->inList[0].a_type = A_LONG;
		x->insize = 1;
	lmin_doOutput(x);
	}
	
	
}

void  lmin_doFloat(x,n)
Lmin *x;
double n;
{
	int i;
	
	if(check_inlet(x))		// constant is in right inlet
	{
		for(i=0;i<MAXSIZE;++i)
			{
			x->theList[i].a_type = A_FLOAT;
			x->theList[i].a_w.w_float = n;
			}
		x->listsize = 1;
	}
	else
		{
		for(i=0;i<x->listsize;++i){
			if((x->theList[i].a_type != A_LONG) && (x->theList[i].a_type != A_FLOAT))
				SETFLOAT(&x->outList[i],n);
			else{
				x->outList[i].a_type = A_FLOAT;
				if(x->theList[i].a_type == A_LONG)
					x->outList[i].a_w.w_float = n < (double) x->theList[i].a_w.w_long ?
													n : (double) x->theList[i].a_w.w_long;
				else
					x->outList[i].a_w.w_float = n < x->theList[i].a_w.w_float ?
													n : x->theList[i].a_w.w_float;
			}
		}
		x->outsize = x->listsize;
		x->inList[0].a_w.w_float = n;
		x->inList[0].a_type = A_FLOAT;
		x->insize = 1;
		lmin_doOutput(x);
		}
	
	
}
void lmin_doOutput(x)
Lmin *x;
{
	
	if(x->outsize == 1)
		{
		if(x->outList[0].a_type == A_LONG)
			outlet_int(x->out,x->outList[0].a_w.w_long);
			else
			outlet_float(x->out,x->outList[0].a_w.w_float);
		}
	else
		outlet_list(x->out,0L,x->outsize,x->outList);
	
}

void  lmin_clearList(x)
Lmin *x;
{
	int i;
	for(i=0;i<MAXSIZE;++i)
		{
		x->theList[i].a_type = A_LONG;
		x->theList[i].a_w.w_long = 0;
		x->inList[i].a_type = A_LONG;
		x->inList[i].a_w.w_long = 0;
		}
	
	x->listsize = 1;
	x->insize = 1;
	
}

void lmin_assist(x,b,msg,arg,dst)
Lmin *x;
void *b;
long  msg,arg;
char *dst;
{
	
	if(msg == ASSIST_INLET)
	{
		switch (arg)
		{
			case 0:
			strcpy(dst,"Compare to stored list");
			break;
			
			default:
			strcpy(dst,"store this");
			break;
		}
	}
	else
	{
		if(msg == ASSIST_OUTLET)
		switch (arg)
		{
			case 0:
			strcpy(dst,"Minimums from two lists");
			break;
			
			default:
			strcpy(dst,"default");
			break;
		}
	}
	
}



void  *lmin_new(s,ac,theArgs)
Symbol *s;
int ac;
Atom *theArgs;
{

	Lmin  *x;
	int i;
	
	if(ac > MAXSIZE) ac = MAXSIZE;
	x = (Lmin *)newobject(class);
	x->proxy = proxy_new(x,1L,&x->inletNumber);
	x->out= outlet_new(x,0L);
	x->insize = 0;
	for(i=0;i<MAXSIZE;++i)
	{
		x->inList[i].a_type = A_LONG;
		x->theList[i].a_w.w_long = 0;  // no harm default if bang comes before any input
	}
	switch(ac)
	{
		case 0:
		for(i = 0 ;i<MAXSIZE;++i){
			x->theList[i].a_type = A_LONG;
			x->theList[i].a_w.w_long = 0;  // no harm default
		}
		x->listsize = 1;
		break;
		
		case 1:					
		for(i=0;i<MAXSIZE;++i){  // set up to deal with input of any length
			x->theList[i].a_type = theArgs[0].a_type;
			x->theList[i].a_w.w_long = theArgs[0].a_w.w_long;
		}
		x->listsize = 1;
		break;
			
		default:
		for(i=0;i<ac;++i){     // an initialized list
			x->theList[i].a_type = theArgs[i].a_type;
			x->theList[i].a_w.w_long = theArgs[i].a_w.w_long;
		}
		for(;i<MAXSIZE;++i){  // input longer than initialized should not be modified.
			x->theList[i].a_type = A_LONG;
			x->theList[i].a_w.w_long = 0;
		}
	x->listsize = ac;
	}
	
	
	return(x);
}

void lmin_free(x)
Lmin *x;
{	
	freeobject(x->proxy);
}



int check_inlet(x)
Lmin *x;
{
//	return x->inletNumber;  // classic only
	return proxy_getinlet((t_object *)x);  // OS X only
}