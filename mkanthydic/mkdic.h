#ifndef _mkdic_h_included_
#define _mkdic_h_included_

#include <xstr.h>

/** ñ�� */
struct word_entry {
  /** �ʻ�̾ */
  char *wt;
  /** ���� */
  int freq;
  /** ñ�� */
  char *word;
  /** ���ե��å� */
  int offset;
};

/** �����ɤ� */
struct yomi_entry {
  /* �����ɤ�*/
  xstr *index_xstr;
  /* ����ե�������Υڡ�����Υ��ե��å� */
  int offset;
  /* �ƥ���ȥ� */
  int nr_entries;
  struct word_entry *entries;
  struct yomi_entry *next;
  struct yomi_entry *hash_next;
};

#define YOMI_HASH 1024

/* �������� */
struct yomi_entry_list {
  struct yomi_entry *head;
  int nr_entries;
  struct yomi_entry *hash[YOMI_HASH];
  struct yomi_entry **ye_array;
};

/* ����hash */
struct versatile_hash {
  char *buf;
  FILE *fp;
};

/* ����񤭽Ф��Ѥ���� */
void write_nl(FILE *fp, int i);

/* ���㼭����� */
struct uc_dict *read_uc_file(const char *fn, struct yomi_entry *ye);
void make_ucdic(FILE *out,  struct uc_dict *uc);
/**/

void fill_uc_to_hash(struct versatile_hash *vh, struct uc_dict *dict);

#endif
