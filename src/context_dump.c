#include "php.h"
#include "php_psi.h"

#include "libjit.h"
#include "libffi.h"

static inline void dump_decl_type(int fd, decl_type *t) {
	const char *pre;

	switch (t->type) {
	case PSI_T_STRUCT:
		pre = "struct ";
		break;
	default:
		pre = "";
	}
	dprintf(fd, "%s%s", pre, t->name);
}

static inline void dump_decl_var(int fd, decl_var *v) {
	dprintf(fd, "%.*s%s", v->pointer_level-!!v->array_size, "**********", v->name);
	if (v->array_size) {
		dprintf(fd, "[%u]", v->array_size);
	}
}

static inline void dump_decl_arg(int fd, decl_arg *a) {
	dump_decl_type(fd, a->type);
	dprintf(fd, " ");
	dump_decl_var(fd, a->var);
}

static inline void dump_level(int fd, unsigned level) {
	dprintf(fd, "%.*s", level > 10 ? 10 : level, "\t\t\t\t\t\t\t\t\t\t");
}

static inline void dump_num_exp(int fd, num_exp *exp) {
	while (exp) {
		switch (exp->t) {
		case PSI_T_NUMBER:
			dprintf(fd, "%s", exp->u.numb);
			break;
		case PSI_T_NSNAME:
			dprintf(fd, "%s", exp->u.cnst->name);
			break;
		case PSI_T_NAME:
			dump_decl_var(fd, exp->u.dvar);
			break;
		EMPTY_SWITCH_DEFAULT_CASE();
		}
		if (exp->operand) {
			char op;

			switch (exp->operator) {
			case PSI_T_PLUS:	op = '+'; break;
			case PSI_T_MINUS:	op = '-'; break;
			case PSI_T_ASTERISK:op = '*'; break;
			case PSI_T_SLASH:	op = '/'; break;
			EMPTY_SWITCH_DEFAULT_CASE();
			}
			dprintf(fd, " %c ", op);
		}
		exp = exp->operand;
	}
}

static inline void dump_impl_set_value(int fd, set_value *set, unsigned level, int last) {
	size_t i;

	if (level > 1) {
		/* only if not directly after `set ...` */
		dump_level(fd, level);
	}

	if (set->func->type == PSI_T_ELLIPSIS) {
		dprintf(fd, "%s(", set->outer.set->func->name);
	} else {
		dprintf(fd, "%s(", set->func->name);
	}

	for (i = 0; i < set->vars->count; ++i) {
		decl_var *svar = set->vars->vars[i];
		if (i) {
			dprintf(fd, ", ");
		}
		dump_decl_var(fd, svar);
	}

	if (set->func->type == PSI_T_ELLIPSIS) {
		dprintf(fd, ", ...");
	}
	if (set->num) {
		dprintf(fd, ", ");
		dump_num_exp(fd, set->num);
	}
	if (set->inner && set->func->type != PSI_T_ELLIPSIS) {
		dprintf(fd, ",\n");
		for (i = 0; i < set->count; ++i) {
			dump_impl_set_value(fd, set->inner[i], level+1, i == (set->count - 1));
		}
		/* only if inner stmts, i.e. with new lines, were dumped */
		dump_level(fd, level);
	}
	if (level > 1) {
		dprintf(fd, ")%s\n", last ? "" : ",");
	} else {
		dprintf(fd, ");");
	}
}

static inline void dump_typedef(int fd, decl_typedef *tdef) {
	dprintf(fd, "typedef ");
	dump_decl_type(fd, tdef->type);
	dprintf(fd, " %s%s;", tdef->type->type == PSI_T_POINTER ? "*":"",
			tdef->alias);
}

static inline void dump_typedefs(int fd, decl_typedefs *defs) {
	size_t i;

	for (i = 0; i < defs->count; ++i) {
		decl_typedef *tdef = defs->list[i];

		dump_typedef(fd, tdef);
		dprintf(fd, "\n");
	}
}

