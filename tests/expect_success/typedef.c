
typedef long int li_t;

static li_t li;

typedef void (*fptr_t)(void);

fptr_t fptr, *fptrptr;

typedef void (func_t)(void);

func_t *funcptr;

typedef struct recur {
	struct recur *x;
} recur;

recur recurinstance;
