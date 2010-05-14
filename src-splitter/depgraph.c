/*
 * ʸ��μ�Ω����(��Ƭ�����������ޤ�)��³��
 * ���졢��ư��ʤɤ���°��Υѥ�����򤿤ɤ롣
 * �ѥ�����ϥ���դȤ�������ե�������Ѱդ��롣
 *
 *
 *  +------+
 *  |      |
 *  |branch+--cond--+--transition--> node
 *  |      |        +--transition--> node
 *  | NODE |
 *  |      |
 *  |branch+--cond-----transition--> node
 *  |      |
 *  |branch+--cond-----transition--> node
 *  |      |
 *  +------+
 *
 * Copyright (C) 2000-2003 TABATA Yusuke
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include <anthy.h>

#include <conf.h>
#include <ruleparser.h>
#include <xstr.h>
#include <logger.h>
#include <segclass.h>
#include <splitter.h>
#include <wtype.h>
#include "wordborder.h"

static int nrNodes;

#define  NORMAL_CONNECTION 1
#define  WEAKER_CONNECTION 2
#define  WEAK_CONNECTION 8

struct dep_transition {
  /** ������ΥΡ��ɤ��ֹ� 0�ξ��Ͻ�ü */
  int next_node;
  /** ���ܤΥ����� */
  int trans_ratio;
  /** �ʻ� */
  int pos;
  /** ���ѷ� */
  int ct;
  /* ��°�쥯�饹 */
  enum dep_class dc;

  int head_pos;
  int weak;
};

struct dep_branch {
  /* ���ܾ�����°������� */
  /** ����Ĺ */
  int nr_strs;
  /** ���ܾ������� */
  xstr **str;
  
  /** ������ΥΡ��� */
  int nr_transitions;
  struct dep_transition *transition;
};

static struct dep_node {
  /** �Ρ��ɤ�̾�� */
  char *name;

  int nr_branch;
  struct dep_branch *branch;
}*gNodes;

/* ��������Τ����ʸ�����ͭ����pool */
static struct {
  int nr_xs;
  xstr **xs;
} xstr_pool ;

static void
match_branch(struct splitter_context *sc,
	     struct word_list *tmpl,
	     xstr *xs, xstr *cond_xs, struct dep_branch *db);
static void
match_nodes(struct splitter_context *sc,
	    struct word_list *wl,
	    xstr follow_str, int node);

static void
release_xstr_pool(void)
{
  int i;
  for (i = 0; i < xstr_pool.nr_xs; i++) {
    free(xstr_pool.xs[i]->str);
    free(xstr_pool.xs[i]);
  }
  free(xstr_pool.xs);
  xstr_pool.nr_xs = 0;
}

/* ʸ����pool����ʸ����򸡺����� */
static xstr *
get_xstr_from_pool(char *str)
{
  int i;
  xstr *xs;
#ifdef USE_UCS4
  xs = anthy_cstr_to_xstr(str, ANTHY_EUC_JP_ENCODING);
#else
  xs = anthy_cstr_to_xstr(str, ANTHY_COMPILED_ENCODING);
#endif
  /* pool�˴��ˤ��뤫õ�� */
  for (i = 0; i < xstr_pool.nr_xs; i++) {
    if (!anthy_xstrcmp(xs, xstr_pool.xs[i])) {
      anthy_free_xstr(xs);
      return xstr_pool.xs[i];
    }
  }
  /* ̵���Τǡ���� */
  xstr_pool.xs = realloc(xstr_pool.xs,
			 sizeof(xstr *) * (xstr_pool.nr_xs+1));
  xstr_pool.xs[xstr_pool.nr_xs] = xs;
  xstr_pool.nr_xs ++;
  return xs;
}

/* ʸˡ����ե�������˶��ΥΡ��ɤ����뤫�����å����� */
static void
check_nodes(void)
{
  int i;
  for (i = 1; i < nrNodes; i++) {
    if (gNodes[i].nr_branch == 0) {
      anthy_log(0, "node %s has no branch.\n", gNodes[i].name);
    }
  }
}


