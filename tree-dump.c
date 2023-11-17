/* raw.githubusercontent.com/gcc-mirror/gcc/a5544970246db337977bb8b69ab120e9ef209317/gcc/tree-dump.c */

/* Tree-dumping functionality for intermediate representation.
   Copyright (C) 1999-2019 Free Software Foundation, Inc.
   Written by Mark Mitchell <mark@codesourcery.com>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.


 20201110 Make output deterministically parseable. We do this by
changing space to underscore in string literals (which loses no
information in this particular application); we also change the colon
in a "srcp" field (e.g. "filename.c:123") to a semicolon.
 20201111 DECL_EXPR now dumps its DECL_EXPR_DECL child

 */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"
#include "tree-pretty-print.h"
#include "tree-dump.h"
#include "langhooks.h"
#include "tree-iterator.h"

/* Dump the CHILD and its children.  */
#define RM_DUMP_CHILD(field, child) \
  rm_enq_and_dump_ix (di, field, child, DUMP_NONE)

static unsigned int rm_enqueue (dump_info_p, const_tree, int);
static void rm_dump_index (dump_info_p, unsigned int);
static void rm_enq_and_dump_ix (dump_info_p, const char *, const_tree, int);
static void rm_dump_new_line (dump_info_p);
static void rm_dump_maybe_nl (dump_info_p);

static void rm_dequeue_and_dump (dump_info_p);
static void rm_dump_node (const_tree t, dump_flags_t flags, FILE *stream);

/* Add T to the end of the queue of nodes to dump.  Returns the index
   assigned to T.  */

static unsigned int rm_enqueue (dump_info_p di, const_tree t, int flags)
{
  dump_queue_p dq;
  dump_node_info_p dni;
  unsigned int index;

  /* Assign the next available index to T.  */
  index = ++di->index;

  /* Obtain a new queue node.  */
  if (di->free_list) {
    dq = di->free_list;
    di->free_list = dq->next;
  } else {
    dq = XNEW (struct dump_queue);
  }

  /* Create a new entry in the splay-tree.  */
  dni = XNEW (struct dump_node_info);
  dni->index = index;
  dni->binfo_p = ((flags & DUMP_BINFO) != 0);
  dq->node = splay_tree_insert (di->nodes, (splay_tree_key) t,
                                (splay_tree_value) dni);

  /* Add it to the end of the queue.  */
  dq->next = 0;
  if (!di->queue_end) {
    di->queue = dq;
  } else {
    di->queue_end->next = dq;
  }
  di->queue_end = dq;

  /* Return the index.  */
  return index;
}

static void rm_dump_index (dump_info_p di, unsigned int index)
{
  fprintf (di->stream, "@%-6u ", index);
  di->column += 8;
}

/* If T has not already been output, queue it for subsequent output.
   FIELD is a string to print before printing the index.  Then, the
   index of T is printed.  */

static void rm_enq_and_dump_ix (dump_info_p di, const char *field,
                                const_tree t, int flags)
{
  unsigned int index;
  splay_tree_node n;

  /* If there's no node, just return.  This makes for fewer checks in
     our callers.  */
  if (!t) {
    return;
  }

  /* See if we've already queued or dumped this node.  */
  n = splay_tree_lookup (di->nodes, (splay_tree_key) t);
  if (n) {
    index = ((dump_node_info_p) n->value)->index;
  } else {
    /* If we haven't, add it to the queue.  */
    index = rm_enqueue (di, t, flags);
  }

  /* Print the index of the node.  */
  rm_dump_maybe_nl (di);
  fprintf (di->stream, "%-4s: ", field);
  di->column += 6;
  rm_dump_index (di, index);
}

/* Dump the type of T.  */

void rm_enq_and_dump_type (dump_info_p di, const_tree t)
{
  rm_enq_and_dump_ix (di, "type", TREE_TYPE (t), DUMP_NONE);
}

/* Dump column control */
#define SOL_COLUMN 25        /* Start of line column. */
#define EOL_COLUMN 55        /* End of line column. */
#define COLUMN_ALIGNMENT 15  /* Spaces between 'tab stops' */

/* Insert a new line in the dump output, and indent to an appropriate
   place to start printing more fields. */
static void rm_dump_new_line (dump_info_p di)
{
  fprintf (di->stream, "\n%*s", SOL_COLUMN, "");
  di->column = SOL_COLUMN;
}

