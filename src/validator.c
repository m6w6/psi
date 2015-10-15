#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <dlfcn.h>

#include <jit/jit.h>

#include "validator.h"

PSI_Validator *PSI_ValidatorInit(PSI_Validator *V, PSI_Parser *P)
{
	if (!V) {
		V = malloc(sizeof(*V));
	}
	memset(V, 0, sizeof(*V));

	PSI_DataExchange((PSI_Data *) V, (PSI_Data *) P);

	return V;
}

void PSI_ValidatorDtor(PSI_Validator *V)
{
	PSI_DataDtor((PSI_Data *) V);
	memset(V, 0, sizeof(*V));
}

void PSI_ValidatorFree(PSI_Validator **V)
{
	if (*V) {
		PSI_ValidatorDtor(*V);
		free(*V);
		*V = NULL;
	}
}

static int validate_lib(PSI_Validator *V) {
	char lib[MAXPATHLEN];
	const char *ptr = V->lib;
	size_t len;

	if (!ptr) {
		/* FIXME: assume stdlib */
		return 1;
		fprintf(stderr, "No import library defined;"
			" use 'lib \"<libname>\";' statement.\n");
	} else if (!strchr(ptr, '/')) {
#ifdef DARWIN
		len = snprintf(lib, MAXPATHLEN, "lib%s.dylib", ptr);
#else
		len = snprintf(lib, MAXPATHLEN, "lib%s.so", ptr);
#endif
		if (MAXPATHLEN == len) {
			fprintf(stderr, "Library name too long: '%s'\n", ptr);
		}
		lib[len] = 0;
		ptr = lib;
	}
	if (!(V->dlopened = dlopen(ptr, RTLD_LAZY|RTLD_LOCAL))) {
		fprintf(stderr, "Could not open library '%s': %s.\n", V->lib, dlerror());
		return 0;
	}
	return 1;
}
static inline int locate_decl_type_alias(decl_typedefs *defs, decl_type *type) {
	size_t i;

	if (type->real) {
		return 1;
	}
	for (i = 0; i < defs->count; ++i) {
		if (!strcmp(defs->list[i]->alias, type->name)) {
			type->real = defs->list[i]->type;
			return 1;
		}
	}
	return 0;
}
static inline int validate_decl_type(PSI_Validator *V, decl_arg *arg, decl_type *type) {
	if (type->type == PSI_T_NAME) {
		size_t i;

		if (!V->defs || !locate_decl_type_alias(V->defs, type)) {
			fprintf(stderr, "Cannot use '%s' as type for '%s';"
				" Use 'typedef <type> <basic_type>;' statement.\n",
				type->name, arg->var->name);
		}
	}
	return 1;
}
static inline int validate_typedef(PSI_Validator *V, decl_typedef *def) {
	/* FIXME: check def->alias */
	if (def->type->type == PSI_T_NAME) {
		fprintf(stderr, "Type '%s' cannot be aliased to '%s'\n",
			def->type->name, def->alias);
		return 0;
	}
	return 1;
}
static inline int validate_typedefs(PSI_Validator *V) {
	size_t i;

	for (i = 0; i < V->defs->count; ++i) {
		if (!validate_typedef(V, V->defs->list[i])) {
			return 0;
		}
	}

	return 1;
}
static inline int validate_decl_func(PSI_Validator *V, decl *decl, decl_arg *func)
{
	void *dlptr;

	if (!strcmp(func->var->name, "dlsym")) {
		fprintf(stderr, "Cannot dlsym dlsym (sic!)\n");
		return 0;
	}

	if (!validate_decl_type(V, func, func->type)) {
		return 0;
	}

	decl->dlptr = dlsym(V->dlopened ?: RTLD_DEFAULT, func->var->name);
	if (!decl->dlptr) {
		fprintf(stderr, "Failed to located symbol '%s': %s\n",
			func->var->name, dlerror());
	}
	return 1;
}
static inline int validate_decl_abi(PSI_Validator *V, decl_abi *abi) {
	if (strcasecmp(abi->convention, "default")) {
		fprintf(stderr, "Invalid calling convention: '%s'\n", abi->convention);
		return 0;
	}
	/* FIXME */
	return 1;
}
static inline int validate_decl_arg(PSI_Validator *V, decl *decl, decl_arg *arg) {
	if (!validate_decl_type(V, arg, arg->type)) {
		return 0;
	}
	return 1;
}
static inline int validate_decl_args(PSI_Validator *V, decl *decl, decl_args *args) {
	size_t i;

	for (i = 0; i < args->count; ++i) {
		if (!validate_decl_arg(V, decl, args->args[i])) {
			return 0;
		}
	}
	return 1;
}
static inline int validate_decl(PSI_Validator *V, decl *decl) {
	if (!validate_decl_abi(V, decl->abi)) {
		return 0;
	}
	if (!validate_decl_func(V, decl, decl->func)) {
		return 0;
	}
	if (decl->args && !validate_decl_args(V, decl, decl->args)) {
		return 0;
	}
	return 1;
}
static inline int validate_decls(PSI_Validator *V) {
	size_t i;

	for (i = 0; i < V->decls->count; ++i) {
		if (!validate_decl(V, V->decls->list[i])) {
			return 0;
		}
	}
	return 1;
}

