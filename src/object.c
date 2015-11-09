#include "object.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "libstring.h"
#include "utils.h"

void lat_set_ctx(lat_object *ns, lat_object *name, lat_object *o)
{
	if (ns->type != T_INSTANCE) {
		debug("ns->type: %d", ns->type);
		log_err("Namespace no es una instancia");
		exit(1);
	}
	else {
		//printf("set_ctx: %s\n", lat_get_str_value(name));
		hash_map *h = ns->data.instance;
		set_hash(h, lat_get_str_value(name), (void *)o);
	}
}

lat_object *lat_get_ctx(lat_object *ns, lat_object *name)
{
	if (ns->type != T_INSTANCE) {
		debug("ns->type: %d", ns->type);
		log_err("Namespace is not an instance");
		exit(1);
	}
	else {
		//printf("get_ctx: %s\n", lat_get_str_value(name));
		hash_map *h = ns->data.instance;
		lat_object *ret = get_hash(h, lat_get_str_value(name));
		if (ret == NULL) {
			log_err("Variable \"%s\" indefinida", lat_get_str_value(name));
			exit(1);
		}
		return ret;
	}
}

int lat_ctx_has(lat_object *ns, lat_object *name)
{
	if (ns->type != T_INSTANCE) {
		debug("ns->type: %d", ns->type);
		log_err("Namespace no es una instancia");
		exit(1);
	}
	else {
		hash_map *h = ns->data.instance;
		lat_object *ret = get_hash(h, lat_get_str_value(name));
		if (ret == NULL) {
			return 0;
		}
		return 1;
	}
}

lat_object *lat_make_object(lat_vm *vm)
{
	/*if (vm->memory_usage > MAX_VIRTUAL_MEMORY){
		lat_gc(vm);
	}*/
	lat_object *ret = (lat_object *)malloc(sizeof(lat_object));
	ret->type = T_NULL;	
	ret->data_size = 0;	
	return ret;
}

lat_object *lat_instance(lat_vm *vm)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_INSTANCE;
	ret->data_size = sizeof(hash_map *);
	ret->data.instance = make_hash_map();
	/*no collect instance types*/
	return ret;
}

lat_object *lat_char(lat_vm *vm, char val)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_CHAR;
	ret->data_size = sizeof(char);
	ret->data.c = val;
	vm->memory_usage += ret->data_size;
	lat_gc_add_object(vm, ret);
	return ret;
}

lat_object *lat_int(lat_vm *vm, long val)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_INT;
	ret->data_size = sizeof(long);
	ret->data.i = val;
	//lat_mark_object(ret, 2);
	vm->memory_usage += ret->data_size;
	lat_gc_add_object(vm, ret);
	return ret;
}

lat_object *lat_double(lat_vm *vm, double val)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_DOUBLE;
	ret->data_size = sizeof(double);
	ret->data.d = val;
	vm->memory_usage += ret->data_size;
	lat_gc_add_object(vm, ret);
	return ret;
}

lat_object *lat_str(lat_vm *vm, char *val)
{
    lat_object *ret = lat_str_new(val, strlen(val));
	/*no collect str types*/
	//vm->memory_usage += ret->data_size;
	lat_mark_object(ret, 3);
	return ret;
}

lat_object *lat_bool(lat_vm *vm, bool val)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_BOOL;
	ret->data_size = sizeof(bool);
	ret->data.b = val;
	//vm->memory_usage += ret->data_size;
	return ret;
}

lat_object *lat_list(lat_vm *vm, list_node *l)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_LIST;
	ret->data_size = sizeof(list_node *);
	ret->data.list = l;
	//vm->memory_usage += ret->data_size;
	//lat_gc_add_object(vm, ret);
	return ret;
}

lat_object *lat_func(lat_vm *vm)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_FUNC;
	ret->data_size = 0;
	/*lat_gc(vm);
	lat_gc_add_object(vm, ret);*/
	return ret; //We don't do anything here: all bytecode will be added later
}

lat_object *lat_cfunc(lat_vm *vm)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_CFUNC;
	/*lat_gc(vm);
	lat_gc_add_object(vm, ret);*/
	return ret;
}

lat_object *lat_struct(lat_vm *vm, void *val)
{
	lat_object *ret = lat_make_object(vm);
	ret->type = T_STRUCT;
	ret->data.cstruct = val;
	/*lat_gc(vm);
	lat_gc_add_object(vm, ret);*/
	return ret;
}

void lat_mark_object(lat_object *o, int m)
{
	if (o != NULL) {
		o->marked = m;
		switch (o->type) {
		case T_INSTANCE:
			lat_mark_hash(o->data.instance, m);
			break;
		case T_LIST:
			lat_mark_list(o->data.list, m);
			break;
		default:
			break;
		}
	}
}

void lat_mark_list(list_node *l, unsigned char m)
{
	if (l != NULL) {
		list_node *c;
		for (c = l->next; c != NULL; c = c->next) {
			if (c->data != NULL) {
				lat_mark_object((lat_object *)c->data, m);
			}
		}
	}
}

void lat_mark_hash(hash_map *h, unsigned char m)
{
	int c = 0;
	list_node *l;
	list_node *cur;
	hash_val *hv;
	for (c = 0; c < 256; ++c) {
		l = h->buckets[c];
		if (l != NULL) {
			for (cur = l->next; cur != NULL; cur = cur->next) {
				if (cur->data != NULL) {
					hv = (hash_val *)cur->data;
					lat_mark_object((lat_object *)hv->val, m);
				}
			}
		}
	}
}

