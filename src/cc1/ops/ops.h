#include <stdio.h>
#include <stdarg.h>

#include "../../util/util.h"

#include "../data_structs.h"

#include "expr_addr.h"
#include "expr_assign.h"
#include "expr_cast.h"
#include "expr_comma.h"
#include "expr_funcall.h"
#include "expr_identifier.h"
#include "expr_if.h"
#include "expr_op.h"
#include "expr_sizeof.h"
#include "expr_val.h"

#include "stmt_break.h"
#include "stmt_case.h"
#include "stmt_case_range.h"
#include "stmt_code.h"
#include "stmt_default.h"
#include "stmt_do.h"
#include "stmt_expr.h"
#include "stmt_for.h"
#include "stmt_goto.h"
#include "stmt_if.h"
#include "stmt_label.h"
#include "stmt_noop.h"
#include "stmt_return.h"
#include "stmt_switch.h"
#include "stmt_while.h"

#include "../cc1.h"
#include "../asm.h"
#include "../fold.h"
#include "../const.h"
#include "../gen_str.h"
#include "../gen_asm.h"