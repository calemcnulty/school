#include "link_layer.h"
#include "timeval_operators.h"

using namespace std;

unsigned short checksum(struct Packet);
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

Link_layer::Link_layer(Physical_layer_interface* physical_layer_interface, unsigned int num_sequence_numbers, unsigned int max_send_window_size,unsigned int timeout)
{
	this->physical_layer_interface = physical_layer_interface;

	receive_buffer_length = 0;
	send_queue = new struct Timed_packet* [max_send_window_size];
	
	// initialize send_queue to NULLs
	for (unsigned int i = 0; i < max_send_window_size; i++) {
		send_queue[i] = NULL;
	}

	if (pthread_create(&thread,NULL,&Link_layer::loop,this) < 0) {
		throw Link_layer_exception();
	}
}

unsigned int Link_layer::send(unsigned char buffer[],unsigned int length)
{
		
	if (length <= 0 || length > MAXIMUM_DATA_LENGTH) {
		throw Link_layer_exception();
	} 

	pthread_mutex_lock(&mutex);
	if (send_queue[max_send_window_size-1] != NULL) {
		struct Timed_packet *p = new Timed_packet;
		gettimeofday(&(p->send_time), NULL);
		p->packet.header.data_length = length;
		p->packet.header.seq = next_send_seq;

		unsigned int n = physical_layer_interface->send(buffer,length);
		pthread_mutex_unlock(&mutex);

		return n;
	} else {
		return 0;
	}
}

unsigned int Link_layer::receive(unsigned char buffer[])
{
	unsigned int length;
	pthread_mutex_lock(&mutex);
	length = receive_buffer_length;
	if (length > 0) {
		for (unsigned int i = 0; i < length; i++) {
			buffer[i] = receive_buffer[i];
		}
		receive_buffer_length = 0;
	}
	pthread_mutex_unlock(&mutex);
	return length;
}

void Link_layer::process_received_packet(struct Packet p)
{
	Packet_header *h = &(p.header);
	if (h->seq == next_receive_seq)
	{
		if (h->data_length > 0 && receive_buffer_length == 0)
		{
			for (unsigned int i = 0; i < MAXIMUM_DATA_LENGTH; i++)
			{
				if (i < h->data_length)
				{
					receive_buffer[i] = p.data[i];
					receive_buffer_length++;
				}
				else receive_buffer[i] = 0;
			}
			next_receive_seq = ++next_receive_seq % num_sequence_numbers;
		}
	}
	last_receive_ack = h->ack;
}

void Link_layer::remove_acked_packets()
{
	int found = -1;
	for (unsigned int i = 0; i < max_send_window_size && send_queue[i]; i++)
	{
		if (send_queue[i]->packet.header.ack == last_receive_ack)
		{
			found = i;
		}
	}
	if (found == -1) return; // none found
	for (unsigned int f = 0; f < max_send_window_length; f++)
	{
		if (f < found)
			delete send_queue[f];
			if (f + found < max_send_window_length) send_queue[f] = send_queue[f + found];
			else send_queue[f] = NULL;
	}
}

void Link_layer::send_timed_out_packets()
{
	timeval timed_out;
	gettimeofday(&timed_out, NULL);
	timed_out -= timeout;
	for (unsigned int i = 0; i < send_queue_length; i++)
	{
		if (!send_queue[i]) break;
		if (send_queue[i]->send_time > timed_out) continue;
		Timed_packet *tp = send_queue[i];
		Packet_header *h = &(tp->packet.header);
		h->ack = next_receive_seq;
		h->checksum = checksum(tp->packet);
		if (physical_layer_interface->send((unsigned char*)send_queue[i], h->data_length))
			gettimeofday(&(tp->send_time), NULL);
	}
}

void Link_layer::generate_ack_packet()
{
	if (send_queue[0]) return;
	struct Packet* p = new struct Packet();
	struct Packet_header *h = &(p->header);	
	
	h->ack = next_receive_seq - 1;
	h->checksum = checksum(*p);
	
}

void* Link_layer::loop(void* thread_creator)
{
	const unsigned int LOOP_INTERVAL = 10;
	Link_layer* link_layer = ((Link_layer*) thread_creator);

	while (true) {
        
        pthread_mutex_lock(&mutex);
		if (link_layer->receive_buffer_length == 0) {
            
			unsigned int length =
			 link_layer->physical_layer_interface->receive
			 (link_layer->receive_buffer);

			if (length > 0) {
				link_layer->receive_buffer_length = length;
			}
		}
        link_layer->process_received_packet(
         *((struct Packet*)(link_layer->receive_buffer)));
        
		pthread_mutex_unlock(&mutex);

		usleep(LOOP_INTERVAL);
	}

	return NULL;
}

// this is the standard Internet checksum algorithm
unsigned short checksum(struct Packet p)
{
	unsigned long sum = 0;
	struct Packet copy;
	unsigned short* shortbuf;
	unsigned int length;

	if (p.header.data_length > Link_layer::MAXIMUM_DATA_LENGTH) {
		throw Link_layer_exception();
	}

	copy = p;
	copy.header.checksum = 0;
	length = sizeof(Packet_header)+copy.header.data_length;
	shortbuf = (unsigned short*) &copy;

	while (length > 1) {
		sum += *shortbuf++;
		length -= 2;
	}
	// handle the trailing byte, if present
	if (length == 1) {
		sum += *(unsigned char*) shortbuf;
	}

	sum = (sum >> 16)+(sum & 0xffff);
	sum = (~(sum+(sum >> 16)) & 0xffff);
	return (unsigned short) sum;
}
