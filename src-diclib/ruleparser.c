/*
 * ����ե�����ʤɤΤ����
 * ���ѤΥե������ɤ߹��ߥ⥸�塼��
 *
 * Copyright (C) 2000-2004 TABATA Yusuke
 *
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <conf.h>
#include <ruleparser.h>
#include <logger.h>

/* ʸˡ�ե�����Υѡ����Ѥ���� */
#define MAX_TOKEN_LEN 256
/* ����Υ��󥯥롼�ɤο��� */
#define MAX_INCLUDE_DEPTH 4

#define PS_INIT 0
#define PS_TOKEN 1
#define PS_EOF 2
#define PS_RET 3

#define NL "NL"

static struct parser_stat {
  FILE *fp_stack[MAX_INCLUDE_DEPTH];
  FILE *fp;
  int cur_fpp;/* �����å��Υ���ǥå��� */
  int line_num;
  char **tokens;
  int nr_token;
} g_ps;

struct line_stat{
  int stat;
  char buf[MAX_TOKEN_LEN];
  int buf_index;
};

static FILE *
open_file_in_confdir(const char *fn)
{
  const char *dn;
  char *full;
  size_t dname_len;

  if (!fn) {
    return stdin;
  }

  if (fn[0] == '/') {
    /* ���Хѥ��ʤΤǤ��Τޤ�fopen */
    return fopen(fn, "r");
  }

  dn = anthy_conf_get_str("ANTHYDIR");
  if (!dn) {
    return 0;
  }
  dname_len =  strlen(dn);
  full = alloca(dname_len + strlen(fn) + 2);
  sprintf(full, "%s/%s", dn, fn);

  return fopen(full, "r");
}

/** �Хå�����å���ˤ�륨�������פ��������getc
 * ���������פ��줿ʸ���ʤ���֤��ͤ�1�ˤʤ롣
 */
static int
mygetc (int *cc)
{
  *cc = fgetc(g_ps.fp);
  if (*cc == '\\') {
    int c2 = fgetc(g_ps.fp);
    switch(c2) {
    case '\\':
      *cc = '\\';
      return 1;
    case '\n':
      *cc = ' ';
      return 1;
    case '\"':
      *cc = '\"';
      return 1;
    default:;
      /* go through */
    }
  }
  return 0;
}

#define myisblank(c)	((c) == ' ' || (c) == '\t')

/* �Ԥ˰�ʸ���ɲä��� */
static void
pushchar(struct line_stat *ls, int cc)
{
  ls->buf[ls->buf_index] = cc;
  ls->buf_index ++;
}

static const char *
get_token_in(struct line_stat *ls)
{
  int cc, esc;
  int in_quote = 0;
  if (ls->stat == PS_EOF) {
    return NULL;
  }
  if (ls->stat == PS_RET) {
    return NL;
  }
  /* �ȡ����󤬻Ϥޤ�ޤǶ�����ɤ����Ф� */
  do {
    esc = mygetc(&cc);
  } while (cc > 0 && myisblank(cc) && esc == 0);
  if (cc == -1) {
    return NULL;
  }
  if (cc == '\n'){
    return NL;
  }

  /**/
  if (cc == '\"' && !esc) {
    in_quote = 1;
  }
  /**/
  do {
    pushchar(ls, cc);
    esc = mygetc(&cc);
    if (cc < 0){
      /* EOF */
      pushchar(ls, 0);
      ls->stat = PS_EOF;
      return ls->buf;
    }
    if (cc == '\n' && !esc) {
      /* ���� */
      pushchar(ls, 0);
      ls->stat = PS_RET;
      return ls->buf;
    }
    if (!in_quote && myisblank(cc)) {
      break;
    }
    if (in_quote && cc == '\"' && !esc) {
      pushchar(ls, '\"');
      break;
    }
  } while (1);
  pushchar(ls, 0);
  return ls->buf;
}

/* ����ɤ� */
static int
get_line_in(void)
{
  const char *t;
  struct line_stat ls;

  ls.stat = PS_INIT;
  do{
    ls.buf_index = 0;
    t = get_token_in(&ls);
    if (!t) {
      return -1;
    }
    if (t == NL) {
      return 0;
    }
    g_ps.nr_token++;
    g_ps.tokens = realloc(g_ps.tokens, sizeof(char *)*g_ps.nr_token);
    g_ps.tokens[g_ps.nr_token-1] = strdup(t);
  } while(1);
}

static void
proc_include(void)
{
  FILE *fp;
  if (g_ps.nr_token != 2) {
    anthy_log(0, "Syntax error in include directive.\n");
    return ;
  }
  if (g_ps.cur_fpp > MAX_INCLUDE_DEPTH - 1) {
    anthy_log(0, "Too deep include.\n");
    return ;
  }
  fp = open_file_in_confdir(g_ps.tokens[1]);
  if (!fp) {
    anthy_log(0, "Failed to open %s.\n", g_ps.tokens[1]);
    return ;
  }
  g_ps.cur_fpp++;
  g_ps.fp_stack[g_ps.cur_fpp] = fp;
  g_ps.fp = fp;
}

/* ���󥯥롼�ɤΥͥ��Ȥ򲼤��� */
static void
pop_file(void)
{
  fclose(g_ps.fp);
  g_ps.cur_fpp --;
  g_ps.fp = g_ps.fp_stack[g_ps.cur_fpp];
}

static void
get_line(void)
{
  int r;
  
 again:
  anthy_free_line();
  g_ps.line_num ++;

  r = get_line_in();
  if (r == -1){
    /* EOF���Ǥ���ʾ��ɤ�� */
    if (g_ps.cur_fpp > 0) {
      pop_file();
      goto again;
    }else{
      return ;
    }
  }
  if (!g_ps.nr_token) {
    return ;
  }
  if (!strcmp(g_ps.tokens[0], "\\include")) {
    proc_include();
    goto again;
  }else if (!strcmp(g_ps.tokens[0], "\\eof")) {
    if (g_ps.cur_fpp > 0) {
      pop_file();
      goto again;
    }else{
      anthy_free_line();
      return ;
    }
  }
  if (g_ps.tokens[0][0] == '#'){
    goto again;
  }
}

void
anthy_free_line(void)
{
  int i;
  for (i = 0; i < g_ps.nr_token; i++) {
    free(g_ps.tokens[i]);
  }
  free(g_ps.tokens);
  g_ps.tokens = 0;
  g_ps.nr_token = 0;
}

int
anthy_open_file(const char *fn)
{
  g_ps.fp_stack[0] = open_file_in_confdir(fn);
  if (!g_ps.fp_stack[0]) {
    return -1;
  }
  /* �ѡ����ξ��֤��������� */
  g_ps.cur_fpp = 0;
  g_ps.fp = g_ps.fp_stack[0];
  g_ps.line_num = 0;
  return 0;
}

void
anthy_close_file(void)
{
  if (g_ps.fp != stdin) {
    fclose(g_ps.fp);
  }
}

int
anthy_read_line(char ***tokens, int *nr)
{
  get_line();
  *tokens = g_ps.tokens;
  *nr = g_ps.nr_token;
  if (!*nr) {
    return -1;
  }
  return 0;
}

int
anthy_get_line_number(void)
{
  return g_ps.line_num;
}