/*
 * �ƥΡ��ɤˤ��������ܾ���ƥ��Ȥ���
 *
 * wl ��Ω������word_list
 * follow_str ��Ω�����ʹߤ�ʸ����
 * node �롼����ֹ�
 */
static void
match_nodes(struct splitter_context *sc,
	    struct word_list *wl,
	    xstr follow_str, int node)
{
  struct dep_node *dn = &gNodes[node];
  struct dep_branch *db;
  int i,j;

  /* �ƥ롼��� */
  for (i = 0; i < dn->nr_branch; i++) {
    db = &dn->branch[i];
    /* �����ܾ�� */
    for (j = 0; j < db->nr_strs; j++) {
      xstr cond_xs;
      /* ��°����������ܾ����Ĺ�����Ȥ�ɬ�� */
      if (follow_str.len < db->str[j]->len){
	continue;
      }
      /* ���ܾ�����ʬ���ڤ�Ф� */
      cond_xs.str = follow_str.str;
      cond_xs.len = db->str[j]->len;

      /* ���ܾ�����Ӥ��� */
      if (!anthy_xstrcmp(&cond_xs, db->str[j])) {
	/* ���ܾ���match���� */
	struct word_list new_wl = *wl;
	struct part_info *part = &new_wl.part[PART_DEPWORD];
	xstr new_follow;

	part->len += cond_xs.len;
	new_follow.str = &follow_str.str[cond_xs.len];
	new_follow.len = follow_str.len - cond_xs.len;
	/* ���ܤ��Ƥߤ� */
	match_branch(sc, &new_wl, &new_follow, &cond_xs, db);
      }
    }
  }
}

/*
 * �����ܤ�¹Ԥ��Ƥߤ�
 *
 * tmpl �����ޤǤ˹�������word_list
 * xs �Ĥ��ʸ����
 * cond_xs ���ܤ˻Ȥ�줿ʸ����
 * db ����Ĵ�����branch
 */
static void
match_branch(struct splitter_context *sc,
	     struct word_list *tmpl,
	     xstr *xs, xstr *cond_xs, struct dep_branch *db)
{
  struct part_info *part = &tmpl->part[PART_DEPWORD];
  int i;

  /* ��������˥ȥ饤���� */
  for (i = 0; i < db->nr_transitions; i++) {
    int conn_ratio = part->ratio; /* score����¸ */
    int weak_len = tmpl->weak_len;/* weak�����ܤ�Ĺ������¸*/ 
    int head_pos = tmpl->head_pos; /* �ʻ�ξ��� */
    enum dep_class dc = part->dc;
    struct dep_transition *transition = &db->transition[i];

    /* �������ܤΥ����� */
    part->ratio *= transition->trans_ratio;
    part->ratio /= RATIO_BASE;
    if (transition->weak || /* �夤���� */
	(transition->dc == DEP_END && xs->len > 0)) { /* ��ü����ʤ��Τ˽�ü°��*/
      tmpl->weak_len += cond_xs->len;
    } else {
      /* �������ܤ���°��˲��� */
      part->ratio += cond_xs->len * cond_xs->len * cond_xs->len * 3;
    }

    tmpl->tail_ct = transition->ct;
    /* ���ܤγ��ѷ����ʻ� */
    if (transition->dc != DEP_NONE) {
      part->dc = transition->dc;

    }
    /* ̾�첽����ư�������ʻ�̾���� */
    if (transition->head_pos != POS_NONE) {
      tmpl->head_pos = transition->head_pos;
    }

    /* ���ܤ���ü�� */
    if (transition->next_node) {
      /* ���� */
      match_nodes(sc, tmpl, *xs, transition->next_node);
    } else {
      struct word_list *wl;
      xstr xs_tmp;

      /* 
       * ��ü�Ρ��ɤ���ã�����Τǡ�
       * �����word_list�Ȥ��ƥ��ߥå�
       */
      wl = anthy_alloc_word_list(sc);
      *wl = *tmpl;
      wl->len += part->len;

      /* ��ʸ������°��Ƕ�����³�Τ�Τ��ɤ�����Ƚ�ꤹ�� */
      xs_tmp = *xs;
      xs_tmp.str--;
      if (wl->part[PART_DEPWORD].len == 1 &&
	  (anthy_get_xchar_type(xs_tmp.str[0]) & XCT_STRONG)) {
	wl->part[PART_DEPWORD].ratio *= 3;
	wl->part[PART_DEPWORD].ratio /= 2;
      }
      /**/
      anthy_commit_word_list(sc, wl);
    }
    /* ���ᤷ */
    part->ratio = conn_ratio;
    part->dc = dc;
    tmpl->weak_len = weak_len;
    tmpl->head_pos = head_pos;
  }
}

