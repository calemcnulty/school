// Link layer parameters
unsigned int timeout = 100000;
unsigned int max_win = 1;
unsigned int num_seq = 2;

// Physical Layer parameters
// const int PHYSICAL_LAYER_DELAY = 10;
const int PHYSICAL_LAYER_DELAY = 0;

Impair a_impair(NULL,0, NULL,0, PHYSICAL_LAYER_DELAY);

Impair b_impair(NULL,0, NULL,0, PHYSICAL_LAYER_DELAY);
