/* Copyright 2004 by Hans Reiser, licensing governed by
 * reiser4/README */

/* Directory fibrations */

/*
 * Suppose we have a directory tree with sources of some project. During
 * compilation .o files are created within this tree. This makes access
 * to the original source files less efficient, because source files are
 * now "diluted" by object files: default directory plugin uses prefix
 * of a file name as a part of the key for directory entry (and this
 * part is also inherited by the key of file body). This means that
 * foo.o will be located close to foo.c and foo.h in the tree.
 *
 * To avoid this effect directory plugin fill highest 7 (unused
 * originally) bits of the second component of the directory entry key
 * by bit-pattern depending on the file name (see
 * fs/reiser4/kassign.c:build_entry_key_common()). These bits are called
 * "fibre". Fibre of the file name key is inherited by key of stat data
 * and keys of file body (in the case of REISER4_LARGE_KEY).
 *
 * Fibre for a given file is chosen by per-directory fibration
 * plugin. Names within given fibre are ordered lexicographically.
 */

#include "../debug.h"
#include "plugin_header.h"
#include "plugin.h"
#include "../super.h"

#include <linux/types.h>

static const int fibre_shift = 57;

#define FIBRE_NO(n) (((__u64)(n)) << fibre_shift)

/*
 * Trivial fibration: all files of directory are just ordered
 * lexicographically.
 */
static __u64 fibre_trivial(const struct inode *dir, const char *name, int len)
{
	return FIBRE_NO(0);
}

/*
 * dot-o fibration: place .o files after all others.
 */
static __u64 fibre_dot_o(const struct inode *dir, const char *name, int len)
{
	/* special treatment for .*\.o */
	if (len > 2 && name[len - 1] == 'o' && name[len - 2] == '.')
		return FIBRE_NO(1);
	else
		return FIBRE_NO(0);
}

/*
 * ext.1 fibration: subdivide directory into 128 fibrations one for each
 * 7bit extension character (file "foo.h" goes into fibre "h"), plus
 * default fibre for the rest.
 */
static __u64 fibre_ext_1(const struct inode *dir, const char *name, int len)
{
	if (len > 2 && name[len - 2] == '.')
		return FIBRE_NO(name[len - 1]);
	else
		return FIBRE_NO(0);
}

/*
 * ext.3 fibration: try to separate files with different 3-character
 * extensions from each other.
 */
static __u64 fibre_ext_3(const struct inode *dir, const char *name, int len)
{
	if (len > 4 && name[len - 4] == '.')
		return FIBRE_NO(name[len - 3] + name[len - 2] + name[len - 1]);
	else
		return FIBRE_NO(0);
}

/* fibration plugins */
fibration_plugin fibration_plugins[LAST_FIBRATION_ID] = {
	[FIBRATION_LEXICOGRAPHIC] = {
		.h = {
			.type_id = REISER4_FIBRATION_PLUGIN_TYPE,
			.id = FIBRATION_LEXICOGRAPHIC,
			.pops = NULL,
			.label = "lexicographic",
			.desc = "no fibration",
			.linkage = TYPE_SAFE_LIST_LINK_ZERO
		},
		.fibre = fibre_trivial
	},
	[FIBRATION_DOT_O] = {
		.h = {
			.type_id = REISER4_FIBRATION_PLUGIN_TYPE,
			.id = FIBRATION_DOT_O,
			.pops = NULL,
			.label = "dot-o",
			.desc = "fibrate .o files separately",
			.linkage = TYPE_SAFE_LIST_LINK_ZERO
		},
		.fibre = fibre_dot_o
	},
	[FIBRATION_EXT_1] = {
		.h = {
			.type_id = REISER4_FIBRATION_PLUGIN_TYPE,
			.id = FIBRATION_EXT_1,
			.pops = NULL,
			.label = "ext-1",
			.desc = "fibrate file by single character extension",
			.linkage = TYPE_SAFE_LIST_LINK_ZERO
		},
		.fibre = fibre_ext_1
	},
	[FIBRATION_EXT_3] = {
		.h = {
			.type_id = REISER4_FIBRATION_PLUGIN_TYPE,
			.id = FIBRATION_EXT_3,
			.pops = NULL,
			.label = "ext-3",
			.desc = "fibrate file by three character extension",
			.linkage = TYPE_SAFE_LIST_LINK_ZERO
		},
		.fibre = fibre_ext_3
	}
};

/* Make Linus happy.
   Local variables:
   c-indentation-style: "K&R"
   mode-name: "LC"
   c-basic-offset: 8
   tab-width: 8
   fill-column: 120
   End:
*/