#include <unistd.h>
#include "stdlib.h"

#include "physical_layer.h"
#include "timeval_operators.h"

using namespace std;

// Impair --------------------------------------------------------
Impair::Impair()
{
}

Impair::Impair(
 double drop0[],unsigned int drop_length0,
 double corrupt0[],unsigned int corrupt_length0,
 unsigned int delay0)
{
	// check for exceptions
	if (drop_length0 > MAXIMUM_IMPAIR_LENGTH ||
	 corrupt_length0 > MAXIMUM_IMPAIR_LENGTH) {
		throw Physical_layer_exception();
	}
	for (unsigned int i = 0; i < drop_length0; i++) {
		if (!(0.0 <= drop0[i] && drop0[i] <= 1.0)) {
			throw Physical_layer_exception();
		}
	}
	for (unsigned int i = 0; i < corrupt_length0; i++) {
		if (!(0.0 <= corrupt0[i] && corrupt0[i] <= 1.0)) {
			throw Physical_layer_exception();
		}
	}

	// copy drop parameters; initialize drop_index
	drop_length = drop_length0;
	if (drop_length > 0) {
		for (unsigned int i = 0; i < drop_length; i++ ) {
			drop[i]= drop0[i];
		}
		drop_index = 0;
	}

	// copy corrupt parameters; initialize corrupt_index
	corrupt_length = corrupt_length0;
	if (corrupt_length > 0) {
		for (unsigned int i = 0; i < corrupt_length; i++ ) {
			corrupt[i]= corrupt0[i];
		}
		corrupt_index = 0;
	}

	// convert delay from microseconds to timeval
	delay.tv_sec = delay0 / 1000000;
	delay.tv_usec = delay0 % 1000000;

	rand_state = 1;
}

bool Impair::drop_packet(void)
{
	// no drop from empty drop array
	if (drop_length == 0) {
		return false;
	}

	// get random int, scale to [0..1], compare to drop element
	int r = rand_r(&rand_state);
	return ((double)r / (double)RAND_MAX) < drop[drop_index];
}

bool Impair::corrupt_packet(unsigned char buffer[],unsigned int length)
{
	// no corruption from empty corrupt array
	if (corrupt_length == 0) {
		return false;
	}

	// get random int, scale to [0..1], compare to corrupt element
	int r = rand_r(&rand_state);
	if (((double)r / (double)RAND_MAX) < corrupt[corrupt_index]) {
		// get random int, scale i to [0..length-1], corrupt buffer[i]
		int i = rand_r(&rand_state) % length;
		buffer[i] ^= 0x01; // corrupt: invert low-order bit
		return true;
	}

	return false;
}

struct timeval Impair::get_delay(void)
{
	return delay;
}

void Impair::next(void)
{
	if (drop_length != 0) {
		drop_index = (drop_index+1) % drop_length;
	}
	if (corrupt_length != 0) {
		corrupt_index = (corrupt_index+1) % corrupt_length;
	}
}

// Physical_layer_interface  --------------------------------------------

Physical_layer_interface::Physical_layer_interface(
 Physical_layer *physical_layer_p0,Impair &impair_p0,
 void (*send_log0)(char,unsigned char[],unsigned int,bool,bool),
 void (*receive_log0)(char,unsigned char[],unsigned int))
{
	impair = Impair(impair_p0);
	physical_layer_p = physical_layer_p0;

	send_log = send_log0;
	receive_log = receive_log0;

	buffer_length = 0;
}

int Physical_layer_interface::send(unsigned char buffer[],unsigned int length)
{
	// ensure length is safe to use
	if (length == 0 || length > MAXIMUM_BUFFER_LENGTH) {
		throw Physical_layer_exception();
	}

	// delegate to the appropriate interface
	int n;
	if (this == physical_layer_p->get_a_interface()) {
		n = send(buffer,length,
		 physical_layer_p->get_a_interface(),
		 physical_layer_p->get_b_interface());
	} else {
		n = send(buffer,length,
		 physical_layer_p->get_b_interface(),
		 physical_layer_p->get_a_interface());
	}

	return n;
}

