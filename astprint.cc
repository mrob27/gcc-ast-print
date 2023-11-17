#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <gcc-plugin.h>
#include <tree.h>
#include <cp/cp-tree.h>
#include <plugin-version.h>
#include <tree-iterator.h>
#include <langhooks.h>
#include <tree-dump.h>
#include <c-family/c-pragma.h>

#include "tree-dump.c"

using namespace std::string_literals;
using namespace std;

// Required symbol.
int plugin_is_GPL_compatible;

namespace {
  FILE* outfile = stdout;

  plugin_info my_plugin_info = {
    VERSION,
    "AST print plugin"
  };

  void print_decl (tree decl)
  {
    // Find out what type of tree we are dealing with.
    // See gcc.gnu.org/onlinedocs/gccint/Tree-overview.html
    auto tc(TREE_CODE(decl));

    // Get a pointer to the object's declared name.
    // gcc.gnu.org/onlinedocs/gccint/Function-Basics.html
    tree id(DECL_NAME(decl));

    // Get the actual function name
    const char* name(id ? IDENTIFIER_POINTER(id) : "<unnamed>");

    fprintf(outfile, "%s %s at %s:%d\n", get_tree_code_name(tc), name,
                            DECL_SOURCE_FILE(decl), DECL_SOURCE_LINE(decl));

    if (tc == FUNCTION_DECL) {
      cerr << "# astprint.cc: decl '" << name << "' is a function\n";
    }
  }

  // For PLUGIN_PRE_GENERICIZE.
  void pre_genericize_cb(void* gcc_data, void* user_data)
  {
    tree t(static_cast<tree>(gcc_data));
    assert(TREE_CODE(t) == FUNCTION_DECL);
    print_decl(t);

    // Formerly we did our own tree walk. Now we just use dump_node.
    // Source for this function is at
    // github.com/gcc-mirror/gcc/blob/master/gcc/tree-dump.c
    rm_dump_node(DECL_SAVED_TREE(t), TDF_NONE, outfile);

    // std::unordered_set<tree> visited;
    // visited.insert(t);
    // my_walk(DECL_SAVED_TREE(t), 0, visited);
  }

  void finish_cb(void* gcc_data, void* data)
  {
    fputs("finish compilation\n", outfile);
    fclose(outfile);
    outfile = nullptr;
  }

} // anonymous namespace


int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
  if (! plugin_default_version_check(version, &gcc_version)) {
    cerr << "# astprint.cc: plugin_init failed version_check" << endl;
    return 1;
  }

  // Standard plugin argument parsing, per gcc.gnu.org/wiki/GCC_PluginAPI :
  //
  //   Loading Plug-ins
  //
  //   Linking GCC with -ld -rdynamic on supported platforms. Plugins
  //   are to be loaded by specifying with
  //
  //       -fplugin=/path/to/NAME.so
  //         -fplugin-arg-NAME-<key1>[=<value1>]
  //         -fplugin-arg-NAME-<key2>[=<value2>]
  //
  //   The plugin arguments are parsed by GCC and passed to respective
  //   plugins as key-value pairs. Multiple plugins can be invoked by
  //   specifying multiple -fplugin arguments.
  //
  cerr << "# astprint.cc: argc == " << plugin_info->argc << endl;
  for (int i = 0; i < plugin_info->argc; i++) {
    cerr << "# astprint.cc: key '" << plugin_info->argv[i].key << "' val '"
                                    << plugin_info->argv[i].value << endl;
    if (plugin_info->argv[i].key == "out"s) {
      outfile = fopen(plugin_info->argv[i].value, "w");
      if (outfile == nullptr) {
        cerr << "cannot open " << plugin_info->argv[i].value << endl;
        exit(1);
      } else {
        cerr << "# astprint.cc: opened " << plugin_info->argv[i].value << endl;
      }
    }
  }

  // Register all the callback functions.
  register_callback(plugin_info->base_name,
                       PLUGIN_INFO, nullptr, &my_plugin_info);

  register_callback(plugin_info->base_name,
                       PLUGIN_PRE_GENERICIZE, pre_genericize_cb, nullptr);

  register_callback(plugin_info->base_name,
                       PLUGIN_FINISH, finish_cb, nullptr);

  // Add more here if necessary.

  return 0;
}
