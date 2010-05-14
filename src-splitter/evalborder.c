/*
 * ʸ��ζ����򸡽Ф��롣
 * ʸ���extent��Ϣ���ɾ�����ơ����������⤯�ʤ�褦��extent������Ǥ�����
 *
 * metaword������ˤ���ͥ��õ����Ԥ�
 * �����ΥΡ��ɤ�Astar_node��search_stat�������ѤΥ��塼
 *
 * anthy_eval_border() �ǻ��ꤵ�줿�ΰ��ʸ���ʬ�䤹��
 *
 * Funded by IPA̤Ƨ���եȥ�������¤���� 2001 10/29
 * Copyright (C) 2000-2003 TABATA Yusuke, UGAWA Tomoharu
 */
#include <stdio.h>
#include <stdlib.h>

#include <alloc.h>
#include <splitter.h>
#include "wordborder.h"

/* ʸ�ᶭ���������­���ʤ�ʸ������� */
static void
seg_constraint_check_all(struct splitter_context *sc,
			 int from, int to,
			 int border)
{
  int i;
  for (i = from; i < to; i++) {
    struct word_list *wl;
    for (wl = sc->word_split_info->cnode[i].wl;
	 wl; wl = wl->next) {
      if (i < border && border < i + wl->len) {
	wl->can_use = ng;
	continue;
      } 
      if (from != border) {
	if (i == from && i + wl->len != border) {
	  wl->can_use = ng;
	  continue;
	}
      }
      wl->can_use = ok;
    }
  }
}

static int
border_check(struct meta_word* mw,
		 int from,
		 int border)
{
  if (from != border) {
    if (mw->from == from && mw->from + mw->len != border) {
      return 0;
    }
  }
  if (from < mw->from && mw->from < border && border <= mw->from + mw->len) {
    return 0;
  }
  return 1;
}

/*
 * �Ƶ�Ū��metaword�����Ѳ�ǽ�������å�����
 */
static void
metaword_constraint_check(struct splitter_context *sc,
			  struct meta_word *mw,
			  int from, 
			  int border)
{
  if (mw->can_use != unchecked) {
    return ;
  }
  switch(anthy_metaword_type_tab[mw->type].check){
  case MW_CHECK_WL_STR:
    if (!mw->wl) {
      if (border_check(mw, from, border)) {
	mw->can_use = ok;	
      } else {
	mw->can_use = ng;
      }
      return;
    }
    /* break̵�� */
  case MW_CHECK_WL_SINGLE:
    {
      if (!mw->wl || mw->wl->can_use == ok) {
	mw->can_use = ok;
      } else {
	mw->can_use = ng;
      }
    }
    break;
  case MW_CHECK_WL_WRAP:
    metaword_constraint_check(sc, mw->mw1, from, border);
    mw->can_use = mw->mw1->can_use;
    break;
  case MW_CHECK_BORDER:
    if (mw->mw1->from + mw->mw1->len == border) {
      /* ���礦�ɶ��ܤ˥ޡ��������äƤ� */
      mw->can_use = ng;
      break;
    }
    /* break̵�� */
  case MW_CHECK_PAIR:
    metaword_constraint_check(sc, mw->mw1, from, border);
    metaword_constraint_check(sc, mw->mw2, from, border);
    if (mw->mw1->can_use == ok && mw->mw2->can_use == ok) {
      mw->can_use = ok;
    }
    break;
  case MW_CHECK_COMPOUND:
    {
      struct meta_word* itr = mw;
      mw->can_use = ok;
      
      for (; itr && (itr->type == MW_COMPOUND_HEAD || itr->type == MW_COMPOUND); itr = itr->mw2) {
	struct meta_word* mw1 = itr->mw1;
	if (!border_check(mw1, from, border)) {
	  mw->can_use = ng;
	  break;
	}
      }
    }
    break;
  case MW_CHECK_OCHAIRE:
    {
      struct meta_word* mw1;
      if (border_check(mw, from, border)) {
	for (mw1 = mw; mw1; mw1 = mw1->mw1) {
	  mw1->can_use = ok;
	}
      } else {
	for (mw1 = mw; mw1; mw1 = mw1->mw1) {
	  mw1->can_use = ng;
	}	
      }
    }
    break;
  case MW_CHECK_NONE:
    break;
  default:
    printf("try to check unknown type of metaword (%d).\n", mw->type);
  }
}

/*
 * word_list�ξ������metaword�����ѤǤ��뤫�����å�����
 */
static void
metaword_constraint_check_all(struct splitter_context *sc,
			      int from, int to,
			      int border)
{
  int i;
  struct word_split_info_cache *info;
  info = sc->word_split_info;

  /* �ޤ�unchecked�ˤ��� */
  for (i = from; i < to; i ++) {
    struct meta_word *mw;
    for (mw = info->cnode[i].mw;
	 mw; mw = mw->next) {
      mw->can_use = unchecked;
    }
  }

  /* ���˹������줿metaword�ˤĤ��ƥ����å� */
  for (i = from; i < to; i ++) {
    struct meta_word *mw;
    for (mw = info->cnode[i].mw; mw; mw = mw->next) {
      metaword_constraint_check(sc, mw, from, border);
    }
  }
}

/*
 * ��������ʸ�ᶭ����ޡ�������
 */
void
anthy_eval_border(struct splitter_context *sc, int from, int from2, int to)
{
  /* ʸ�����Τ����Ȥ����ΤΤ����� */
  seg_constraint_check_all(sc, from, to, from2);
  metaword_constraint_check_all(sc, from, to, from2);

  /* extent��ɾ������ */
  anthy_hmm(sc, from, to);
}
