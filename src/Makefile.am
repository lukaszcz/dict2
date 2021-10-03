bin_PROGRAMS = dict2
dict2_SOURCES = dict2.c dictionary.c gui.c utils.c file.c options.c cache.c \
	wforms.c rbtest.c rbtree.c strutils.c list.c hash_32a.c hash_32.c hashtable.c \
	hashtable_itr.c

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
dict2_LDFLAGS = $(all_libraries) -lm
noinst_HEADERS = dictionary.h utils.h file.h limits.h gui.h options.h paths.h \
		cache.h wforms.h strutils.h list.h fnv.h hashtable.h hashtable_itr.h \
	hashtable_private.h rbtree.h

dict2_LDADD = $(GTK_LIBS)
