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

using namespace std::string_literals;


// Required symbol.
int plugin_is_GPL_compatible;


#define F << std::flush

namespace {

  FILE* outfile = stderr;


  template<typename _CharT, typename _Traits>
  std::basic_ostream<_CharT, _Traits>&
  operator<<(std::basic_ostream<_CharT, _Traits>& s, unsigned __int128 n)
  {
    char buf[64];
    auto cp = &buf[sizeof(buf) - 1];
    *cp = '\0';
    if (n == 0)
      *--cp = '0';
    else
      while (n != 0) {
        *--cp = '0' + (n % 10);
        n /= 10;
      }
    s << cp;
    return s;
  }


  plugin_info my_plugin_info = {
    VERSION,
    "AST print plugin"
  };


  void print_decl (tree decl)
  {
    auto tc(TREE_CODE(decl));
    tree id(DECL_NAME(decl));
    const char* name(id ? IDENTIFIER_POINTER(id) : "<unnamed>");
   
   fprintf(outfile, "%s %s at %s:%d\n",
           get_tree_code_name(tc), name, DECL_SOURCE_FILE(decl), DECL_SOURCE_LINE(decl));

    // if (tc == FUNCTION_DECL)
    //   std::cerr << "is function\n";
  }

  void print_type(tree t)
  {
    auto code = TREE_CODE(t);
    std::cerr << ' ' << TREE_CODE_CLASS_STRING(TREE_CODE_CLASS(code)) F;
    if (code == VOID_TYPE)
      std::cerr << " void\n";
    else if (POINTER_TYPE_P(t))
      std::cerr << " pointer\n";
    else if (code == BOOLEAN_TYPE)
      std::cerr << " boolean\n";
    else if (code == INTEGER_TYPE)
      std::cerr << " integer " << TYPE_PRECISION(t) << std::endl;
    else if (code == REAL_TYPE)
      std::cerr << " real " << TYPE_PRECISION(t) << std::endl;
    else
      std::cerr << " ??? " << code << std::endl;
  }

  void print_node(tree t, tree_code code, unsigned level)
  {
    for (auto i = 0u; i < level; ++i)
      std::cerr << "  " F;
    std::cerr << get_tree_code_name(code) F;
    switch (code) {
    case INTEGER_CST:
      if (tree_fits_shwi_p(t)) {
        unsigned HOST_WIDE_INT n = tree_to_shwi(t);
        if (TREE_CODE(TREE_TYPE(t)) == INTEGER_TYPE && 8*sizeof(decltype(n)) >= TYPE_PRECISION(TREE_TYPE(t))) {
          auto mask = ~(decltype(n))0;
          mask >>= 8*sizeof(mask) - TYPE_PRECISION(TREE_TYPE(t));
          n &= mask;
        }
        std::cerr << " = " << n F;
      } else
        std::cerr << " = large integer" F;
      break;
    case PARM_DECL:
      std::cerr << " = " << IDENTIFIER_POINTER(DECL_NAME(t)) F;
      break;
    case HANDLER:
      return;
      break;
    default:
      break;
    }
    print_type(TREE_TYPE(t));
  }

  void my_walk_fields(tree t, unsigned level, std::unordered_set<tree>& visited)
  {

  }