static inline void dump_struct(int fd, decl_struct *strct) {
	size_t j;

	dprintf(fd, "struct %s::(%zu)", strct->name, strct->size);
	if (strct->args && strct->args->count) {
		dprintf(fd, " {\n");
		for (j = 0; j < strct->args->count; ++j) {
			decl_arg *sarg = strct->args->args[j];

			dprintf(fd, "\t");
			dump_decl_arg(fd, sarg);
			dprintf(fd, "::(%zu, %zu);\n", sarg->layout->pos, sarg->layout->len);
		}
		dprintf(fd, "}");
	} else {
		dprintf(fd, ";");
	}
}
static inline void dump_structs(int fd, decl_structs *structs) {
	size_t i;

	for (i = 0; i < structs->count; ++i) {
		decl_struct *strct = structs->list[i];

		dump_struct(fd, strct);
		dprintf(fd, "\n");
	}
}

static inline void dump_constant(int fd, constant *cnst) {
	dprintf(fd, "const %s %s = ", cnst->type->name, cnst->name);
	if (cnst->val->type == PSI_T_QUOTED_STRING) {
		dprintf(fd, "\"%s\";", cnst->val->text);
	} else {
		dprintf(fd, "%s;", cnst->val->text);
	}
}

static inline void dump_constants(int fd, constants *consts) {
	size_t i;

	for (i = 0; i < consts->count; ++i) {
		constant *cnst = consts->list[i];

		dump_constant(fd, cnst);
		dprintf(fd, "\n");
	}
}

static inline void dump_decl(int fd, decl *decl) {
	size_t j;

	dprintf(fd, "%s ", decl->abi->convention);
	dump_decl_arg(fd, decl->func);
	dprintf(fd, "(");
	if (decl->args) {
		for (j = 0; j < decl->args->count; ++j) {
			if (j) {
				dprintf(fd, ", ");
			}
			dump_decl_arg(fd, decl->args->args[j]);
		}
		if (decl->args->varargs) {
			dprintf(fd, ", ...");
		}
	}
	dprintf(fd, ");");
}

static inline void dump_decls(int fd, decls *decls) {
	size_t i;

	for (i = 0; i < decls->count; ++i) {
		decl *decl = decls->list[i];

		dump_decl(fd, decl);
		dprintf(fd, "\n");
	}
}

static inline void dump_impl_func(int fd, impl_func *func) {
	size_t j;

	dprintf(fd, "function %s(", func->name);
	if (func->args) {
		for (j = 0; j < func->args->count; ++j) {
			impl_arg *iarg = func->args->args[j];

			dprintf(fd, "%s%s %s$%s",
					j ? ", " : "",
					iarg->type->name,
					iarg->var->reference ? "&" : "",
					iarg->var->name);
			if (iarg->def) {
				dprintf(fd, " = %s", iarg->def->text);
			}
		}
		if (func->args->vararg.name) {
			impl_arg *vararg = func->args->vararg.name;

			dprintf(fd, ", %s %s...$%s",
					vararg->type->name,
					vararg->var->reference ? "&" : "",
					vararg->var->name);
		}
	}
	dprintf(fd, ") : %s%s",
			func->return_reference ? "&":"",
			func->return_type->name);
}

static inline void dump_impl_let_stmt(int fd, let_stmt *let) {
	dprintf(fd, "\tlet %s", let->var->name);
	if (let->val) {
		dprintf(fd, " = %s", let->val->flags.one.is_reference ? "&" : "");
		switch (let->val->kind) {
		case PSI_LET_NULL:
			dprintf(fd, "NULL");
			break;
		case PSI_LET_TMP:
			dump_decl_var(fd, let->val->data.var);
			break;
		case PSI_LET_CALLOC:
			dprintf(fd, "calloc(");
			dump_num_exp(fd, let->val->data.alloc->nmemb);
			dprintf(fd, ", ");
			dump_num_exp(fd, let->val->data.alloc->size);
			dprintf(fd, ")");
			break;
		case PSI_LET_FUNC:
			dprintf(fd, "%s($%s)", let->val->data.func->name,
					let->val->data.func->var->name);
			break;
		case PSI_LET_NUMEXP:
			dump_num_exp(fd, let->val->data.num);
			break;

		EMPTY_SWITCH_DEFAULT_CASE();
		}
		dprintf(fd, ";");
	}
}

