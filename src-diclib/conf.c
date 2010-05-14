/*
 * Anthy������Υǡ����١���
 * conf_init�����ꤵ����ѿ���conf_override�����ꤵ���
 * �ѿ��δط������
 *
 * Copyright (C) 2000-2004 TABATA Yusuke
 */
#include <unistd.h>
#include <pwd.h>
#include <time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <alloc.h>
#include <conf.h>
#include <logger.h>

#include <config.h>

/** ������ѿ����� */
struct val_ent {
  /** �ѿ�̾ */
  const char *var;
  /** �� */
  const char *val;
  /** �ꥹ�Ȥμ����� */
  struct val_ent *next;
};

static struct val_ent *ent_list;
/** ������Ѥߥե饰 */
static int confIsInit;
static allocator val_ent_ator;

static void
val_ent_dtor(void *p)
{
  struct val_ent *v = p;
  free((void *)v->var);
  if (v->val) {
    free((void *)v->val);
  }
}

/** �ѿ�̾���б�����val_ent��������� */
static struct val_ent *
find_val_ent(const char *v)
{
  struct val_ent *e;
  for (e = ent_list; e; e = e->next) {
    if(!strcmp(v, e->var)) {
      return e;
    }
  }
  e = malloc(sizeof(struct val_ent));
  e->var = strdup(v);
  e->val = 0;
  e->next = ent_list;
  ent_list = e;
  return e;
}

static void
add_val(const char *var, const char *val)
{
  struct val_ent *e;
  e = find_val_ent(var);
  if (e->val) {
    free((void *)e->val);
  }
  e->val = strdup(val);
}

static void
read_conf_file(void)
{
  const char *fn;
  FILE *fp;
  char buf[256];
  fn = anthy_conf_get_str("CONFFILE");
  fp = fopen(fn, "r");
  if (!fp){
    anthy_log(0, "Failed to open %s\n", fn);
    return ;
  }
  while(fgets(buf, 256, fp)) {
    if (buf[0] != '#') {
      char var[256], val[256];
      if (sscanf(buf, "%s %s", var, val) == 2){
	add_val(var, val);
      }
    }
  }
  fclose(fp);
}

void
anthy_do_conf_override(const char *var, const char *val)
{
  if (!strcmp(var,"CONFFILE")) {
    add_val(var, val);
    anthy_do_conf_init();
  }else{
    anthy_do_conf_init();
    add_val(var, val);
  }
}

/* ��ˡ����ʥ��å����ID����ݤ��� */
#define SID_FORMAT	"%s-%08x-%05d" /* HOST-TIME-PID */
#define MAX_SID_LEN  	(MAX_HOSTNAME+8+5+2)
#define MAX_HOSTNAME 	64

static void
alloc_session_id(void)
{
  time_t t;
  pid_t pid;
  char hn[MAX_HOSTNAME];
  char sid[MAX_SID_LEN];
  t = time(0);
  pid = getpid();
  gethostname(hn, MAX_HOSTNAME);
  hn[MAX_HOSTNAME-1] = '\0';
  sprintf(sid, SID_FORMAT, hn, (unsigned int)t & 0xffffffff, pid & 0xffff);
  add_val("SESSION-ID", sid);
}

void
anthy_do_conf_init(void)
{

  if (!confIsInit) {
    const char *fn;
    struct passwd *pw;
    val_ent_ator = anthy_create_allocator(sizeof(struct val_ent), val_ent_dtor);
    /*�ǥե���Ȥ��ͤ����ꤹ�롣*/
    add_val("VERSION", VERSION);
    fn = anthy_conf_get_str("CONFFILE");
    if (!fn){
      add_val("CONFFILE", CONF_DIR"/anthy-conf");
    }
    pw = getpwuid(getuid());
    add_val("HOME", pw->pw_dir);
    alloc_session_id();
    read_conf_file();
    confIsInit = 1;
  }
}

void
anthy_conf_free(void)
{
  struct val_ent *e, *next;
  for (e = ent_list; e; e = next) {
    free((char *)e->var);
    free((char *)e->val);
    next = e->next;
    free(e);
  }
  ent_list = NULL;
  confIsInit = 0;
}

const char *
anthy_conf_get_str(const char *var)
{
  struct val_ent *e;
  e = find_val_ent(var);
  return e->val;
}