/* Space over to a new column, and if necessary, start a new line. */
static void rm_dump_maybe_nl (dump_info_p di)
{
  int extra;

  if (di->column > EOL_COLUMN) {
    /* We need to go to a new line.  */
    rm_dump_new_line (di);
  } else if ((extra = (di->column - SOL_COLUMN) % COLUMN_ALIGNMENT) != 0) {
    /* We need to add spaces to reach the next column.  */
    fprintf (di->stream, "%*s", COLUMN_ALIGNMENT - extra, "");
    di->column += COLUMN_ALIGNMENT - extra;
  }
}

/* Dump a pointer-type field.  */
void rm_dump_pointer (dump_info_p di, const char *field, void *ptr)
{
  rm_dump_maybe_nl (di);
  fprintf (di->stream, "%-4s: %-8" HOST_WIDE_INT_PRINT "x ", field,
           (unsigned HOST_WIDE_INT) (uintptr_t) ptr);
  di->column += 15;
}

/* Dump a field with an integer type value.  */
void rm_dump_int (dump_info_p di, const char *field, int i)
{
  rm_dump_maybe_nl (di);
  fprintf (di->stream, "%-4s: %-7d ", field, i);
  di->column += 14;
}

/* Dump a field with a floating point value.  */
static void rm_dump_real (dump_info_p di, const char *field,
                          const REAL_VALUE_TYPE *r)
{
  char buf[32];
  real_to_decimal (buf, r, sizeof (buf), 0, true);
  rm_dump_maybe_nl (di);
  fprintf (di->stream, "%-4s: %s ", field, buf);
  di->column += strlen (buf) + 7;
}

/* Dump a field with a fixed-point type value. */
static void rm_dump_fixed (dump_info_p di, const char *field,
                           const FIXED_VALUE_TYPE *f)
{
  char buf[32];
  fixed_to_decimal (buf, f, sizeof (buf));
  rm_dump_maybe_nl (di);
  fprintf (di->stream, "%-4s: %s ", field, buf);
  di->column += strlen (buf) + 7;
}


/* Dump a string-type field, taking care to remap the characters we need for
   deterministic parsing by the consumer of our output. */
void rm_dump_str_field (dump_info_p di, const char *field, const char *string)
{
  const char * cp;
  rm_dump_maybe_nl (di);
  /* fprintf (di->stream, "%-4s: '%s' ", field, string); */
  fprintf (di->stream, "%-4s: ", field);
  for(cp=string; *cp; cp++) {
    if (*cp == ' ') {
      fprintf(di->stream, "%c", '_');
    } else if (*cp == ':') {
      fprintf(di->stream, "%c", ';');
    } else {
      fprintf(di->stream, "%c", *cp);
    }
  }
  fprintf (di->stream, " ");

  /*        "strg: "   '                     '  spc */
  di->column += 6    + 1 + strlen (string) + 1 + 1;
}

