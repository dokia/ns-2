//#ifndef DASH-EXP_H
//#define DASH-EXP_H

#include "app.h"
#include "tcp-full.h"
#include <random.h>
#include <tcl.h>
#include <vector>
/*
class MPD {
public:
	MPD();
	int get_packetsize(int seg_num_, int bitrate);
private:
	vector<vector<int> > packet_size;
	vector<int> bitrate_list;
	int max_seg_num_;
};*/
/*
class ReadBufferEvent : public Event {
public:
	ReadBufferEvent (double & buffer, double read; double interval, double threshold) {
	    buffer_ = buffer; 
	    read_ = read;
	    interval_ = interval;
	    threshold_ = threshold;
	};
	read_action () {
	    if (*buffer_ > threshold_)
		*buffer_ -= read_;
	}
	double & buffer_; // read the client buffer;
	double read_;	  // amount of read;
	double interval_; // the interval for the reading;
	double threshold_;// the threshold for buffer reading
			  // should larger than read_;
};

class ReadBufferHandler : public Handler {
public:
	void handle(Event* event) {
	    ReadBufferEvent* rb = (ReadBufferEvent*)event;
	    rb->read_action();
	};
	
} read_buffer_handler;
*/

typedef unsigned int tran_t;

static const tran_t TRAN_BEGIN = 0;
static const tran_t TRAN_PAUSE = 1;
static const tran_t TRAN_OVER = 2;


class DashAppClient : public Application {
public:
	DashAppClient();
protected:
	int command(int argc, const char*const* argv);
	void recv(int nbytes);
	void start();
	void request_mpd();	
	
	void process_mpd();
	int qoe_cal(double interval, int nbytes);
	void request_first_seg();
	void request_next_seg(int seqno_, int bitrate_);
	Agent* getAgent() {return agent_;}

	int seqno_;	/* the segment sequence number 
			 * received, start from 1
			 */
	int data_type_; // 1 for media and 0 for mpd
	double send_time;
	double recv_time;
	double buffer_;	// buffer len in seconds
	// this two value is for the data segment in the tcp
	// transmission
	int reqBytes_;
	int revBytes_;
	
	// buffer read
	double buffer_start_; // current buffer
	double buffer_end_; // location

	double threshold_; // for buffer read;
	tran_t tran_flag_; // 0 for transmitting begin, 1 for play pause, 
			// 2 for transmitting over;
	bool swch_flag_;	// true for switch channel
				// false for not
	bool rebuffer_flag_;	// true for rebuffering
				// false of not rebuffering
	double rebuffer_start_time_;
	
	void initClient();
	void readBuffer (double interval);
	void pauseplay();
	void backplay();
	void stop(bool abandon_flag);
	void jumpto(double time);  // jump to some time point
	void switchChannel(const char* channel);
	void switchChannelAction();
	char former_play_channel_[1024];
	char play_channel_[1024];
	double play_time_; //currrent video play time
	int pause_count_;	

	int pause_thre_;	//pause threshold
	double rebu_thre_;	//rebuffering threshold

	// mpd info
	int seg_num_;
	double seg_len_;// length of each segment (in sec)
	vector<int> bitrate_list;// received from the server
	int former_bitrate_;
	/* 0 for have received response
	 * 1 for waiting for response
	 * from the server */
	//int state_;
	
	// get the total bytes received
	int bytes_;
};

class DashAppServer : public Application {
public:
	DashAppServer();
protected:
	void recv(int nbytes);
	void send_mpd();
	void send_seg(int bitrate_);

	void init_mpd();
private:
	int command(int argc, const char*const* argv);
	int seqno_;	// the media segment segment in request
	int bitrate_;	// the bitrate in request
	int data_type_; // 1 for media and 0 for mpd

	// mpd info
	int seg_num_;
	double seg_len_;// length of each segment (in sec)
	vector<int> bitrate_list;/* start from the 
			     	   * lowest bitrate
			           */
	// MPD info
	/*MPD mpd;*/
	int flag; // 0 for in transmission
		  // 1 for transmission over
};
