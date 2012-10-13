#include <iostream>
#include <pthread.h>
#include "sys/time.h"

using namespace std;

class Physical_layer;

// Physical_layer_exception ---------------------------------------------

class Physical_layer_exception: public exception {
};

// Impair ---------------------------------------------------------------

class Impair {
public:
	enum {MAXIMUM_IMPAIR_LENGTH = 50};

	Impair();
	Impair(double*,unsigned int,double*,unsigned int,unsigned int);

	bool drop_packet(void);
	bool corrupt_packet(unsigned char buffer[],unsigned int length);
	struct timeval get_delay(void);
	void next(void);
private:
	double drop[MAXIMUM_IMPAIR_LENGTH];
	unsigned int drop_length,drop_index;

	double corrupt[MAXIMUM_IMPAIR_LENGTH];
	unsigned int corrupt_length,corrupt_index;

	struct timeval delay;
	unsigned int rand_state;
};

// Physical_layer_interface ----------------------------------------------

class Physical_layer_interface {
public:
	enum {MAXIMUM_BUFFER_LENGTH = 100};

	Physical_layer_interface(Physical_layer*,Impair&,
	 void (*send_log)(char,unsigned char[],unsigned int,bool,bool),
	 void (*receive_log)(char,unsigned char[],unsigned int));

	unsigned int receive(unsigned char buffer[]);

	int send(unsigned char buffer[],unsigned int length);
private:
	int send(unsigned char[],unsigned int,
	 Physical_layer_interface*,Physical_layer_interface*);

	void (*send_log)(char,unsigned char[],unsigned int,bool,bool);
	void (*receive_log)(char,unsigned char[],unsigned int);

	int receive(unsigned char[],Physical_layer_interface*);

	Physical_layer *physical_layer_p;
	Impair impair;

	// buffer state
	unsigned int buffer_length; // ignore other fields if buffer_length is 0
	unsigned char buffer[MAXIMUM_BUFFER_LENGTH];
	struct timeval buffer_release_time;
	bool buffer_is_corrupted,buffer_will_be_dropped;
};

// Physical_layer --------------------------------------------------------

class Physical_layer {
public:
	Physical_layer(Impair&,Impair&,
	 void (*send_log)(char,unsigned char[],unsigned int,bool,bool),
	 void (*receive_log)(char,unsigned char[],unsigned int));

	Physical_layer_interface *get_a_interface(void);

	Physical_layer_interface *get_b_interface(void);

	void lock_buffers();
	void unlock_buffers();
private:
	pthread_mutex_t buffer_mutex;

	Physical_layer_interface *a_interface,*b_interface;
};