static inline void dump_impl_return_stmt(int fd, return_stmt *ret) {
	dprintf(fd, "\treturn ");
	dump_impl_set_value(fd, ret->set, 1, 0);
}

static inline void dump_impl_set_stmt(int fd, set_stmt *set) {
	dprintf(fd, "\tset $%s = ", set->var->name);
	dump_impl_set_value(fd, set->val, 1, 0);
}

static inline void dump_impl_free_call(int fd, free_call *call) {
	size_t l;

	dprintf(fd, "%s(", call->func);
	for (l = 0; l < call->vars->count; ++l) {
		decl_var *fvar = call->vars->vars[l];

		dump_decl_var(fd, fvar);
	}
	dprintf(fd, ");");
}

static inline void dump_impl_free_stmt(int fd, free_stmt *fre) {
	size_t k;

	dprintf(fd, "\tfree ");
	for (k = 0; k < fre->calls->count; ++k) {
		free_call *call = fre->calls->list[k];

		if (k) {
			dprintf(fd, ", ");
		}
		dump_impl_free_call(fd, call);
		dprintf(fd, "\n");
	}
}
static inline void dump_impl_stmts(int fd, impl_stmts *stmts) {
	size_t j;

	for (j = 0; j < stmts->let.count; ++j) {
		let_stmt *let = stmts->let.list[j];
		dump_impl_let_stmt(fd, let);
		dprintf(fd, "\n");
	}
	for (j = 0; j < stmts->ret.count; ++j) {
		return_stmt *ret = stmts->ret.list[j];
		dump_impl_return_stmt(fd, ret);
		dprintf(fd, "\n");
	}
	for (j = 0; j < stmts->set.count; ++j) {
		set_stmt *set = stmts->set.list[j];

		dump_impl_set_stmt(fd, set);
		dprintf(fd, "\n");
	}
	for (j = 0; j < stmts->fre.count; ++j) {
		free_stmt *fre = stmts->fre.list[j];

		dump_impl_free_stmt(fd, fre);
		dprintf(fd, "\n");
	}
}

static inline void dump_impl(int fd, impl *impl) {

	dump_impl_func(fd, impl->func);
	dprintf(fd, " {\n");
	if (impl->stmts) {
		dump_impl_stmts(fd, impl->stmts);
	}
	dprintf(fd, "}");
}

static inline void dump_impls(int fd, impls *impls) {
	size_t i;

	for (i = 0; i < impls->count; ++i) {
		impl *impl = impls->list[i];

		dump_impl(fd, impl);
		dprintf(fd, "\n");
	}
}

void PSI_ContextDump(PSI_Context *C, int fd)
{
#ifdef HAVE_LIBJIT
	if (C->ops == PSI_Libjit()) {
		dprintf(fd, "// psi.engine=jit\n");
	}
#endif
#ifdef HAVE_LIBFFI
	if (C->ops == PSI_Libffi()) {
		dprintf(fd, "// psi.engine=ffi\n");
	}
#endif
	dprintf(fd, "\n");

	if (C->defs) {
		dump_typedefs(fd, C->defs);
		dprintf(fd, "\n");
	}

	if (C->structs) {
		dump_structs(fd, C->structs);
		dprintf(fd, "\n");
	}
	if (C->consts) {
		dump_constants(fd, C->consts);
		dprintf(fd, "\n");
	}
	if (C->decls) {
		dump_decls(fd, C->decls);
		dprintf(fd, "\n");
	}
	if (C->impls) {
		dump_impls(fd, C->impls);
		dprintf(fd, "\n");
	}
}