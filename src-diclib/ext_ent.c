/*
 * "123" "ABC" �Τ褦�ʼ���ˤΤäƤʤ�
 * ʸ������Ф�����礻�ξ������Ƥθ���򤳤�����������
 *
 * Copyright (C) 2001-2003 TABATA Yusuke
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dic_main.h"
#include "dic_ent.h"
#include "xchar.h"

/* ext entry */
static struct seq_ent unkseq_ent;/*̤��ʸ���󤿤Ȥ��б�ʸ����Ȥ�*/
static struct seq_ent num_ent;/*�����ʤ�*/
static struct seq_ent sep_ent;/*���ѥ졼���ʤɡ�*/
/* ext entry��wtype*/
static wtype_t wt_num;

static xchar narrow_wide_tab[]= {WIDE_0, WIDE_1, WIDE_2,
				 WIDE_3, WIDE_4, WIDE_5,
				 WIDE_6, WIDE_7, WIDE_8, WIDE_9};
static int kj_num_tab[]={KJ_0, KJ_1, KJ_2, KJ_3, KJ_4,
			 KJ_5, KJ_6, KJ_7, KJ_8, KJ_9};

static void
init_ext_ents(void)
{
  unkseq_ent.seq_type = ST_UNKSEQ|ST_WORD;
  unkseq_ent.nr_dic_ents = 0;
  unkseq_ent.flags = F_NONE;
  num_ent.seq_type = ST_WORD;
  num_ent.nr_dic_ents = 0;
  num_ent.flags = F_NONE;
  sep_ent.seq_type = ST_WORD|ST_SEP;
  sep_ent.nr_dic_ents = 0;
  sep_ent.flags = F_NONE;
}

/* Ⱦ�Ѥο����������Ѥο�������� */
static xchar
narrow_num_to_wide_num(xchar xc)
{
  if (xc > '9' || xc < '0') {
    return WIDE_0;
  }
  return narrow_wide_tab[(int)(xc - '0')];
}

/* ���Ѥο�������Ⱦ�Ѥο�������� */
static xchar
wide_num_to_narrow_num(xchar xc)
{
  int i;
  for (i = 0; i < 10; i++) {
    if (xc == narrow_wide_tab[i]) {
      return i + '0';
    }
  }
  return '0';
}
/*
 * ������������������Ѵ�����
 */
static xchar
get_kj_num(int n)
{
  if (n > 9 || n < 1) {
    return KJ_10;
  }
  return kj_num_tab[n];
}

/*
 * 4��ʬ�����������ʸ����Ȥ��Ƥ���������
 */
static void
compose_num_component(xstr *xs, long long num)
{
  int n[4],i;
  int a[4] = { 0 , KJ_10, KJ_100, KJ_1000};
  for (i = 0; i < 4; i++) {
    n[i] = num-(num/10)*10;
    num /= 10;
  }
  /* 10,100,1000�ΰ� */
  for (i = 3; i > 0; i--) {
    if (n[i] > 0) {
      if (n[i] > 1) {
	anthy_xstrappend(xs, get_kj_num(n[i]));
      }
      anthy_xstrappend(xs, a[i]);
    }
  }
  /* 1�ΰ� */
  if (n[0]) {
    anthy_xstrappend(xs, get_kj_num(n[0]));
  }
}

/** ��������ʸ������� */
static int
gen_kanji_num(long long num, xstr *dest)
{
  int i;
  int n[10];
  if (num < 1 || num >= 10000000000000000LL) {
    return -1;
  }
  /* 4�夺������n�ˤĤ�� */
  for (i = 0; i < 10; i ++) {
    n[i] = num-(num/10000)*10000;
    num = num/10000;
  }
  /**/
  dest->len = 0;
  dest->str = 0;
  /* ���ΰ̤�Ĥ��� */
  if (n[3]) {
    compose_num_component(dest, n[3]);
    anthy_xstrappend(dest, KJ_1000000000000);
  }
  /* ���ΰ̤�Ĥ��� */
  if (n[2]) {
    compose_num_component(dest, n[2]);
    anthy_xstrappend(dest, KJ_100000000);
  }
  /* ���ΰ̤�Ĥ��� */
  if (n[1]) {
    compose_num_component(dest, n[1]);
    anthy_xstrappend(dest, KJ_10000);
  }
  /**/
  compose_num_component(dest, n[0]);
  return 0;
}

static int
get_nr_num_ents(long long num)
{
  if (num > 0 && num < 10000000000000000LL) {
    if (num > 999) {
      /* ����ӥ�����(���Τޤ�)������ӥ�����(����Ⱦ�����ؤ�)��
	 ��������3����ڤ�(���ѡ�Ⱦ��) */
      return 5;
    }
    /* ����ӥ�����(���Τޤ�)������Ⱦ�����ؤ��������� */
    return 3;
  }
  /* ����ӥ�����(���Τޤ�)������Ⱦ�����ؤ� */
  return 2;
}


/*
 * �����Ĥι����Υ���ȥ꡼�����뤫
 */
int
anthy_get_nr_dic_ents_of_ext_ent(seq_ent_t se, xstr *xs)
{
  if (se == &unkseq_ent) {
    return 1;
  }
  if (anthy_get_xstr_type(xs) & (XCT_NUM|XCT_WIDENUM)) {
    long long num = anthy_xstrtoll(xs);
    return get_nr_num_ents(num);
  }
  return 0;
}