int Physical_layer_interface::send
 (unsigned char send_buffer[],unsigned int send_buffer_length,
 Physical_layer_interface *send_interface,
 Physical_layer_interface *receive_interface)
{
	physical_layer_p->lock_buffers(); // ***** LOCK
	// return if device busy
	if (receive_interface->buffer_length > 0) {
		physical_layer_p->unlock_buffers(); // ***** UNLOCK
		return 0;
	}

	// copy send_buffer
	for (unsigned int i = 0; i < send_buffer_length; i++) {
		receive_interface->buffer[i] = send_buffer[i];
	}
	receive_interface->buffer_length = send_buffer_length;

	// apply impairment
	receive_interface->buffer_is_corrupted =
	 send_interface->impair.corrupt_packet
	 (receive_interface->buffer,send_buffer_length);
	receive_interface->buffer_will_be_dropped =
	 send_interface->impair.drop_packet();

	// compute release time
	gettimeofday(&receive_interface->buffer_release_time,NULL);
	receive_interface->buffer_release_time +=
	 send_interface->impair.get_delay();

	send_interface->impair.next(); // for next send

	// handle send logging
	if (send_log != NULL) {
		char side;
		if (this == physical_layer_p->get_a_interface()) {
			side = 'a';
		} else {
			side = 'b';
		}
		send_log(side,
		 receive_interface->buffer,receive_interface->buffer_length,
		 receive_interface->buffer_will_be_dropped,
		 receive_interface->buffer_is_corrupted);
	}

	// set length to force drop
	if (receive_interface->buffer_will_be_dropped) {
		receive_interface->buffer_length = 0;
	}
	physical_layer_p->unlock_buffers(); // ***** UNLOCK

	return send_buffer_length;
}

unsigned int Physical_layer_interface::receive(unsigned char buffer[])
{
	int n;

	if (this == physical_layer_p->get_a_interface()) {
		n = receive(buffer,physical_layer_p->get_a_interface());
	} else {
		n = receive(buffer,physical_layer_p->get_b_interface());
	}
	return n;
}

int Physical_layer_interface::receive(unsigned char receive_buffer[],
 Physical_layer_interface *interface)
{
	struct timeval now;
	int length;

	gettimeofday(&now,NULL);

	physical_layer_p->lock_buffers(); // ***** LOCK
	if (interface->buffer_length > 0 &&
	 interface->buffer_release_time < now) {
		if (interface->buffer_will_be_dropped) {
			length = 0; // no data to return
		} else {
			// copy buffer
			for (unsigned int i = 0;
			 i < interface->buffer_length; i++) {
				receive_buffer[i] = interface->buffer[i];
			}
			length = interface->buffer_length;
		}
		interface->buffer_length = 0; // release buffer
	} else {
		length = 0; // no data to return
	}
	if (interface->receive_log != NULL && length > 0) {
		char side;
		if (this == physical_layer_p->get_a_interface()) {
			side = 'a';
		} else {
			side = 'b';
		}
		interface->receive_log(side,receive_buffer,length);
	}
	physical_layer_p->unlock_buffers(); // ***** UNLOCK

	return length;
}

// Physical_layer --------------------------------------------------------

Physical_layer::Physical_layer(Impair &a_impair,Impair &b_impair,
 void (*send_log)(char,unsigned char[],unsigned int,bool,bool),
 void (*receive_log)(char,unsigned char[],unsigned int))
{
	a_interface = new Physical_layer_interface(this,a_impair,
	 send_log,receive_log);
	b_interface = new Physical_layer_interface(this,b_impair,
	 send_log,receive_log);

	pthread_mutex_init(&buffer_mutex,NULL);
}

Physical_layer_interface* Physical_layer::get_a_interface()
{
	return a_interface;
}
Physical_layer_interface* Physical_layer::get_b_interface()
{
	return b_interface;
}
void Physical_layer::lock_buffers(void)
{
	pthread_mutex_lock(&buffer_mutex);
}
void Physical_layer::unlock_buffers(void)
{
	pthread_mutex_unlock(&buffer_mutex);
}
