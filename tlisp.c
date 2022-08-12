#include "mpc.h"

/* Macros for Error Checking */
#define LASSERT(args, cond, fmt, ...) \
	if (!(cond)) { \
		lval* err = lval_err(fmt, ##__VA_ARGS__); \
		lval_del(args); \
		return err; \
	}
#define LASSERT_TYPE(func, args, index, expect) \
	LASSERT(args, args->cell[index]->type == expect, \
		"Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
		func, index, ltype_name(args->cell[index]->type), ltype_name(expect))
#define LASSERT_NUM(func, args, num) \
	LASSERT(args, args->count == num, \
		"Function '%s' passed incorrect number of arguments %i. Got %s, Expected %s.", \
		func, args->count, num)
#define LASSERT_NOT_EMPTY(func, args, index) \
	LASSERT(args, args->cell[index]->count != 0, \
		"Function '%s' passed {} for argument %i.", func, index);

		

#ifdef  _WIN32
static char buffer[2048];

char * readline(char* prompt)
{
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char* cpy = malloc(strlen(buffer)+1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy)-1] = '\0';
	return cpy;
}
void add_history(char* unused) {}
#else 
#include <editline.h>
#endif

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
/* Lisp Value */
enum {LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR};
typedef lval*(*lbuiltin)(lenv*, lval*);
/* New lval Struct */
struct lval
{
	int type;
	long num;
	char* err;
	char* sym;
	lbuiltin fun;
	/* Count and Pointer to list of "lval" */
	int count;
	struct lval** cell;
};

struct lenv
{
	int count;
	char** syms;
	lval** vals;
};

/* Pointer Constructors */
/* New number type lval */
lval* lval_num(long x) 
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}
/* Pointer to a new Error lval */
lval* lval_err(char* fmt, ...)
{
	lval* v = malloc(sizeof(lval)); 
	v->type = LVAL_ERR;
	/* Create a valist and initialize it */
	va_list va;
	va_start(va, fmt);

	/* Allocate 512 bytes of space */
	v->err = malloc(512);
	/* printf the error string with a maximum of 511 chars */
	vsnprintf(v->err, 511, fmt, va);
	/* Reallocate to number of bytes actually used */
	v->err = realloc(v->err, strlen(v->err)+1);
	/* Cleanup */
	va_end(va);

	return v;

}
/* Pointer constructor to a new Symbol lval */
lval* lval_sym(char* s)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}
/* Pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) 
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}
/* Pointer to new empty Qexpr lval */
lval* lval_qexpr(void)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}
/* Constructor to function for lbuiltin */
lval* lval_fun(lbuiltin func)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->fun = func;
	return v;
}

/* Function Prototypes */
void lval_del(lval* v);
void lenv_del(lenv* e);
lval* lval_add(lval* v, lval* x);
lval* lval_pop(lval* v, int i);
lval* lval_take(lval* v, int i);

void lval_print(lval* v);
void lval_expr_print(lval* v, char open, char close);
void lval_println(lval* v) {lval_print(v); putchar('\n');}


lenv* lenv_new(void);
void lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);

lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_def(lenv* e, lval* a);

lval* lval_join(lval* x, lval* y);
lval* builtin_join(lenv* e, lval* a);
lval* builtin(lenv* e, lval* a, char* func);


lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_eval(lenv* e, lval* v);
lval* lval_eval_sexpr(lenv* e, lval* v);

