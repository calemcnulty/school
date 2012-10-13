#include <iostream>
#include <stdlib.h>
#include <stdio.h>

#include "link_layer.h"

#include "test_configuration.cpp"

using namespace std;

void send_n(Link_layer *send_link_layer,Link_layer *receive_link_layer,int n)
{
	unsigned char send_buffer
	 [Physical_layer_interface::MAXIMUM_BUFFER_LENGTH];
	unsigned char receive_buffer
	 [Physical_layer_interface::MAXIMUM_BUFFER_LENGTH];

	int send_count = 0;
	int receive_count = 0;

	send_buffer[0] = 0;
	while (send_count < n || receive_count < n) {
		int n0 = send_link_layer->send(send_buffer,1);
		if (send_count < n && n0 == 1) {
			cout << "send:" << (int)send_buffer[0] << endl;
			send_buffer[0]++;
			send_count++;
		}

		n0 = receive_link_layer->receive(receive_buffer);
		if (n0 > 0) {
			cout << "\treceive:" << (int)receive_buffer[0] << endl;
			receive_count++;
		}
	}
}

Physical_layer physical_layer(a_impair,b_impair,NULL,NULL);

Link_layer* a_link_layer = new Link_layer
 (physical_layer.get_a_interface(),num_seq,max_win,timeout);

Link_layer* b_link_layer = new Link_layer
 (physical_layer.get_b_interface(),num_seq,max_win,timeout);

int main(int argc,char* argv[])
{
	if (argc != 3) {
		cout << "Syntax: " << argv[0] <<
		 " a_to_b_count b_to_a_count" << endl;
		exit(1);
	}

	cout << "----- a to b..." << endl;
	send_n(a_link_layer,b_link_layer,atoi(argv[1]));

	cout << "----- b to a..." << endl;
	send_n(b_link_layer,a_link_layer,atoi(argv[2]));

	return 0;
}
