
// my definitions

// this is how arguments are passed to threads, via a generic struct
typedef struct arg_struct {
    int id;
    int mode;
    int start;
    int end;
} args_for_thread;

#define NUMBERS 1000

// my functions
void parse_args(int argc, char **argv);
void bubble_sort(int start,int end,int mode);
void merge(int starta,int enda,int startb,int endb);
void worker(void *arguments);

