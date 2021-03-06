/****************************************************************************
 *
 *   Copyright (c) 2020 unmanned-technologies.de. All rights reserved.
 *   Author:  Stefan Arbes <info@unmanned-technologies.de>
 *
 *   based partly on the work of the MAVlink Development Team.
 *           Trent Lukaczyk, <aerialhedgehog@gmail.com>
 *           Jaycee Lock,    <jaycee.lock@gmail.com>
 *           Lorenz Meier,   <lm@inf.ethz.ch>
 *           Hannes Diethelm, <hannes.diethelm@gmail.com> 
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name unmanned-technologies.de nor the names of its 
 *    contributors may be used to endorse or promote products derived 
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

// ----------------------------------------------------------------------------------
//   TrackerFilter Interface Class
// ----------------------------------------------------------------------------------
#include "trackerfilter_interface.h"

// ------------------------------------------------------------------------------
//   Con/De structors
// ------------------------------------------------------------------------------
TrackerFilter_Interface::
TrackerFilter_Interface(Generic_Port *source_port_, Generic_Port *dest_port_)
{
	// initialize attributes
	reading_status = 0;      // whether the read thread is running
	time_to_exit   = false;  // flag to signal thread exit

	read_tid  = 0; // read thread id

	source_port = source_port_; // port management object
	dest_port = dest_port_; // port management object

}

TrackerFilter_Interface::
~TrackerFilter_Interface()
{}

// ------------------------------------------------------------------------------
//   Read Messages
// ------------------------------------------------------------------------------
void
TrackerFilter_Interface::
read_messages()
{
	bool success;               // receive success flag

	while ( !time_to_exit )
	{
		mavlink_message_t message;
		success = source_port->read_message(message);
		if( success )
		{
			
			mavlink_status_t* chan_state = mavlink_get_channel_status(MAVLINK_COMM_0);
			chan_state->flags |= MAVLINK_STATUS_FLAG_OUT_MAVLINK1;

//			message.magic = MAVLINK_STX_MAVLINK1;
			switch (message.msgid)
			{

				case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
				{
//					printf("MAVLINK_MSG_ID_GLOBAL_POSITION_INT\n");
					dest_port->write_message(message);
					break;
				}

				case MAVLINK_MSG_ID_GPS_RAW_INT:
				{
//					printf("MAVLINK_MSG_ID_GPS_RAW_INT\n");
					dest_port->write_message(message);
					break;
				}

				case MAVLINK_MSG_ID_ATTITUDE:
				{
//					printf("MAVLINK_MSG_ID_ATTITUDE\n");
					dest_port->write_message(message);
					break;
				}

				case MAVLINK_MSG_ID_HEARTBEAT:
				{
					get_own_ip();
					string my_out_string = "TESTZEILE mit 20 Z.." + my_own_ip;
//					const char *cstr = "TESTZEILE mit 20 Z.." + my_own_ip.c_str();
					const char *cstr =my_out_string.c_str();
					mavlink_msg_statustext_pack (0, 0, &message, 1, cstr);
					dest_port->write_message(message);
				}

				default:
				{
					break;
				}
			}
		}
	}
	return;
}


// ------------------------------------------------------------------------------
//   STARTUP
// ------------------------------------------------------------------------------
void
TrackerFilter_Interface::
start()
{
	int result;

	if ( !source_port->is_running() ) 
	{
//		fprintf(stderr,"ERROR: source port not open\n");
		cerr << "ERROR: source port not open" << endl;
		throw 1;
	}

	if ( !dest_port->is_running() )
	{
//		fprintf(stderr,"ERROR: destination source port not open\n");
		cerr << "ERROR: destination port not open" << endl;
		throw 1;
	}


	cout << "Starting filter thread" << endl;

	result = pthread_create( &read_tid, NULL, &start_TrackerFilter_interface_read_thread, this );
	if ( result ) throw result;
	return;
}


// ------------------------------------------------------------------------------
//   SHUTDOWN
// ------------------------------------------------------------------------------
void
TrackerFilter_Interface::
stop()
{
	cout << "CLOSE THREADS" << endl;
	time_to_exit = true;
	pthread_join(read_tid ,NULL);

}

// ------------------------------------------------------------------------------
//   Start of Filter Thread
// ------------------------------------------------------------------------------
void
TrackerFilter_Interface::
start_read_thread()
{

	if ( reading_status != 0 )
	{
		cerr << "read thread already running" << endl;
		return;
	}
	else
	{
		read_thread();
		return;
	}

}

// ------------------------------------------------------------------------------
//   Quit Handler
// ------------------------------------------------------------------------------
void
TrackerFilter_Interface::
handle_quit( int sig )
{

	try {
		stop();

	}
	catch (int error) {
		cerr << "Warning, could not stop TrackerFilter interface" << endl;
	}

}


// ------------------------------------------------------------------------------
//   Read Thread
// ------------------------------------------------------------------------------
void
TrackerFilter_Interface::
read_thread()
{
	reading_status = true;

	while ( ! time_to_exit )
	{
		read_messages();
	}

	return;
}

// ------------------------------------------------------------------------------
//   Own IP 
// ------------------------------------------------------------------------------
void
TrackerFilter_Interface::
get_own_ip()
{
	int sock_fd = socket( AF_INET , SOCK_DGRAM , 0 );
    if( sock_fd == -1 ) 
		{ 
			cerr << "Error: creating socket."; 
		} else
			{
    			struct ifreq ifr;
    			strcpy( ifr.ifr_name , "eth0" );
    			if( ioctl( sock_fd , SIOCGIFADDR , &ifr ) == -1 ) 
					{ 
						cerr << "problems with ioctl.";
					} else
						{
    						my_own_ip = inet_ntoa( ((struct sockaddr_in *) (&ifr.ifr_addr))->sin_addr );
    						close( sock_fd );
						}		
	}
}


// End TrackerFilter_Interface

// ------------------------------------------------------------------------------
//  Pthread Starter Helper Functions
// ------------------------------------------------------------------------------

void*
start_TrackerFilter_interface_read_thread(void *args)
{
	// takes an TrackerFilter object argument
	TrackerFilter_Interface *TrackerFilter_interface = (TrackerFilter_Interface *)args;

	// run the object's read thread
	TrackerFilter_interface->start_read_thread();

	// done!
	return NULL;
}