int main(int argc, char** argv)
{

	/* create some parsers */
	mpc_parser_t* Number	= mpc_new("number");
	mpc_parser_t* Symbol	= mpc_new("symbol");
	mpc_parser_t* Sexpr		= mpc_new("sexpr");
	mpc_parser_t* Qexpr		= mpc_new("qexpr");
	mpc_parser_t* Expr		= mpc_new("expr");
	mpc_parser_t* tLisp		= mpc_new("tlisp");

	/* define them with the following language */
	mpca_lang(MPCA_LANG_DEFAULT,									
		"															\
			number	: /-?[0-9]+/ ;									\
	    	symbol	: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;			\
			sexpr	: '(' <expr>* ')' ;								\
			qexpr	: '{' <expr>* '}' ;								\
			expr	: <number> | <symbol> | <sexpr> | <qexpr> ;		\
			tlisp	: /^/ <expr>* /$/ ;								\
		", Number, Symbol, Sexpr, Qexpr, Expr, tLisp);


	/* print version and exit information */
	puts("tlisp version 0.0.0.0.7");
	puts("press ctrl+c to exit\n");

	lenv* e = lenv_new();
	lenv_add_builtins(e);
	/* infinite loop */
	for (;;)
	{
		/* output our prompt */
		char* input = readline("tlisp> ");
		add_history(input); 

		mpc_result_t r;
		if (mpc_parse("<stdin>", input, tLisp, &r)) 
		{
			lval* x = lval_eval(e, lval_read(r.output));
			lval_println(x);
			lval_del(x);

			mpc_ast_delete(r.output);
		} else {
			/* otherwise print the error */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}
		/* free retrieved input */
		free(input);
	}
	lenv_del(e);

	return 0;
}

void lval_del(lval* v)
{
	switch (v->type) {
		/* do nothing special for num type */
		case LVAL_NUM: break;

		/* for err or sym free the string data */
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
		case LVAL_FUN: break;
		/* if sexpr or qexpr then delete all elements inside */
		case LVAL_QEXPR:
		case LVAL_SEXPR:
			for (int i = 0; i < v->count; i++)
			{
				lval_del(v->cell[i]);
			}
			/* also free the memory allocated to contain the pointers */
			free(v->cell);
		break;
	}

	/* free the memory allocated for the "lval" struct itself */
	free(v);
}