/* ʸ���������Ⱦ�Ѥ�򴹤��� */
static void
toggle_wide_narrow(xstr *dest, xstr *src)
{
  int f, i;
  dest->len = src->len;
  dest->str = anthy_xstr_dup_str(src);
  f = anthy_get_xstr_type(src) & XCT_WIDENUM;
  for (i = 0; i < dest->len; i++) {
    if (f) {
      dest->str[i] = wide_num_to_narrow_num(src->str[i]);
    } else {
      dest->str[i] = narrow_num_to_wide_num(src->str[i]);
    }
  }
}

/* 3��˶��ڤä��������������� */
static int
gen_separated_num(long long num, xstr *dest, int full)
{
  int width = 0, dot_count;
  long long tmp;
  int i, pos;

  if (num < 1000) {
    return -1;
  }

  /* ���������� */
  for (tmp = num; tmp != 0; tmp /= 10) {
    width ++;
  }
  /* ���ο� */
  dot_count = width / 3;
  /* ��Ǽ����Τ�ɬ�פ�ʸ������Ѱդ��� */
  dest->len = dot_count + width;
  dest->str = malloc(sizeof(xchar)*dest->len);

  /**/
  for (i = 0, pos = dest->len - 1; i < width; i++, pos --) {
    int n = num % 10;
    /* ����ޤ��ɲ� */
    if (i > 0 && (i % 3) == 0) {
      if (full) {
	dest->str[pos] = WIDE_COMMA;
      } else {
	dest->str[pos] = ',';
      }
      pos --;
    }
    if (full) {
      dest->str[pos] = narrow_wide_tab[n];
    } else {
      dest->str[pos] = 48 + n;
    }
    num /= 10;
  }
  return 0;
}

/*
 * nth�Ĥ�θ������Ф�
 */
int
anthy_get_nth_dic_ent_str_of_ext_ent(seq_ent_t se, xstr *xs,
				     int nth, xstr *dest)
{
  if (nth == 0) {
    /* ̵�Ѵ�ʸ���� */
    dest->len = xs->len;
    dest->str = anthy_xstr_dup_str(xs);
    return 0;
  }
  if (se == &unkseq_ent) {
    switch(nth) {
    case 1:
      /* ���ѡ�Ⱦ�ѤΥȥ��� */
      return 0;
    }
  }
  if (anthy_get_xstr_type(xs) & (XCT_NUM|XCT_WIDENUM)) {
    long long num = anthy_xstrtoll(xs);
    /* ������������ӥ�����������Ⱦ�����ؤ� */
    switch(nth) {
    case 1:
      /* ����Ⱦ�Ѥ����촹������� */
      toggle_wide_narrow(dest, xs);
      return 0;
    case 2:
      /* ������ */
      if (!gen_kanji_num(num, dest)) {
	return 0;
      }
      /* break̵�� */
    case 3:
      /* 3��Ƕ��ڤä����� */
      if (!gen_separated_num(num, dest, 0)) {
	return 0;
      }
      /* break̵�� */
    case 4:
      /* 3��Ƕ��ڤä�����(����) */
      if (!gen_separated_num(num, dest, 1)) {
	return 0;
      }
      /* break̵�� */
    default:
      break;
    }
    return -1;
  }
  return 0;
}

int
anthy_get_ext_seq_ent_indep(struct seq_ent *se)
{
  if (se == &num_ent || se == &unkseq_ent) {
    return 1;
  }
  return 0;
}

int
anthy_get_ext_seq_ent_ct(struct seq_ent *se, int pos, int ct)
{
  if (anthy_get_ext_seq_ent_pos(se, pos) && ct == CT_NONE) {
    return 10;
  }
  return 0;
}

int
anthy_get_ext_seq_ent_pos(struct seq_ent *se, int pos)
{
  if (se == &num_ent && pos == POS_NOUN) {
    return 10;
  }
  if ((se->seq_type & ST_UNKSEQ) && pos == POS_NOUN) {
    return 10;
  }
  return 0;
}

/*
 * ����ˤΤäƤ��ʤ��������󥹤����
 */
seq_ent_t
anthy_get_ext_seq_ent_from_xstr(xstr *x)
{
  int t = anthy_get_xstr_type(x);

  if (t & (XCT_NUM | XCT_WIDENUM)) {
    return &num_ent;
  }
  /* �ѿ��ʤ�unkseq */
  if (t & XCT_ASCII) {
    return &unkseq_ent;
  }
  if (t & XCT_KANJI) {
    return &unkseq_ent;
  }
  if (t & XCT_KATA) {
    return &unkseq_ent;
  }
  if (x->len == 1) {
    /* ����ˤΤäƤʤ���1ʸ���ʤ饻�ѥ졼�� */
    return &sep_ent;
  }
  return 0;
}

int
anthy_get_nth_dic_ent_wtype_of_ext_ent(xstr *xs, int nth,
				       wtype_t *wt)
{
  (void)nth;
  if (anthy_get_xstr_type(xs) & XCT_NUM) {
    *wt = wt_num;
    return 0;
  }
  return -1;
}

int
anthy_get_ext_seq_ent_wtype(struct seq_ent *se, wtype_t w)
{
  if (se == &num_ent) {
    if (anthy_wtypecmp(w, wt_num)) {
      return 10;
    }
    return 0;
  }
  if (anthy_wtype_get_pos(w) == POS_NOUN &&
      anthy_wtype_get_cos(w) == COS_NONE &&
      anthy_wtype_get_scos(w) == SCOS_NONE) {
    return 10;
  }
  return 0;
}

void
anthy_init_ext_ent(void)
{
  init_ext_ents();
  anthy_init_wtype_by_name("����", &wt_num);
}