  bool my_walk(tree t, unsigned level, std::unordered_set<tree>& visited)
  {
    dump_info di = {};
    di.stream = ::stderr;

    return lang_hooks.tree_dump.dump_tree(&di, t);


    if (t == nullptr)
      return false;

    if (visited.find(t) != visited.end()) {
      for (auto i = 0u; i < level; ++i)
        std::cerr << "  ";
      std::cerr << "CYCLE: t\n";
      return false;
    }

    visited.insert(t);

    auto code = TREE_CODE(t);
    print_node(t, code, level);

    switch (code) {
      constructor_elt* ce;
      tree decl;
    case ERROR_MARK:
    case IDENTIFIER_NODE:
    case INTEGER_CST:
    case REAL_CST:
    case FIXED_CST:
    case VECTOR_CST:
    case STRING_CST:
    case BLOCK:
    case PLACEHOLDER_EXPR:
    case SSA_NAME:
    case FIELD_DECL:
    case RESULT_DECL:
      // Nothing else to do.
      break;
    case TREE_LIST:
      my_walk(TREE_VALUE(t), level + 1, visited);
      my_walk(TREE_CHAIN(t), level, visited);
      break;
    case TREE_VEC:
      for (auto i = TREE_VEC_LENGTH(t) - 1; i >= 0; --i)
        my_walk(TREE_VEC_ELT(t, i), level, visited);
      break;
    case COMPLEX_CST:
      my_walk(TREE_REALPART(t), level + 1, visited);
      my_walk(TREE_IMAGPART(t), level + 1, visited);
      break;
    case CONSTRUCTOR:
      for (unsigned HOST_WIDE_INT i = 0; vec_safe_iterate(CONSTRUCTOR_ELTS(t), i, &ce); ++i)
        my_walk(ce->value, level + 1, visited);
      break;
    case SAVE_EXPR:
      my_walk(TREE_OPERAND(t, 0), level + 1, visited);
      break;
    case BIND_EXPR:
      for (decl = BIND_EXPR_VARS(t); decl; decl = DECL_CHAIN(decl)) {
        print_type(TREE_TYPE(decl));
        // if (my_walk(DECL_INITIAL(t), level + 1, visited)) {
        //   if (DECL_SIZE(t) != nullptr)
        //     print_node(DECL_SIZE(t), TREE_CODE(DECL_SIZE(t)), level + 1);
        //   if (DECL_SIZE_UNIT(t) != nullptr)
        //     print_node(DECL_SIZE_UNIT(t), TREE_CODE(DECL_SIZE_UNIT(t)), level + 1);
        // }
      }
      my_walk(BIND_EXPR_BODY(t), level + 1, visited);
      break;
    case STATEMENT_LIST:
      for (auto i = tsi_start(t); ! tsi_end_p(i); tsi_next(&i))
        my_walk(*tsi_stmt_ptr(i), level + 1, visited);
      break;
    case TARGET_EXPR:
      for (auto i = 0; i <= (TREE_OPERAND(t, 3) == TREE_OPERAND(t, 1) ? 2 : 3); ++i)
        my_walk(TREE_OPERAND(t, i), level + 1, visited);
      break;
    case OMP_CLAUSE:
      abort();
      break;  
    case DECL_EXPR:
      if (TREE_CODE(DECL_EXPR_DECL(t)) == TYPE_DECL) {
        auto t2 = TREE_TYPE(DECL_EXPR_DECL(t));
        auto code2 =  TREE_CODE(t2);
        if (code2 == ERROR_MARK)
          break;
        print_node(t2, code2, level + 1);
        if (! POINTER_TYPE_P(t2))
          my_walk_fields(t2, level + 1, visited);
        if (RECORD_OR_UNION_TYPE_P(t2)) {
          for (auto field = TYPE_FIELDS(t2); field; field = DECL_CHAIN(field))
            if (TREE_CODE(field) == FIELD_DECL) {
              my_walk(DECL_FIELD_OFFSET(field), level + 1, visited) &&
              my_walk(DECL_SIZE(field), level + 1, visited) &&
              my_walk(DECL_SIZE_UNIT(field), level + 1, visited);
              if (TREE_CODE(t2) == QUAL_UNION_TYPE)
                my_walk(DECL_QUALIFIER(field), level + 1, visited);
            }
        } else if (TREE_CODE(t2) == BOOLEAN_TYPE ||
                   TREE_CODE(t2) == ENUMERAL_TYPE ||
                   TREE_CODE(t2) == INTEGER_TYPE ||
                   TREE_CODE(t2) == FIXED_POINT_TYPE ||
                   TREE_CODE(t2) == REAL_TYPE) {
          my_walk(TYPE_MIN_VALUE(t2), level + 1, visited);
          my_walk(TYPE_MAX_VALUE(t2), level + 1, visited);
        }

        my_walk(TYPE_SIZE(t2), level + 1, visited);
        my_walk(TYPE_SIZE_UNIT(t2), level + 1, visited);
        break;
      }
    case HANDLER:
      if (HANDLER_PARMS(t))
        my_walk(HANDLER_PARMS(t), level + 1, visited);
      my_walk(HANDLER_BODY(t), level+1, visited);
      break;
    default:
      if (IS_EXPR_CODE_CLASS(TREE_CODE_CLASS(code))) {
        auto len = TREE_OPERAND_LENGTH(t);
        for (auto i = 0; i < len; ++i)
          my_walk(TREE_OPERAND(t, i), level + 1, visited);
      } else if (TYPE_P(t))
        my_walk_fields(t, level + 1, visited);
      break;
    }

    visited.erase(t);

    return true;
  }

  // For PLUGIN_PRE_GENERICIZE.
  void pre_genericize_cb(void* gcc_data, void* user_data)
  {
    tree t(static_cast<tree>(gcc_data));
    assert(TREE_CODE(t) == FUNCTION_DECL);
    print_decl(t);
    dump_node(DECL_SAVED_TREE(t), 0, outfile);
    // std::unordered_set<tree> visited;
    // visited.insert(t);
    // my_walk(DECL_SAVED_TREE(t), 0, visited);
  }

#if 0
  // For PLUGIN_FINISH_DECL.
  void finish_decl_cb(void* gcc_data, void* user_data)
  {
    std::cerr << "finish_decl_cb\n";
    tree t(static_cast<tree>(gcc_data));
    print_decl(t);
  }
#endif

  void finish_cb(void* gcc_data, void* data)
  {
    fputs("finish compilation\n", outfile);
    fclose(outfile);
    outfile = nullptr;
  }

} // anonymous namespace


int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
  if (! plugin_default_version_check(version, &gcc_version))
    return 1;

  for (int i = 0; i < plugin_info->argc; i++)
    if (plugin_info->argv[i].key == "out"s) {
      outfile = fopen(plugin_info->argv[i].value, "w");
      if (outfile == nullptr) {
        std::cerr << "cannot open " << plugin_info->argv[i].value << std::endl;
        exit(1);
      }
    }

  register_callback(plugin_info->base_name, PLUGIN_INFO, nullptr, &my_plugin_info);

  // Register all the callback functions.
  register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE, pre_genericize_cb, nullptr);
  // register_callback(plugin_info->base_name, PLUGIN_FINISH_DECL, finish_decl_cb, nullptr);
  register_callback(plugin_info->base_name, PLUGIN_FINISH, finish_cb, nullptr);

  // Add more here if necessary.

  return 0;
}
