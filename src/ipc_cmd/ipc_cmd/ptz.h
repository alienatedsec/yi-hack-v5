#define V5               1
#define MSTAR            2
#define ALLWINNER        3
#define ALLWINNER_V2     4
#define ALLWINNER_V2_ALT 5

int is_ptz(char *model);
int get_model_suffix(char *model, int size);
int read_ptz(int *px, int *py);
