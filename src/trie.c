/* ircd-micro, trie.c -- radix trie
   Copyright (C) 2013 Alex Iadicicco

   This file is protected under the terms contained
   in the COPYING file in the project root */

#include "ircd.h"

static void __null_canonize(char *s) { }

static u_trie_e *trie_e_new(u_trie_e *up)
{
	u_trie_e *e;
	int i;

	e = malloc(sizeof(*e));
	e->val = NULL;
	e->up = up;
	for (i=0; i<16; i++)
		e->n[i] = NULL;

	return e;
}

static void trie_e_del(u_trie_e *e)
{
	free(e);
}

u_trie *u_trie_new(void (*canonize)(char*))
{
	u_trie *trie;
	int i;

	trie = malloc(sizeof(*trie));
	trie->canonize = canonize ? canonize : __null_canonize;

	trie->n.val = NULL;
	trie->n.up = NULL;
	for (i=0; i<16; i++)
		trie->n.n[i] = 0;

	return trie;
}

static void free_real(u_trie_e *e, u_trie_cb_t *cb, void *priv)
{
	int i;

	for (i=0; i<16; i++) {
		if (!e->n[i])
			continue;
		free_real(e->n[i], cb, priv);
		trie_e_del(e->n[i]);
	}

	if (cb && e->val)
		cb(e->val, priv);
}

void u_trie_free(u_trie *trie, u_trie_cb_t *cb, void *priv)
{
	free_real(&trie->n, cb, priv);
	free(trie);
}

static uchar nibble(uchar *s, int i)
{
	return (i%2==0) ? s[i/2]>>4 : s[i/2]&0xf;
}

static u_trie_e *retrieval(u_trie *trie, char *tkey, int create)
{
	u_trie_e *n;
	char key[U_TRIE_KEY_MAX];
	uint c, nib = 0; /* even=high, odd=low */

	u_strlcpy(key, tkey, U_TRIE_KEY_MAX);

	trie->canonize(key);

	if (!key[0])
		return NULL;

	n = &trie->n;

	for (; n && key[nib/2]; nib++) {
		c = nibble((uchar*)key, nib);
		if (n->n[c] == NULL) {
			if (create)
				n->n[c] = trie_e_new(n);
			else
				return NULL;
		}
		n = n->n[c];
	}

	return n;
}

void u_trie_set(u_trie *trie, char *key, void *val)
{
	u_trie_e *n = retrieval(trie, key, 1);
	u_log(LG_FINE, "TRIE:SET: [%p] %s", trie, key);
	n->val = val;
}

void *u_trie_get(u_trie *trie, char *key)
{
	u_trie_e *n = retrieval(trie, key, 0);
	return n ? n->val : NULL;
}

static void each(u_trie_e *e, u_trie_cb_t *cb, void *priv)
{
	int i;

	if (e->val != NULL)
		cb(e->val, priv);

	for (i=0; i<16; i++) {
		if (e->n[i] != NULL)
			each(e->n[i], cb, priv);
	}
}

void u_trie_each(u_trie *trie, char *pfx, u_trie_cb_t *cb, void *priv)
{
	u_trie_e *e = &trie->n;
	if (pfx && pfx[0]) {
		e = retrieval(trie, pfx, 0);
		if (!e) return;
	}
	each(e, cb, priv);
}

void *u_trie_del(u_trie *trie, char *key)
{
	u_trie_e *prev, *cur;
	void *val;
	int i, nempty;

	cur = retrieval(trie, key, 0);

	if (cur == NULL)
		return NULL;

	u_log(LG_FINE, "TRIE:DEL: [%p] %s", trie, key);

	val = cur->val;

	cur->val = NULL;
	prev = NULL;
	while (cur) {
		nempty = 0;
		for (i=0; i<16; i++) {
			if (cur->n[i] == prev)
				cur->n[i] = NULL;
			if (!cur->n[i])
				nempty++;
		}
		prev = cur;
		if (nempty != 16 || cur->val || cur == &trie->n)
			break;
		cur = cur->up;
		trie_e_del(prev);
	}

	return val;
}