/** ��������
 */
void
anthy_scan_node(struct splitter_context *sc,
		struct word_list *tmpl,
		xstr *follow, int node)
{
  /* ��°����դ��Ƥ��ʤ����֤��鸡���򳫻Ϥ��� */
  match_nodes(sc, tmpl, *follow, node);
}

int
anthy_get_node_id_by_name(const char *name)
{
  int i;
  /* ��Ͽ�ѤߤΤ�Τ���õ�� */
  for (i = 0; i < nrNodes; i++) {
    if (!strcmp(name,gNodes[i].name)) {
      return i;
    }
  }
  /* �ʤ��ä��ΤǺ�� */
  gNodes = realloc(gNodes, sizeof(struct dep_node)*(nrNodes+1));
  gNodes[nrNodes].name = strdup(name);
  gNodes[nrNodes].nr_branch = 0;
  gNodes[nrNodes].branch = 0;
  nrNodes++;
  return nrNodes-1;
}

/*
 * ���ܤ�parse����
 *  doc/SPLITTER����
 */
static void
parse_transition(char *token, struct dep_transition *tr)
{
  int conn = NORMAL_CONNECTION;
  int ct = CT_NONE;
  int pos = POS_NONE;
  enum dep_class dc = DEP_NONE;
  char *str = token;
  tr->head_pos = POS_NONE;
  tr->weak = 0;
  /* ���ܤ�°�������*/
  while (*token != '@') {
    switch(*token){
    case ':':
      conn = WEAKER_CONNECTION;
      tr->weak = 1;
      break;
    case '.':
      conn = WEAK_CONNECTION;
      tr->weak = 1;
      break;
    case 'C':
      /* ���ѷ� */
      switch (token[1]) {
      case 'z': ct = CT_MIZEN; break;
      case 'y': ct = CT_RENYOU; break;
      case 's': ct = CT_SYUSI; break;
      case 't': ct = CT_RENTAI; break;
      case 'k': ct = CT_KATEI; break;
      case 'm': ct = CT_MEIREI; break;
      case 'g': ct = CT_HEAD; break;
      }
      token ++;
      break;
    case 'H':
      /* ��Ω�������ʻ� */
      switch (token[1]) {
      case 'n':	tr->head_pos = POS_NOUN; break;
      case 'v':	tr->head_pos = POS_V; break;
      case 'j':	tr->head_pos = POS_AJV; break;
      }
      token ++;
      break;
    case 'S':
      /* ʸ���°�� */
      switch (token[1]) {
	/*      case 'n': sc = DEP_NO; break;*/
      case 'f': dc = DEP_FUZOKUGO; break;
      case 'k': dc = DEP_KAKUJOSHI; break;
      case 'y': dc = DEP_RENYOU; break;
      case 't': dc = DEP_RENTAI; break;
      case 'e': dc = DEP_END; break;
      case 'r': dc = DEP_RAW; break;
      default: printf("unknown (S%c)\n", token[1]);
      }
      token ++;
      break;
    default:
      printf("Unknown (%c) %s\n", *token, str);
      break;
    }
    token ++;
  }
  /* @�����ϥΡ��ɤ�̾�� */
  tr->next_node = anthy_get_node_id_by_name(token);
  /* ��³�ζ��� */
  tr->trans_ratio = RATIO_BASE / conn;
  /**/
  tr->pos = pos;
  tr->ct = ct;
  tr->dc = dc;
}