static inline int validate_impl_type(PSI_Validator *V, impl *impl, impl_type *type) {
	/* FIXME */
	return 1;
}
static inline int validate_impl_arg(PSI_Validator *V, impl *impl, impl_arg *arg) {
	return 1;
}
static inline int validate_impl_args(PSI_Validator *V, impl *impl, impl_args *args) {
	size_t i;

	for (i = 0; i < args->count; ++i) {
		if (!validate_impl_arg(V, impl, args->args[i])) {
			return 0;
		}
	}
	return 1;
}
static inline int validate_impl_func(PSI_Validator *V, impl *impl, impl_func *func) {
	/* FIXME: does name need any validation? */
	if (!validate_impl_type(V, impl, func->return_type)) {
		return 0;
	}
	if (func->args && !validate_impl_args(V, impl, func->args)) {
		return 0;
	}
	return 1;
}
static inline decl *locate_impl_decl(decls *decls, ret_stmt *ret) {
	size_t i;

	for (i = 0; i < decls->count; ++i) {
		if (!strcmp(decls->list[i]->func->var->name, ret->decl->name)) {
			return decls->list[i];
		}
	}
	return NULL;
}
static inline int validate_impl_stmts(PSI_Validator *V, impl *impl, impl_stmts *stmts) {
	/* okay,
	 * - we must have exactly one ret stmt delcaring the native func to call and which type cast to apply
	 * - we can have multiple let stmts; every arg of the ret stmts var (the function to call) must have one
	 * - we can have any count of set stmts; processing out vars, etc.
	 */
	size_t i, j;
	ret_stmt *ret;
	decl *decl;
	int *check;

	 if (!stmts) {
 		fprintf(stderr, "Missing body for implementation %s!\n", impl->func->name);
 		return 0;
 	}
	if (stmts->ret.count != 1) {
		if (stmts->ret.count > 1) {
			fprintf(stderr, "Too many `ret` statements for implmentation %s; found %zu, exactly one is needed.\n",
				impl->func->name, stmts->ret.count);
		} else {
			fprintf(stderr, "Missing `ret` statement for implementation %s.\n", impl->func->name);
		}
		return 0;
	}

	ret = stmts->ret.list[0];
	decl = locate_impl_decl(V->decls, ret);
	if (!decl) {
		fprintf(stderr, "Missing declaration for implementation %s.\n", impl->func->name);
		return 0;
	}

	/* check that we have a let stmt for every decl arg */
	check = calloc(decl->args->count, sizeof(int));
	for (i = 0; i < stmts->let.count; ++i) {
		let_stmt *let = stmts->let.list[i];

		for (j = 0; j < decl->args->count; ++j) {
			if (!strcmp(decl->args->args[j]->var->name, let->var->name)) {
				check[j] = 1;
				break;
			}
		}
	}
	for (i = 0; i < decl->args->count; ++i) {
		if (!check[i]) {
			fprintf(stderr, "Missing `let` statement for arg '%s %.*s%s' of declaration '%s' for implementation '%s'.\n",
				decl->args->args[i]->type->name, (int) decl->args->args[i]->var->pointer_level, "*****", decl->args->args[i]->var->name, decl->func->var->name, impl->func->name);
			free(check);
			return 0;
		}
	}
	free(check);

	impl->decl = decl;

	return 1;
}

static inline int validate_impl(PSI_Validator *V, impl *impl) {
	if (!validate_impl_func(V, impl, impl->func)) {
		return 0;
	}
	if (!validate_impl_stmts(V, impl, impl->stmts)) {
		return 0;
	}
	return 1;
}
static inline int validate_impls(PSI_Validator *V) {
	size_t i;

	for (i = 0; i < V->impls->count; ++i) {
		if (!validate_impl(V, V->impls->list[i])) {
			return 0;
		}
	}
	return 1;
}

int PSI_ValidatorValidate(PSI_Validator *V)
{
	if (!validate_lib(V)) {
		return 0;
	}
	if (V->defs && !validate_typedefs(V)) {
		return 0;
	}
	if (V->decls && !validate_decls(V)) {
		return 0;
	}
	if (V->impls && !validate_impls(V)) {
		return 0;
	}
	return 1;
}