/* Dump the next node in the queue. */
static void rm_dequeue_and_dump (dump_info_p di)
{
  dump_queue_p dq;
  splay_tree_node stn;
  dump_node_info_p dni;
  tree t;
  unsigned int index;
  enum tree_code code;
  enum tree_code_class code_class;
  const char* code_name;

  /* Get the next node from the queue.  */
  dq = di->queue;
  stn = dq->node;
  t = (tree) stn->key;
  dni = (dump_node_info_p) stn->value;
  index = dni->index;

  /* Remove the node from the queue, and put it on the free list.  */
  di->queue = dq->next;
  if (!di->queue) {
    di->queue_end = 0;
  }
  dq->next = di->free_list;
  di->free_list = dq;

  /* Print the node index.  */
  rm_dump_index (di, index);
  /* And the type of node this is.  */
  if (dni->binfo_p) {
    code_name = "binfo";
  } else {
    code_name = get_tree_code_name (TREE_CODE (t));
  }
  fprintf (di->stream, "%-16s ", code_name);
  di->column = 25;

  /* Figure out what kind of node this is.  */
  code = TREE_CODE (t);
  code_class = TREE_CODE_CLASS (code);

  /* Although BINFOs are TREE_VECs, we dump them specially so as to be
     more informative.  */
  if (dni->binfo_p) {
    unsigned ix;
    tree base;
    vec<tree, va_gc> *accesses = BINFO_BASE_ACCESSES (t);

    RM_DUMP_CHILD ("type", BINFO_TYPE (t));

    if (BINFO_VIRTUAL_P (t)) {
      rm_dump_str_field (di, "spec", "virt");
    }

    rm_dump_int (di, "bases", BINFO_N_BASE_BINFOS (t));
    for (ix = 0; BINFO_BASE_ITERATE (t, ix, base); ix++) {
      tree access = (accesses ? (*accesses)[ix] : access_public_node);
      const char *string = NULL;

      if (access == access_public_node) {
        string = "pub";
      } else if (access == access_protected_node) {
        string = "prot";
      } else if (access == access_private_node) {
        string = "priv";
      } else {
        gcc_unreachable ();
      }

      rm_dump_str_field (di, "accs", string);
      rm_enq_and_dump_ix (di, "binf", base, DUMP_BINFO);
    }

    goto done;
  }

  /* We can knock off a bunch of expression nodes in exactly the same
     way.  */
  if (IS_EXPR_CODE_CLASS (code_class)) {
    /* If we're dumping children, dump them now.  */
    rm_enq_and_dump_type (di, t);

    switch (code_class)
      {
      case tcc_unary:
        RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
        break;

      case tcc_binary:
      case tcc_comparison:
        RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
        RM_DUMP_CHILD ("op 1", TREE_OPERAND (t, 1));
        break;

      case tcc_expression:
      case tcc_reference:
      case tcc_statement:
      case tcc_vl_exp:
        /* These nodes are handled explicitly below.  */
        break;

      default:
        gcc_unreachable ();
      }
  } else if (DECL_P (t)) {
    expanded_location xloc;
    /* All declarations have names.  */
    if (DECL_NAME (t)) {
      RM_DUMP_CHILD ("name", DECL_NAME (t));
    }
    if (HAS_DECL_ASSEMBLER_NAME_P (t)
        && DECL_ASSEMBLER_NAME_SET_P (t)
        && DECL_ASSEMBLER_NAME (t) != DECL_NAME (t))
    {
      RM_DUMP_CHILD ("mngl", DECL_ASSEMBLER_NAME (t));
    }
    if (DECL_ABSTRACT_ORIGIN (t)) {
      RM_DUMP_CHILD ("orig", DECL_ABSTRACT_ORIGIN (t));
    }

    /* And types.  */
    rm_enq_and_dump_type (di, t);
    RM_DUMP_CHILD ("scpe", DECL_CONTEXT (t));
    /* And a source position.  */
    xloc = expand_location (DECL_SOURCE_LOCATION (t));
    if (xloc.file) {
      const char *filename = lbasename (xloc.file);

      rm_dump_maybe_nl (di);
      fprintf (di->stream, "srcp: %s;%-6d ", filename, xloc.line);
      di->column += 6 + strlen (filename) + 8;
    }
    /* And any declaration can be compiler-generated.  */
    if (CODE_CONTAINS_STRUCT (TREE_CODE (t), TS_DECL_COMMON)
        && DECL_ARTIFICIAL (t))
    {
      rm_dump_str_field (di, "note", "artificial");
    }
    if (DECL_CHAIN (t) && !dump_flag (di, TDF_SLIM, NULL)) {
      RM_DUMP_CHILD ("chain", DECL_CHAIN (t));
    }
  } else if (code_class == tcc_type) {
    /* All types have qualifiers.  */
    int quals = lang_hooks.tree_dump.type_quals (t);

    if (quals != TYPE_UNQUALIFIED) {
      fprintf (di->stream, "qual: %c%c%c     ",
               (quals & TYPE_QUAL_CONST) ? 'c' : ' ',
               (quals & TYPE_QUAL_VOLATILE) ? 'v' : ' ',
               (quals & TYPE_QUAL_RESTRICT) ? 'r' : ' ');
      di->column += 14;
    }

    /* All types have associated declarations.  */
    RM_DUMP_CHILD ("name", TYPE_NAME (t));

    /* All types have a main variant.  */
    if (TYPE_MAIN_VARIANT (t) != t) {
      RM_DUMP_CHILD ("unql", TYPE_MAIN_VARIANT (t));
    }

    /* And sizes.  */
    RM_DUMP_CHILD ("size", TYPE_SIZE (t));

    /* All types have alignments.  */
    rm_dump_int (di, "algn", TYPE_ALIGN (t));
  } else if (code_class == tcc_constant) {
    /* All constants can have types.  */
    rm_enq_and_dump_type (di, t);
  }

  /* Give the language-specific code a chance to print something.  If
     it's completely taken care of things, don't bother printing
     anything more ourselves.  */
  if (lang_hooks.tree_dump.dump_tree (di, t)) {
    // fprintf(stderr, "lh-handled\n");
    goto done;
  } else {
    // fprintf(stderr, "custom\n");
  }

  /* Now handle the various kinds of nodes.  */
  switch (code) {
    int i;

    case IDENTIFIER_NODE:
      rm_dump_str_field (di, "strg", IDENTIFIER_POINTER (t));
      rm_dump_int (di, "lngt", IDENTIFIER_LENGTH (t));
      break;

    case TREE_LIST:
      RM_DUMP_CHILD ("purp", TREE_PURPOSE (t));
      RM_DUMP_CHILD ("valu", TREE_VALUE (t));
      RM_DUMP_CHILD ("chan", TREE_CHAIN (t));
      break;

    case STATEMENT_LIST:
      {
        tree_stmt_iterator it;
        for (i=0, it = tsi_start (t); !tsi_end_p (it); tsi_next (&it), i++) {
          char buffer[32];
          sprintf (buffer, "%u", i);
          RM_DUMP_CHILD (buffer, tsi_stmt (it));
        }
      }
      break;

    case TREE_VEC:
      rm_dump_int (di, "lngt", TREE_VEC_LENGTH (t));
      for (i = 0; i < TREE_VEC_LENGTH (t); ++i) {
        char buffer[32];
        sprintf (buffer, "%u", i);
        RM_DUMP_CHILD (buffer, TREE_VEC_ELT (t, i));
      }
      break;

    case INTEGER_TYPE:
    case ENUMERAL_TYPE:
      rm_dump_int (di, "prec", TYPE_PRECISION (t));
      rm_dump_str_field (di, "sign", TYPE_UNSIGNED (t) ? "unsigned": "signed");
      RM_DUMP_CHILD ("min", TYPE_MIN_VALUE (t));
      RM_DUMP_CHILD ("max", TYPE_MAX_VALUE (t));

      if (code == ENUMERAL_TYPE) {
        RM_DUMP_CHILD ("csts", TYPE_VALUES (t));
      }
      break;

    case REAL_TYPE:
      rm_dump_int (di, "prec", TYPE_PRECISION (t));
      break;

    case FIXED_POINT_TYPE:
      rm_dump_int (di, "prec", TYPE_PRECISION (t));
      rm_dump_str_field (di, "sign", TYPE_UNSIGNED (t) ? "unsigned": "signed");
      rm_dump_str_field (di, "saturating",
                         TYPE_SATURATING (t) ? "saturating": "non-saturating");
      break;

    case POINTER_TYPE:
      RM_DUMP_CHILD ("ptd", TREE_TYPE (t));
      break;

    case REFERENCE_TYPE:
      RM_DUMP_CHILD ("refd", TREE_TYPE (t));
      break;

    case METHOD_TYPE:
      RM_DUMP_CHILD ("clas", TYPE_METHOD_BASETYPE (t));
      /* Fall through.  */

    case FUNCTION_TYPE:
      RM_DUMP_CHILD ("retn", TREE_TYPE (t));
      RM_DUMP_CHILD ("prms", TYPE_ARG_TYPES (t));
      break;

    case ARRAY_TYPE:
      RM_DUMP_CHILD ("elts", TREE_TYPE (t));
      RM_DUMP_CHILD ("domn", TYPE_DOMAIN (t));
      break;

    case RECORD_TYPE:
    case UNION_TYPE:
      if (TREE_CODE (t) == RECORD_TYPE) {
        rm_dump_str_field (di, "tag", "struct");
      } else {
        rm_dump_str_field (di, "tag", "union");
      }

      RM_DUMP_CHILD ("flds", TYPE_FIELDS (t));
      rm_enq_and_dump_ix (di, "binf", TYPE_BINFO (t),
                            DUMP_BINFO);
      break;

    case CONST_DECL:
      /* tree.def: "DECL_INITIAL holds the value to initialize a
         variable to, or the value of a constant." */
      RM_DUMP_CHILD ("cnst", DECL_INITIAL (t));
      break;

    case DECL_EXPR:
      /* tree.def: "[DECL_EXPR is] Used to represent a local
         declaration. The operand is DECL_EXPR_DECL." */
      RM_DUMP_CHILD ("decl", DECL_EXPR_DECL (t));
      break;

    case DEBUG_EXPR_DECL:
      rm_dump_int (di, "-uid", DEBUG_TEMP_UID (t));
      /* Fall through.  */

    case VAR_DECL:
    case PARM_DECL:
    case FIELD_DECL:
    case RESULT_DECL:
      if (TREE_CODE (t) == PARM_DECL) {
        RM_DUMP_CHILD ("argt", DECL_ARG_TYPE (t));
      } else {
        RM_DUMP_CHILD ("init", DECL_INITIAL (t));
      }
      RM_DUMP_CHILD ("size", DECL_SIZE (t));
      rm_dump_int (di, "algn", DECL_ALIGN (t));

      if (TREE_CODE (t) == FIELD_DECL) {
        if (DECL_FIELD_OFFSET (t)) {
          RM_DUMP_CHILD ("bpos", bit_position (t));
        }
      } else if (VAR_P (t) || TREE_CODE (t) == PARM_DECL) {
        rm_dump_int (di, "used", TREE_USED (t));
        if (DECL_REGISTER (t)) {
          rm_dump_str_field (di, "spec", "register");
        }
      }
      break;

    case FUNCTION_DECL:
      RM_DUMP_CHILD ("args", DECL_ARGUMENTS (t));
      if (DECL_EXTERNAL (t)) {
        rm_dump_str_field (di, "body", "undefined");
      }
      if (TREE_PUBLIC (t)) {
        rm_dump_str_field (di, "link", "extern");
      } else {
        rm_dump_str_field (di, "link", "static");
      }
      if (DECL_SAVED_TREE (t) && !dump_flag (di, TDF_SLIM, t)) {
        RM_DUMP_CHILD ("body", DECL_SAVED_TREE (t));
      }
      break;

    case INTEGER_CST:
      fprintf (di->stream, "int: ");
      print_decs (wi::to_wide (t), di->stream);
      break;

    case STRING_CST:
      fprintf (di->stream, "strg: %-7s ", TREE_STRING_POINTER (t));
      rm_dump_int (di, "lngt", TREE_STRING_LENGTH (t));
      break;

    case REAL_CST:
      rm_dump_real (di, "valu", TREE_REAL_CST_PTR (t));
      break;

    case FIXED_CST:
      rm_dump_fixed (di, "valu", TREE_FIXED_CST_PTR (t));
      break;

    case TRUTH_NOT_EXPR:
    case ADDR_EXPR:
    case INDIRECT_REF:
    case CLEANUP_POINT_EXPR:
    case SAVE_EXPR:
    case REALPART_EXPR:
    case IMAGPART_EXPR:
    case SIZEOF_EXPR:
      /* These nodes are unary, but do not have code class `1'.  */
      RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
      break;

    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case INIT_EXPR:
    case MODIFY_EXPR:
    case COMPOUND_EXPR:
    case PREDECREMENT_EXPR:
    case PREINCREMENT_EXPR:
    case POSTDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
      /* These nodes are binary, but do not have code class `2'.  */
      RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("op 1", TREE_OPERAND (t, 1));
      break;

    case COMPONENT_REF:
    case BIT_FIELD_REF:
      RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("op 1", TREE_OPERAND (t, 1));
      RM_DUMP_CHILD ("op 2", TREE_OPERAND (t, 2));
      break;

    case ARRAY_REF:
    case ARRAY_RANGE_REF:
      RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("op 1", TREE_OPERAND (t, 1));
      RM_DUMP_CHILD ("op 2", TREE_OPERAND (t, 2));
      RM_DUMP_CHILD ("op 3", TREE_OPERAND (t, 3));
      break;

    case COND_EXPR:
      RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("op 1", TREE_OPERAND (t, 1));
      RM_DUMP_CHILD ("op 2", TREE_OPERAND (t, 2));
      break;

    case TRY_FINALLY_EXPR:
      RM_DUMP_CHILD ("op 0", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("op 1", TREE_OPERAND (t, 1));
      break;

    case CALL_EXPR:
      {
        int i = 0;
        tree arg;
        call_expr_arg_iterator iter;
        RM_DUMP_CHILD ("fn", CALL_EXPR_FN (t));
        FOR_EACH_CALL_EXPR_ARG (arg, iter, t) {
          char buffer[32];
          sprintf (buffer, "%u", i);
          RM_DUMP_CHILD (buffer, arg);
          i++;
        }
      }
      break;

    case CONSTRUCTOR:
      {
        unsigned HOST_WIDE_INT cnt;
        tree index, value;
        rm_dump_int (di, "lngt", CONSTRUCTOR_NELTS (t));
        FOR_EACH_CONSTRUCTOR_ELT (CONSTRUCTOR_ELTS (t), cnt, index, value) {
          RM_DUMP_CHILD ("idx", index);
          RM_DUMP_CHILD ("val", value);
        }
      }
      break;

    case BIND_EXPR:
      RM_DUMP_CHILD ("vars", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("body", TREE_OPERAND (t, 1));
      break;

    case LOOP_EXPR:
      RM_DUMP_CHILD ("body", TREE_OPERAND (t, 0));
      break;

    case EXIT_EXPR:
      RM_DUMP_CHILD ("cond", TREE_OPERAND (t, 0));
      break;

    case RETURN_EXPR:
      RM_DUMP_CHILD ("expr", TREE_OPERAND (t, 0));
      break;

    case TARGET_EXPR:
      RM_DUMP_CHILD ("decl", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("init", TREE_OPERAND (t, 1));
      RM_DUMP_CHILD ("clnp", TREE_OPERAND (t, 2));
      /* There really are two possible places the initializer can be.
         After RTL expansion, the second operand is moved to the
         position of the fourth operand, and the second operand
         becomes NULL.  */
      RM_DUMP_CHILD ("init", TREE_OPERAND (t, 3));
      break;

    case CASE_LABEL_EXPR:
      RM_DUMP_CHILD ("name", CASE_LABEL (t));
      if (CASE_LOW (t)) {
        RM_DUMP_CHILD ("low ", CASE_LOW (t));
        if (CASE_HIGH (t)) {
          RM_DUMP_CHILD ("high", CASE_HIGH (t));
        }
      }
      break;

    case LABEL_EXPR:
      RM_DUMP_CHILD ("name", TREE_OPERAND (t,0));
      break;

    case GOTO_EXPR:
      RM_DUMP_CHILD ("labl", TREE_OPERAND (t, 0));
      break;

    case SWITCH_EXPR:
      RM_DUMP_CHILD ("cond", TREE_OPERAND (t, 0));
      RM_DUMP_CHILD ("body", TREE_OPERAND (t, 1));
      break;

    case OMP_CLAUSE:
      {
        int i;
        fprintf (di->stream, "%s\n", omp_clause_code_name[OMP_CLAUSE_CODE (t)]);
        for (i = 0; i < omp_clause_num_ops[OMP_CLAUSE_CODE (t)]; i++) {
          RM_DUMP_CHILD ("op: ", OMP_CLAUSE_OPERAND (t, i));
        }
      }
      break;
    case VIEW_CONVERT_EXPR:
      {
        tree op = TREE_OPERAND(t, 0);
        RM_DUMP_CHILD ("vcop", op);
      }
      break;
    default:
      /* There are no additional fields to print.  */
      break;
  }

 done:
  if (dump_flag (di, TDF_ADDRESS, NULL)) {
    rm_dump_pointer (di, "addr", (void *)t);
  }

  /* Terminate the line.  */
  fprintf (di->stream, "\n");
} /* End of rm.dequeue_and_dump */

/* Return nonzero if FLAG has been specified for the dump, and NODE
   is not the root node of the dump. */
int dump_flag (dump_info_p di, dump_flags_t flag, const_tree node)
{
  return (di->flags & flag) && (node != di->node);
}

/* Dump T, and all its children, on STREAM. */
void rm_dump_node (const_tree t, dump_flags_t flags, FILE *stream)
{
  struct dump_info di;
  dump_queue_p dq;
  dump_queue_p next_dq;

  /* Initialize the dump-information structure.  */
  di.stream = stream;
  di.index = 0;
  di.column = 0;
  di.queue = 0;
  di.queue_end = 0;
  di.free_list = 0;
  di.flags = flags;
  di.node = t;
  di.nodes = splay_tree_new (splay_tree_compare_pointers, 0,
                             splay_tree_delete_pointers);

  /* Queue up the first node.  */
  rm_enqueue (&di, t, DUMP_NONE);

  /* Until the queue is empty, keep dumping nodes.  */
  while (di.queue) {
    rm_dequeue_and_dump (&di);
  }

  /* Now, clean up.  */
  for (dq = di.free_list; dq; dq = next_dq) {
    next_dq = dq->next;
    free (dq);
  }
  splay_tree_delete (di.nodes);
}
