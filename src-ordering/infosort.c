/*
 * ʸ��ι�¤metaword�򥽡��Ȥ���
 *
 * ʸ����Ф���ʣ���ι�¤�θ���򥽡��Ȥ���
 *
 */
#include <stdlib.h>

#include <segment.h>
#include <ordering.h>
#include <splitter.h>
#include "sorter.h"


static int
metaword_compare_func(const void *p1, const void *p2)
{
  const struct meta_word * const *s1 = p1;
  const struct meta_word * const *s2 = p2;
  return (*s2)->score - (*s1)->score;
}

void
anthy_sort_metaword(struct segment_list *seg)
{
  int i;
  for (i = 0; i < seg->nr_segments; i++) {
    struct seg_ent *se = anthy_get_nth_segment(seg, i);
    qsort(se->mw_array, se->nr_metaword, sizeof(struct meta_word *),
	  metaword_compare_func);
  }
}
