/*
 * ���Ѥ�����ե�����ѡ���
 */
#ifndef _ruleparser_h_included_
#define _ruleparser_h_included_

/*
 * �ե�����̾��'/'�ǻϤޤäƤ�������Хѥ�
 * �ե�����̾��NULL�ʤ��ɸ������
 * �����Ǥʤ���С�ANTHYDIR��Υե�����򳫤���
 */
int anthy_open_file(const char *fn);
void anthy_close_file(void);
int anthy_read_line(char ***tokens, int *nr);
int anthy_get_line_number(void);
void anthy_free_line(void);

#endif
