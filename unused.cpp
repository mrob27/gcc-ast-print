
#define F << std::flush

  // Define << to print 128-bit integers to output streams
  template<typename _CharT, typename _Traits>
  std::basic_ostream<_CharT, _Traits>&
  operator<<(std::basic_ostream<_CharT, _Traits>& s, unsigned __int128 n)
  {
    char buf[64]; // C string, temporary buffer for converting the number
    auto cp = &buf[sizeof(buf) - 1];
    // We build up the string in reverse order, so we write into the buffer
    // from the end and work downwards
    *cp = '\0';
    if (n == 0)
      *--cp = '0';
    else {
      // Take the least significant digit, convert to char, add to buffer
      while (n != 0) {
        *--cp = '0' + (n % 10);
        n /= 10;
      }
    }
    // send the string to the output stream
    s << cp;
    // we must return the output stream so that this overload of << can
    // be chained with other overloads of << in the normal way.
    return s;
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
  } // end of print.type

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
  } // end of print.node


  bool my_walk(tree t, unsigned level, std::unordered_set<tree>& visited)
  {
    dump_info di = {};
    di.stream = ::stderr;
    return false;
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
      } else if (TYPE_P(t)) {
        my_walk_fields(t, level + 1, visited);
      } else {
        // %%% unhandled cases
      }
      break;
    }

    visited.erase(t);

    return true;
  } // End of my_walk

  void my_walk_fields(tree t, unsigned level, std::unordered_set<tree>& visited)
  {

  }


#if 0
  // For PLUGIN_FINISH_DECL.
  void finish_decl_cb(void* gcc_data, void* user_data)
  {
    cerr << "finish_decl_cb\n";
    tree t(static_cast<tree>(gcc_data));
    print_decl(t);
  }
#endif

//  register_callback(plugin_info->base_name,
//                       PLUGIN_FINISH_DECL, finish_decl_cb, nullptr);
