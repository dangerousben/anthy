require 'mkmf'
$LDFLAGS += ' -lanthydic -lanthy'
$objs = []
$objs.push "anthy_swig_wrap.o"
create_makefile('anthy')