lval* lval_add(lval* v, lval* x)
{
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_pop(lval* v, int i)
{
	/* find the item at "i" */
	lval* x = v->cell[i];

	/* shift memory after the item at "i" over the top */
	memmove(&v->cell[i], &v->cell[i+1],
	  sizeof(lval*) * (v->count-i-1));

	/* decrease the count of items in the list */
	v->count--; 

	/* reallocate the memory used */
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i)
{
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

void lval_expr_print(lval* v, char open, char close)
{
	putchar(open);
	for (int i = 0; i < v->count; i++)
	{
		/* print value within */
		lval_print(v->cell[i]);

		if (i != (v->count-1)) {
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print(lval* v) 
{
	switch (v->type) 
	{
		/* in the case the type is a number print it */
		/* then 'break' out of the switch */
		case LVAL_NUM:		printf("%li", v->num); break;
		case LVAL_ERR:		printf("error: %s", v->err); break;
		case LVAL_SYM:		printf("%s", v->sym); break;
		case LVAL_FUN:		printf("<function>"); break;
		case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
		case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;

	}
}

char* ltype_name(int t)
{
	switch(t) {
		case LVAL_FUN: return "Function";
		case LVAL_NUM: return "Number";
		case LVAL_ERR: return "Error";
		case LVAL_SYM: return "Symbol";
		case LVAL_SEXPR: return "S-Expression";
		case LVAL_QEXPR: return "Q-Expression";
		default: return "Unknown";
	}
}


lval* lval_join(lval* x, lval* y)
{
	while (y->count)
	{
		x = lval_add(x, lval_pop(y, 0));
	}
	/* Delete the empty 'y' and return 'x' */
	lval_del(y);
	return x;
}

lval* lval_copy(lval* v)
{
	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch (v->type)
	{
		/* Copy Funcs and Nums directly */
		case LVAL_FUN: x->fun = v->fun; break;
		case LVAL_NUM: x->num = v->num; break;
		/* Copy Strings using malloc & strcpy */
		case LVAL_ERR:
				x->err = malloc(strlen(v->err) + 1);
				strcpy(x->err, v->err); break;
		case LVAL_SYM:
				x->sym = malloc(strlen(v->sym) + 1);
				strcpy(x->sym, v->sym); break;
		/* Copy Lists by copying each sub-expression */
		case LVAL_SEXPR:
		case LVAL_QEXPR:
				x->count = v->count;
				x->cell = malloc(sizeof(lval*) * x->count);
				for (int i = 0; i < x->count; i++)
				{
					x->cell[i] = lval_copy(v->cell[i]);
				}
		break;
	}
	return x;
}

/* Lisp Environment */
lenv* lenv_new(void)
{
	lenv* e = malloc(sizeof(lenv));
	e->count = 0;
	e->syms = NULL;
	e->vals = NULL;
	return e;
}

void lenv_del(lenv* e)
{
	for (int i=0; i < e->count; i++)
	{
		free(e->syms[i]);
		lval_del(e->vals[i]);
	}
	free(e->syms);
	free(e->vals);
	free(e);
}

lval* lenv_get(lenv* e, lval* k)
{
	/* Iterate over all items in environment */
	for (int i=0; i < e->count; i++)
	{
		/* Check if the stored string matches the symbol string */
		/* if it does, return a copy of the value */
		if (strcmp(e->syms[i], k->sym) == 0) {
			return lval_copy(e->vals[i]);
		}
	}
	/* If no sym found return error */
	return lval_err("Unbound Symbol '%s'", k->sym);
}

void lenv_put(lenv* e, lval* k, lval* v)
{
	for (int i=0; i < e->count;i++)
	{
		/* If variable is found del item at that pos */
		/* if it does, return a copy of the value */
		if (strcmp(e->syms[i], k->sym) == 0) {
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return ;
		}
	}

	/* if no existing entry found allocate space for new entry */
	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->syms = realloc(e->syms, sizeof(char*) * e->count);

	/* copy contents of lval and symbol string into new location */
	e->vals[e->count-1] = lval_copy(v);
	e->syms[e->count-1] = malloc(strlen(k->sym)+1);
	strcpy(e->syms[e->count-1], k->sym);
}

/* Builtins */
lval* builtin_list(lenv* e, lval* a) 
{
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_head(lenv* e, lval* a)
{
	/* check for error conditions */
	LASSERT_NUM("head", a, 1);
	LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("head", a, 0);
	
	/* otherwise take first argument */
	lval* v = lval_take(a, 0);
	/* delete all elements that are not head an return */
	while (v->count > 1) {lval_del(lval_pop(v, 1));}
	return v;
}

lval* builtin_tail(lenv* e, lval* a)
{
	/* check for error conditions */
	LASSERT_NUM("tail", a, 1);
	LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("tail", a, 0);
	/* take first argument */
	lval* v = lval_take(a, 0);
	/* delete first element and return */
	lval_del(lval_pop(v, 0));
	return v;
}

lval* builtin_eval(lenv* e, lval* a)
{
	LASSERT(a, a->count == 1,
			"Function 'eval' passed too many arguments.");
	LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
			"Function 'eval' passed incorrect type.");

	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a)
{
	for (int i = 0; i < a->count; i++)
	{
		LASSERT_TYPE("join", a, i, LVAL_QEXPR);
	}

	lval* x = lval_pop(a, 0);

	while (a->count)
	{
		lval* y = lval_pop(a, 0);
		x = lval_join(x, y);
	}

	lval_del(a);
	return x;
}

lval* builtin_op(lenv* e, lval* a, char* op)
{
	/* ensure all arguments are numbers */
	for (int i = 0; i < a->count; i++)
	{
		LASSERT_TYPE(op, a, i, LVAL_NUM);
	}
	/* pop first element */
	lval* x = lval_pop(a, 0);
	/* if no arguments and sub then perform unary negation */
	if ((strcmp(op, "-") == 0) && a->count == 0) {
		x->num = -x->num;
	}

	/* while elements are still remaining */
	while (a->count > 0)
	{
		/* pop the next element */
		lval* y = lval_pop(a, 0);

		if (strcmp(op, "+") == 0) {x->num += y->num;}
		if (strcmp(op, "-") == 0) {x->num -= y->num;}
		if (strcmp(op, "*") == 0) {x->num *= y->num;}
		if (strcmp(op, "/") == 0) {
			/* if  second operand is zero return error */
			if (y->num == 0) {
				lval_del(x); lval_del(y);
				x = lval_err("division by zero!"); 
				break;
			}
			x->num /= y->num;
		  }
		lval_del(y);
	}
	
	lval_del(a); 
	return x;
}

lval* builtin_add(lenv* e, lval* a)
{
	return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a)
{
	return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a)
{
	return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a)
{
	return builtin_op(e, a, "/");
}

lval* builtin_def(lenv* e, lval* a)
{
	LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

	/* First argument is symbol list */
	lval* syms = a->cell[0];

	/* Ensure all elements of first list are symbols */
	for (int i = 0; i < syms->count; i++)
	{
		LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
			"Function 'def' cannot define non-symbol"
			"Got %s, expected %s.", 
			ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
	}

	/* Check correct number of symbols and values */
	LASSERT(a, (syms->count == a->count-1), 
		"Function 'def' passed too many arguments for symbols. "
		"Got %i, Expected %i.",
		syms->count, a->count-1);

	/* Assign copies of values to symbols */
	for (int i = 0; i < syms->count; i++)
	{
		lenv_put(e, syms->cell[i], a->cell[i+1]);
	}

	lval_del(a);
	return lval_sexpr();
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func)
{
	lval* k = lval_sym(name);
	lval* v = lval_fun(func);
	lenv_put(e, k, v);
	lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e)
{
	/* List Functions */
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);

	/* Mathematical Functions */
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);

	/* Variable Functions */
	lenv_add_builtin(e, "def", builtin_def);
}

/* Eval */
lval* lval_eval_sexpr(lenv* e, lval* v) 
{
	/* eval children */
	for (int i = 0; i < v->count; i++)
	{
		v->cell[i] = lval_eval(e, v->cell[i]);
	}
	/* error checking */
	for (int i = 0; i < v->count; i++)
	{
		if (v->cell[i]->type == LVAL_ERR) {return lval_take(v, i);}
	}
	/* empty expression */
	if (v->count == 0) {return v;}
	/* single expression */
	if (v->count == 1) {return lval_take(v, 0);}
	/* ensure first element is a function after evaluation */
	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_FUN) {
		lval_del(v); lval_del(f);
		return lval_err("first element is not in function");
	}
	/* if so call function to get result */
	lval* result = f->fun(e, v);
	lval_del(f);
	return result;
}

lval* lval_eval(lenv* e, lval* v)
{
	/* evaluate sexpressions */
	if (v->type == LVAL_SYM) {
		lval* x = lenv_get(e, v);
		lval_del(v);
		return x;
	}
	if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }
	return v;
}

/* Read */
lval* lval_read_num(mpc_ast_t* t)
{
	errno = 0; 
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ?
		lval_num(x) : lval_err("invalid numuber");
}

lval* lval_read(mpc_ast_t* t)
{
	/* if symbol or number return conversion to that type */
	if (strstr(t->tag, "number")) {return lval_read_num(t);}
	if (strstr(t->tag, "symbol")) {return lval_sym(t->contents);}

	/* if root (>) or sexpr then creat empty list */
	lval* x = NULL;
	if (strcmp(t->tag, ">") == 0) {x = lval_sexpr();}
	if (strstr(t->tag, "sexpr"))  {x = lval_sexpr();}
	if (strstr(t->tag, "qexpr"))  {x = lval_qexpr();}

	for (int i = 0; i < t->children_num; i++)
	{
		if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
		if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
		if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
		if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
	}
	return x;
}