void lat_delete_object(lat_vm *vm, lat_object *o)
{
	switch (o->type) {
	case T_NULL:
		return;
		break;
	case T_INSTANCE:
		return;
		break;
	case T_LIST:
		//lat_delete_list(vm, o->data.list);
		break;
	case T_CHAR:
	case T_INT:
	case T_DOUBLE:
	case T_BOOL:
		vm->memory_usage -= o->data_size;
		break;
	case T_STR:
		//if (o->data.str != NULL){
			//vm->memory_usage -= sizeof(o->data.str);
		//	free(o->data.str);
		//}
		return;
		break;
	case T_FUNC:
	case T_CFUNC:
		return;
	case T_STRUCT:
		return;
		break;
	}
	//printf("deleted object: %p\n", &o);
	free(o);
}

void lat_delete_list(lat_vm *vm, list_node *l)
{
	if (l != NULL) {
		list_node *c;
		for (c = l->next; c != NULL; c = c->next) {
			if (c->data != NULL) {
				lat_delete_object(vm, (lat_object *)c->data);
			}
			free(c);
		}
	}
	free(l);
}

void lat_delete_hash(lat_vm *vm, hash_map *h)
{
	int c = 0;
	list_node *l;
	list_node *cur;
	hash_val *hv;
	for (c = 0; c < 256; ++c) {
		l = h->buckets[c];
		if (l != NULL) {
			for (cur = l->next; cur != NULL; cur = cur->next) {
				if (cur != NULL){
					if (cur->data != NULL) {
						hv = (hash_val *)cur->data;
						lat_delete_object(vm, (lat_object *)hv->val);
						free(hv);
					}
					//free(cur);
				}
			}
			free(l);
		}
	}
}

lat_object *lat_clone_object(lat_vm *vm, lat_object *obj)
{
	lat_object *ret;
	switch (obj->type) {
	case T_INSTANCE:
		ret = lat_make_object(vm);
		ret->type = T_INSTANCE;
		ret->data_size = sizeof(hash_map *);
		ret->data.instance = lat_clone_hash(vm, obj->data.instance);
		//ret->data.instance = obj->data.instance;
		break;
	case T_LIST:
		ret = lat_list(vm, lat_clone_list(vm, obj->data.list));
		//ret = lat_list(vm, obj->data.list);
		break;
	case T_FUNC:
	case T_CFUNC:
		ret = obj;
		break;
	default:
		ret = lat_make_object(vm);
		ret->type = obj->type;
		ret->marked = obj->marked;
		ret->data_size = obj->data_size;
		ret->data = obj->data;
		break;
	}
	//vm->memory_usage += ret->data_size;
	return ret;
}

list_node *lat_clone_list(lat_vm *vm, list_node *l)
{
	list_node *ret = make_list();
	if (l != NULL) {
		list_node *c;
		for (c = l->next; c != NULL; c = c->next) {
			if (c->data != NULL) {
				insert_list(ret, lat_clone_object(vm, (lat_object *)c->data));
			}
		}
	}
	return ret;
}

hash_map *lat_clone_hash(lat_vm *vm, hash_map *h)
{
	int c = 0;
	hash_map *ret = make_hash_map();
	list_node *l;
	for (c = 0; c < 256; ++c) {
		l = h->buckets[c];
		if (l != NULL) {
			ret->buckets[c] = make_list();
			if (l != NULL) {
				list_node *cur;
				for (cur = l->next; cur != NULL; cur = cur->next) {
					if (cur->data != NULL) {
						hash_val *hv = (hash_val *)malloc(sizeof(hash_val));
						//vm->memory_usage += sizeof(hash_val);
						strncpy(hv->key, ((hash_val *)cur->data)->key, 256);
						hv->val = lat_clone_object(vm, (lat_object *)((hash_val *)cur->data)->val);
						insert_list(ret->buckets[c], hv);
					}
				}
			}
		}
	}
	return ret;
}

char lat_get_char_value(lat_object *o)
{
	if (o->type == T_CHAR) {
		return o->data.c;
	}
	log_err("Object no es un tipo caracter");
	exit(1);
}

int lat_get_int_value(lat_object *o)
{
	if (o->type == T_INT) {
		return o->data.i;
	}
	log_err("Object no es un tipo entero");
	exit(1);
}

double lat_get_double_value(lat_object *o)
{
	if (o->type == T_DOUBLE) {
		return o->data.d;
	}
	else if (o->type == T_INT) {
		return (double)o->data.i;
	}
	log_err("Object no es un tipo numerico");
	exit(1);
}

char *lat_get_str_value(lat_object *o)
{
	if (o->type == T_STR) {
		return o->data.str;
	}
	log_err("Object no es un tipo cadena");
	exit(1);
}

bool lat_get_bool_value(lat_object *o)
{
	if (o->type == T_BOOL) {
		return o->data.b;
	}
	if (o->type == T_INT) {
		return o->data.i;
	}
	log_err("Object no es un tipo logico");
	exit(1);
}

list_node *lat_get_list_value(lat_object *o)
{
	if (o->type == T_LIST) {
		return o->data.list;
	}
	log_err("Object no es un tipo lista");
	exit(1);
}

void *lat_get_struct_value(lat_object *o)
{
	if (o->type == T_STRUCT) {
		return o->data.list;
	}
	log_err("Object no es un tipo estructura");
	exit(1);
}