/* ���ܾ�狼��branch���ܤ��Ф� */
static struct dep_branch *
find_branch(struct dep_node *node, xstr **strs, int nr_strs)
{
  struct dep_branch *db;
  int i, j;
  /* Ʊ�����ܾ��Υ֥�����õ�� */
  for (i = 0; i < node->nr_branch; i++) {
    db = &node->branch[i];
    if (nr_strs != db->nr_strs) {
      continue ;
    }
    for (j = 0; j < nr_strs; j++) {
      if (anthy_xstrcmp(db->str[j], strs[j])) {
	goto fail;
      }
    }
    /**/
    return db;
  fail:;
  }
  /* �������֥�������ݤ��� */
  node->branch = realloc(node->branch,
			 sizeof(struct dep_branch)*(node->nr_branch+1));
  db = &node->branch[node->nr_branch];
  node->nr_branch++;
  db->str = malloc(sizeof(xstr*)*nr_strs);
  for (i = 0; i < nr_strs; i++) {
    db->str[i] = strs[i];
  }
  db->nr_strs = nr_strs;
  db->nr_transitions = 0;
  db->transition = 0;
  return db;
}

/*
 * �Ρ���̾ ���ܾ��+ ������+
 */
static void
parse_line(char **tokens, int nr)
{
  int id, row = 0;
  struct dep_branch *db;
  struct dep_node *dn;
  int nr_strs;
  xstr **strs = alloca(sizeof(xstr*) * nr);

  /* �Ρ��ɤȤ���id����� */
  id = anthy_get_node_id_by_name(tokens[row]);
  dn = &gNodes[id];
  row ++;

  nr_strs = 0;

  /* ���ܾ�����°���������� */
  for (; row < nr && tokens[row][0] == '\"'; row++) {
    char *s;
    s = strdup(&tokens[row][1]);
    s[strlen(s)-1] =0;
    strs[nr_strs] = get_xstr_from_pool(s);
    nr_strs ++;
    free(s);
  }

  /* ���ܾ�郎�ʤ����Ϸٹ��Ф��ơ��������ܾ����ɲä��� */
  if (nr_strs == 0) {
    char *s;
    anthy_log(0, "node %s has a branch without any transition condition.\n",
	      tokens[0]);
    s = strdup("");
    strs[0] = get_xstr_from_pool(s);
    nr_strs = 1;
    free(s);
  }

  /* �֥�����������ΥΡ��ɤ��ɲä��� */
  db = find_branch(dn, strs, nr_strs);
  for ( ; row < nr; row++){
    db->transition = realloc(db->transition,
			     sizeof(struct dep_transition)*
			     (db->nr_transitions+1));
    parse_transition(tokens[row], &db->transition[db->nr_transitions]);
    db->nr_transitions ++;
  }
}

int
anthy_init_depword_tab()
{
  const char *fn;
  char **tokens;
  int nr;

  xstr_pool.nr_xs = 0;
  xstr_pool.xs = NULL;

  /* id 0 ����Ρ��ɤ˳����Ƥ� */
  anthy_get_node_id_by_name("@");

  fn = anthy_conf_get_str("DEPWORD");
  if (!fn) {
    anthy_log(0, "Dependent word dictionary is unspecified.\n");
    return -1;
  }
  if (anthy_open_file(fn) == -1) {
    anthy_log(0, "Failed to open dep word dict (%s).\n", fn);
    return -1;
  }
  /* ��Ԥ�����°�쥰��դ��ɤ� */
  while (!anthy_read_line(&tokens, &nr)) {
    parse_line(tokens, nr);
    anthy_free_line();
  }
  anthy_close_file();
  check_nodes();
  return 0;
}

void
anthy_release_depword_tab(void)
{
  int i, j;
  for (i = 0; i < nrNodes; i++) {
    free(gNodes[i].name);
    for (j = 0; j < gNodes[i].nr_branch; j++) {
      free(gNodes[i].branch[j].str);
      free(gNodes[i].branch[j].transition);
    }
    free(gNodes[i].branch);
  }
  free(gNodes);
  gNodes = 0;
  nrNodes = 0;

  release_xstr_pool();
}
