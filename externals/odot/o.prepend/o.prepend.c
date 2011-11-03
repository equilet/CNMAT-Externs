/*
Written by John MacCallum, The Center for New Music and Audio Technologies,
University of California, Berkeley.  Copyright (c) 2009, The Regents of
the University of California (Regents). 
Permission to use, copy, modify, distribute, and distribute modified versions
of this software and its documentation without fee and without a signed
licensing agreement, is hereby granted, provided that the above copyright
notice, this paragraph and the following two paragraphs appear in all copies,
modifications, and distributions.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
NAME: o.prepend
DESCRIPTION: Prepend a string to the addresses in a bundle
AUTHORS: John MacCallum
COPYRIGHT_YEARS: 2009
SVN_REVISION: $LastChangedRevision: 587 $
VERSION 0.0: First try
version 1.0: Rewritten to only take one argument (the symbol to be prepended) which can be overridden by a symbol at the beginning of a mesage
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
*/

#include "version.h"
#include "ext.h"
#include "version.c"
#include "ext_obex.h"
#include "ext_obex_util.h"
#include "osc.h"
#include "osc_bundle_s.h"
#include "osc_bundle_iterator_s.h"
#include "omax_util.h"

typedef struct _oppnd{
	t_object ob;
	void *outlet;
	t_symbol *sym_to_prepend;
	int sym_to_prepend_len;
	char *buffer;
	int bufferLen;
	int bufferPos;
} t_oppnd;

struct context{
	char *buffer;
	int bufferLen;
	int bufferPos;
	t_symbol *sym_to_prepend;
	int sym_to_prepend_len;
};

void *oppnd_class;

void oppnd_fullPacket(t_oppnd *x, long len, long ptr);
void oppnd_doFullPacket(t_oppnd *x, long len, long ptr, t_symbol *sym_to_prepend, int sym_to_prepend_len);
void oppnd_cbk(t_osc_msg msg, void *v);
void oppnd_set(t_oppnd *x, t_symbol *sym_to_prepend);
void oppnd_anything(t_oppnd *x, t_symbol *msg, short argc, t_atom *argv);
void oppnd_list(t_oppnd *x, t_symbol *msg, int argc, t_atom *argv);
void oppnd_float(t_oppnd *x, double f);
void oppnd_long(t_oppnd *x, long l);
void oppnd_free(t_oppnd *x);
void oppnd_assist(t_oppnd *x, void *b, long m, long a, char *s);
void *oppnd_new(t_symbol *msg, short argc, t_atom *argv);

t_symbol *ps_FullPacket;

void oppnd_fullPacket(t_oppnd *x, long len, long ptr){
	oppnd_doFullPacket(x, len, ptr, x->sym_to_prepend, x->sym_to_prepend_len);
}

void oppnd_doFullPacket(t_oppnd *x, long len, long ptr, t_symbol *sym_to_prepend, int sym_to_prepend_len){
	int num_messages = 0;
	osc_bundle_s_getMsgCount(len, (char *)ptr, &num_messages);
	char buf[len + (num_messages * (sym_to_prepend_len + 4))]; // not exact, but more than enough
	char *bufptr = buf;
	memcpy(bufptr, (char *)ptr, OSC_HEADER_SIZE);
	bufptr += OSC_HEADER_SIZE;
	t_osc_bndl_it_s *it = osc_bndl_it_s_get(len, (char *)ptr);
	while(osc_bndl_it_s_hasNext(it)){
		t_osc_msg_s *msg = osc_bndl_it_s_next(it);
		int msg_address_len = strlen(osc_message_s_getAddress(msg));
		char *msg_address = osc_message_s_getAddress(msg);
		char new_address[sym_to_prepend_len + msg_address_len + 1];
		memcpy(new_address, sym_to_prepend->s_name, sym_to_prepend_len);
		memcpy(new_address + sym_to_prepend_len, msg_address, msg_address_len);
		new_address[sym_to_prepend_len + msg_address_len] = '\0';
		bufptr += osc_message_s_renameCopy(bufptr, msg, sym_to_prepend_len + msg_address_len, new_address);
		bufptr += 4;
	}
	osc_bndl_it_s_destroy(it);

	t_atom out[2];
	atom_setlong(out, bufptr - buf);
	atom_setlong(out + 1, (long)buf);
	outlet_anything(x->outlet, ps_FullPacket, 2, out);
}

void oppnd_set(t_oppnd *x, t_symbol *sym_to_prepend){
	x->sym_to_prepend = sym_to_prepend;
	x->sym_to_prepend_len = strlen(sym_to_prepend->s_name);
}

