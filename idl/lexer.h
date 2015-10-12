/* This file was automatically generated.  Do not edit! */
#undef INTERFACE
#define PSI_T_COMMENT                         1
#define PSI_T_NULL                           27
#define PSI_T_MIXED                          42
#define PSI_T_VOID                           10
#define PSI_T_BOOLVAL                        35
#define PSI_T_BOOL                           43
#define PSI_T_INTVAL                         33
#define PSI_T_INT                            11
#define PSI_T_FLOATVAL                       34
#define PSI_T_FLOAT                          12
#define PSI_T_FUNCTION                       24
#define PSI_T_DOUBLE                         13
#define PSI_T_SET                            36
#define PSI_T_SINT16                         16
#define PSI_T_SINT32                         18
#define PSI_T_SINT64                         20
#define PSI_T_SINT8                          14
#define PSI_T_STRING                         44
#define PSI_T_STRVAL                         32
#define PSI_T_UINT16                         17
#define PSI_T_UINT32                         19
#define PSI_T_UINT64                         21
#define PSI_T_UINT8                          15
#define PSI_T_ARRAY                          45
#define PSI_T_TO_BOOL                        40
#define PSI_T_TO_FLOAT                       39
#define PSI_T_TO_INT                         38
#define PSI_T_TO_STRING                      37
#define PSI_T_TYPEDEF                         5
#define PSI_T_LET                            31
#define PSI_T_LIB                             2
#define PSI_T_RET                            41
#define PSI_T_NSNAME                         25
#define PSI_T_QUOTED_STRING                   3
#define PSI_T_DIGIT                          46
#define PSI_T_NAME                            6
#define PSI_T_REFERENCE                      29
#define PSI_T_POINTER                        50
#define PSI_T_DOLLAR                         28
#define PSI_T_EQUALS                         30
#define PSI_T_DOT                            47
#define PSI_T_RBRACE                         23
#define PSI_T_LBRACE                         22
#define PSI_T_COLON                          26
#define PSI_T_COMMA                           9
#define PSI_T_EOS                             4
#define PSI_T_RPAREN                          8
#define PSI_T_LPAREN                          7
typedef int token_t;
typedef struct PSI_Lexer PSI_Lexer;
typedef struct decl_typedefs decl_typedefs;
typedef struct decl_typedef decl_typedef;
typedef struct decl_type decl_type;
struct decl_type {
	char *name;
	token_t type;
	struct decl_type *real;
};
struct decl_typedef {
	char *alias;
	decl_type *type;
};
struct decl_typedefs {
	size_t count;
	decl_typedef **list;
};
typedef struct decls decls;
typedef struct decl decl;
typedef struct decl_abi decl_abi;
struct decl_abi {
	char *convention;
};
typedef struct decl_arg decl_arg;
typedef struct decl_var decl_var;
struct decl_var {
	char *name;
	unsigned pointer_level;
};
struct decl_arg {
	decl_type *type;
	decl_var *var;
};
typedef struct decl_args decl_args;
struct decl_args {
	decl_arg **args;
	size_t count;
};
struct decl {
	decl_abi *abi;
	decl_arg *func;
	decl_args *args;
	void *dlptr;
};
struct decls {
	size_t count;
	decl **list;
};
typedef struct impls impls;
typedef struct impl impl;
typedef struct impl_func impl_func;
typedef struct impl_args impl_args;
typedef struct impl_arg impl_arg;
typedef struct impl_type impl_type;
struct impl_type {
	char *name;
	token_t type;
};
typedef struct impl_var impl_var;
struct impl_var {
	char *name;
	unsigned reference:1;
};
typedef struct impl_def_val impl_def_val;
struct impl_def_val {
	token_t type;
	union {
		int64_t digits;
		double decimals;
	} v;
	unsigned is_null:1;
};
struct impl_arg {
	impl_type *type;
	impl_var *var;
	impl_def_val *def;
};
struct impl_args {
	impl_arg **args;
	size_t count;
};
struct impl_func {
	char *name;
	impl_args *args;
	impl_type *return_type;
};
typedef struct impl_stmts impl_stmts;
typedef struct impl_stmt impl_stmt;
typedef struct let_stmt let_stmt;
typedef struct let_value let_value;
typedef struct let_func let_func;
struct let_func {
	token_t type;
	char *name;
};
struct let_value {
	let_func *func;
	impl_var *var;
	unsigned null_pointer_ref:1;
};
struct let_stmt {
	decl_var *var;
	let_value *val;
};
typedef struct set_stmt set_stmt;
typedef struct set_value set_value;
typedef struct set_func set_func;
struct set_func {
	token_t type;
	char *name;
};
typedef struct decl_vars decl_vars;
struct decl_vars {
	decl_var **vars;
	size_t count;
};
struct set_value {
	set_func *func;
	decl_vars *vars;
};
struct set_stmt {
	impl_var *var;
	set_value *val;
};
typedef struct ret_stmt ret_stmt;
struct ret_stmt {
	set_func *func;
	decl_var *decl;
};
struct impl_stmt {
	token_t type;
	union {
		let_stmt *let;
		set_stmt *set;
		ret_stmt *ret;
		void *ptr;
	} s;
};
struct impl_stmts {
	impl_stmt **stmts;
	size_t count;
};
struct impl {
	impl_func *func;
	impl_stmts *stmts;
};
struct impls {
	size_t count;
	impl **list;
};
#define BSIZE 256
struct PSI_Lexer {
	decl_typedefs *defs;
	decls *decls;
	impls *impls;
	char *lib;
	char *fn;
	FILE *fp;
	size_t line;
	char *cur, *tok, *lim, *eof, *ctx, *mrk, buf[BSIZE];
};
token_t PSI_LexerScan(PSI_Lexer *L);
PSI_Lexer *PSI_LexerInit(PSI_Lexer *L,const char *filename);
void PSI_LexerFree(PSI_Lexer **L);
void PSI_LexerDtor(PSI_Lexer *L);
size_t PSI_LexerFill(PSI_Lexer *L,size_t n);
typedef struct PSI_Token PSI_Token;
struct PSI_Token {
	token_t type;
	unsigned line;
	size_t size;
	char text[1];
};
PSI_Token *PSI_TokenAlloc(PSI_Lexer *L,token_t t);
#define YYMAXFILL 10
#define INTERFACE 0
