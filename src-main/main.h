#ifndef _main_h_included_
#define _main_h_included_

#include <xstr.h>
#include <dic.h>
#include <splitter.h>
#include <segment.h>
#include <ordering.h>

/** Anthyの変換コンテキスト
 * 変換中の文字列などが入っている
 */
struct anthy_context {
  /** コンテキストの持つ文字列 */
  xstr str;
  /** 文節のリスト */
  struct segment_list seg_list;
  /** 辞書セッション */
  dic_session_t dic_session;
  /** splitterの情報 */
  struct splitter_context split_info;
  /** 候補の並び替え情報 */
  struct ordering_context_wrapper ordering_info;
  /** エンコーディング */
  int encoding;
};


/* context.c */
void anthy_init_contexts(void);
void anthy_quit_contexts(void);
void anthy_init_personality(void);
void anthy_quit_personality(void);
int anthy_do_set_personality(const char *id);
struct anthy_context *anthy_do_create_context(int);
int anthy_do_context_set_str(struct anthy_context *c, xstr *x);
void anthy_do_reset_context(struct anthy_context *c);
void anthy_do_release_context(struct anthy_context *c);

void anthy_do_resize_segment(struct anthy_context *c,int nth,int resize);

/* for debug */
void anthy_do_print_context(struct anthy_context *c, int encoding);


#endif
/* なるべく階層をフラットにするよろし */
