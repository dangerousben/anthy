# adding Id cause confusion
# Autoconf �θߴ��ޥ���
# ������ Autoconf (2.5x) �δĶ��Ǥ� ifdef �� define ��
# undefine ����Ƥ���Τǡ������̵�뤵���
ifdef([AC_HELP_STRING], [],
  [define([AC_HELP_STRING], [builtin([format], [  %-22s  %s], [$1], [$2])])])