void oppnd_anything(t_oppnd *x, t_symbol *msg, short argc, t_atom *argv){
	t_symbol *address = msg, *sym_to_prepend = x->sym_to_prepend;
	if(atom_gettype(argv) == A_SYM){
		if(atom_getsym(argv) == ps_FullPacket){
			oppnd_doFullPacket(x, atom_getlong(argv + 1), atom_getlong(argv + 2), msg, strlen(msg->s_name));
			return;
		}else if(atom_getsym(argv)->s_name[0] == '/'){
			// msg and argv are both OSC addresses.  prepend msg to argv
			address = atom_getsym(argv);
			sym_to_prepend = msg;
			argc--;
			argv++;
		}

	}
	int address_len = 0, sym_to_prepend_len = 0;
	if(address){
		address_len = strlen(address->s_name);
	}
	if(sym_to_prepend){
		sym_to_prepend_len = strlen(sym_to_prepend->s_name);
	}
	char buf[address_len + sym_to_prepend_len];
	char address_string[address_len + 1];
	char sym_to_prepend_string[sym_to_prepend_len + 1];
	address_string[0] = '\0';
	sym_to_prepend_string[0] = '\0';
	if(address){
		strncpy(address_string, address->s_name, address_len);
		address_string[address_len] = '\0';
	}
	if(sym_to_prepend){
		strncpy(sym_to_prepend_string, sym_to_prepend->s_name, sym_to_prepend_len);
		sym_to_prepend_string[sym_to_prepend_len] = '\0';
	}
	sprintf(buf, "%s%s", sym_to_prepend_string, address_string);
	t_symbol *newaddress = gensym(buf);
	int len = omax_util_get_bundle_size_for_atoms(newaddress, argc, argv);
	char oscbuf[len];
	memset(oscbuf, '\0', len);
	strncpy(oscbuf, "#bundle\0", 8);
	omax_util_encode_atoms(oscbuf + 16, newaddress, argc, argv);
	t_atom out[2];
	atom_setlong(out, len);
	atom_setlong(out + 1, (long)oscbuf);
	outlet_anything(x->outlet, ps_FullPacket, 2, out);
	/*
	if(atom_gettype(argv) == A_SYM){
		if(atom_getsym(argv) == ps_FullPacket){
			oppnd_doFullPacket(x, atom_getlong(argv + 1), atom_getlong(argv + 2), msg);
			return;
		}else{
			t_symbol *sym_to_prepend = atom_getsym(argv);
			char buf[strlen(msg->s_name) + strlen(sym_to_prepend->s_name)];
			sprintf(buf, "%s%s", msg->s_name, sym_to_prepend->s_name);
			t_symbol *address = gensym(buf);
			int len = omax_util_get_bundle_size_for_atoms(gensym(buf), argc - 1, argv + 1);
			char oscbuf[len];
			memset(oscbuf, '\0', len);
			strncpy(oscbuf, "#bundle\0", 8);
			omax_util_encode_atoms(oscbuf + 16, address, argc - 1, argv + 1);
			t_atom out[2];
			atom_setlong(out, len);
			atom_setlong(out + 1, (long)oscbuf);
			outlet_anything(x->outlet, ps_FullPacket, 2, out);
			//outlet_anything(x->outlet, gensym(buf), argc - 1, argv + 1);
		}
	}else{
		char buf[strlen(msg->s_name) + strlen(x->sym_to_prepend->s_name)];
		sprintf(buf, "%s%s", x->sym_to_prepend->s_name, msg->s_name);
		t_symbol *address = gensym(buf);
		int len = omax_util_get_bundle_size_for_atoms(gensym(buf), argc, argv);
		char oscbuf[len];
		memset(oscbuf, '\0', len);
		strncpy(oscbuf, "#bundle\0", 8);
		omax_util_encode_atoms(oscbuf + 16, address, argc, argv);
		t_atom out[2];
		atom_setlong(out, len);
		atom_setlong(out + 1, (long)oscbuf);
		outlet_anything(x->outlet, ps_FullPacket, 2, out);
	}
	*/
}

void oppnd_list(t_oppnd *x, t_symbol *msg, int argc, t_atom *argv){
	oppnd_anything(x, NULL, argc, argv);
}

void oppnd_float(t_oppnd *x, double f){
	t_atom a;
	atom_setfloat(&a, f);
	oppnd_anything(x, NULL, 1, &a);
}

void oppnd_long(t_oppnd *x, long l){
	t_atom a;
	atom_setlong(&a, l);
	oppnd_anything(x, NULL, 1, &a);
}

void oppnd_assist(t_oppnd *x, void *b, long m, long a, char *s){
	if (m == ASSIST_OUTLET)
		if(x->sym_to_prepend){
			sprintf(s,"FullPacket with %s prepend to each address", x->sym_to_prepend->s_name);
		}else{
			sprintf(s,"FullPacket with <nothing> prepend to each address");
		}
	else {
		switch (a) {	
		case 0:
			if(x->sym_to_prepend){
				sprintf(s,"OSC FullPacket:  %s will be prepended to each address", x->sym_to_prepend->s_name);
			}else{
				sprintf(s,"OSC FullPacket:  <nothing> will be prepended to each address");
			}
			break;
		}
	}
}

void oppnd_free(t_oppnd *x){
}

void *oppnd_new(t_symbol *msg, short argc, t_atom *argv){
	t_oppnd *x;
	if(x = (t_oppnd *)object_alloc(oppnd_class)){
		x->outlet = outlet_new(x, "FullPacket");
		x->sym_to_prepend = NULL;
		if(argc){
			x->sym_to_prepend = atom_getsym(argv);
			x->sym_to_prepend_len = strlen(x->sym_to_prepend->s_name);
		}
	}
		   	
	return(x);
}

int main(void){
	t_class *c = class_new("o.prepend", (method)oppnd_new, (method)oppnd_free, sizeof(t_oppnd), 0L, A_GIMME, 0);
    
	class_addmethod(c, (method)oppnd_fullPacket, "FullPacket", A_LONG, A_LONG, 0);
	//class_addmethod(c, (method)oppnd_notify, "notify", A_CANT, 0);
	class_addmethod(c, (method)oppnd_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)oppnd_anything, "anything", A_GIMME, 0);
	class_addmethod(c, (method)oppnd_list, "list", A_GIMME, 0);
	class_addmethod(c, (method)oppnd_float, "float", A_FLOAT, 0);
	class_addmethod(c, (method)oppnd_long, "int", A_LONG, 0);

	class_addmethod(c, (method)oppnd_set, "set", A_SYM, 0);
    
	class_register(CLASS_BOX, c);
	oppnd_class = c;

	ps_FullPacket = gensym("FullPacket");
	common_symbols_init();
	return 0;
}
