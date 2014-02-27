#include "dash-exp.h"

static class DashAppClientClass : public TclClass {
public:
	DashAppClientClass() : TclClass("Application/DashAppClient") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new DashAppClient);
	}
} class_appclient_dash;

DashAppClient::DashAppClient() :
	seqno_(1), data_type_(0), seg_num_(0), seg_len_(1.0), tran_flag_(TRAN_BEGIN), 
	bytes_(0), buffer_(0), reqBytes_(0), revBytes_(0), threshold_(1),
	play_time_(0), rebuffer_flag_(false), rebuffer_start_time_(0.0), pause_count_(0),
	pause_thre_(3), rebu_thre_(3.0), former_bitrate_(0)
{
    	swch_flag_ = false;
	//double last_recv_time = 0;
	//double cur_recv_time = 0;
	bind("seqno", &seqno_);
	bind("segnumber", &seg_num_);
	bind("seglength", &seg_len_);
	bind("bytes", &bytes_);
	bind("buffer", &buffer_);
	bind("playtime", &play_time_);

	bind("pause_thre_", &pause_thre_);
	bind("rebu_thre_", &rebu_thre_);
}

int DashAppClient::command(int argc, const char*const* argv) {
	//Tcl&tcl = Tcl::instance();
	if (strcmp(argv[1], "bitratelist") == 0) {
		// set the bitrate list; start from
		// the lowest bitrate.
		if (argc >= 3) {
			for (int i = 2; i < argc; i++) {
				bitrate_list.push_back(atoi(argv[i])*1000 );
			}
			return (TCL_OK);
		}		
	} else if (strcmp(argv[1], "readbuffer") == 0) {
		double inter = atof(argv[2]);
		readBuffer(inter);
		return (TCL_OK);
	} else if (strcmp(argv[1], "pauseplay") == 0) {
		pauseplay();
		return (TCL_OK);
	} else if (strcmp(argv[1], "backtoplay") == 0) {
		backplay();
		return (TCL_OK);
	} else if (strcmp(argv[1], "jumpto") == 0) {
		double time = atof(argv[2]);
		jumpto(time);	
		return (TCL_OK);
	} else if (strcmp(argv[1], "init-channel") == 0) {
	        strcpy(play_channel_, argv[2]);
		return (TCL_OK);
	} else if (strcmp(argv[1], "switch-channel") == 0) {
		switchChannel(argv[2]);
		return (TCL_OK);
	}
	return (Application::command(argc, argv));
}

void DashAppClient::start () {
	Tcl& tcl = Tcl::instance();
	//tcl.evalf("puts $fub_ \"Start a new video\"");
	tcl.evalf("puts $fpb_ \"Name: %s\"", play_channel_);
	double now = Scheduler::instance().clock();
	tcl.evalf("puts $fpb_ \"%f %f start\"", now, 0.0);
	tcl.evalf("puts $fpb_ \"%f %f API_invoked Client/roll\"", now, 0.0);
	//tcl.evalf("puts $fub_ \"video name: %s\"", play_channel_);
	initClient();
	request_mpd();
}

void DashAppClient::initClient () {
	seqno_ = 1; data_type_ = 0; seg_num_ = 0; 
	seg_len_ = 1.0; tran_flag_ = TRAN_BEGIN;
	bytes_ = 0; buffer_ = 0; reqBytes_ = 0; 
	revBytes_ = 0; threshold_ = 1;
	play_time_ = 0; rebuffer_flag_ = false;
	rebuffer_start_time_ = 0.0;
	pause_count_ = 0;
}

void DashAppClient::pauseplay () {
	printf("pause\n");
	if (tran_flag_ == TRAN_BEGIN)
	    tran_flag_ = TRAN_PAUSE;

	pause_count_++;
	double now = Scheduler::instance().clock();
	double pause_dur = Random::uniform() * 5;
	Tcl& tcl = Tcl::instance();
	tcl.evalf("$ns_ at %f \"$client backtoplay\"", now + pause_dur);
	
	tcl.evalf("puts $fpb_ \"%f %f pause\"", now, play_time_);
	//tcl.evalf("puts $fub_ \"System time: %f\n\"", now);
}

void DashAppClient::backplay() {
    	printf("back to play\n");
	if (tran_flag_ == TRAN_PAUSE)
	    tran_flag_ = TRAN_BEGIN;

	Tcl& tcl = Tcl::instance();
	double now = Scheduler::instance().clock();
	tcl.evalf("puts $fpb_ \"%f %f resume\"", now, play_time_);
	//tcl.evalf("puts $fub_ \"System time: %f\n\"", now);
}

void DashAppClient::stop(bool abandon_flag) {
	printf("stop, play is over\n");
	Tcl& tcl = Tcl::instance();
	
	//((FullTcpAgent*)agent_)->close();
	//tcl.evalf("puts $fub_ \"Stop action at: %f\"", play_time_);
	if (abandon_flag)
	    tcl.evalf("puts $fpb_ \"Abandoned? %s\"", "Yes");
	else
	    tcl.evalf("puts $fpb_ \"Abandoned? %s\"", "No");

	double now = Scheduler::instance().clock();
	tcl.evalf("puts $fpb_ \"%f %f stop\n\"", now, play_time_);

	tcl.evalf("stop");
	tcl.evalf("$ns_ halt");
}
void DashAppClient::jumpto(double time) {
	if(time < 0 || time > seg_num_ * seg_len_) return;

	printf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
	printf("buffer start: %f\n", buffer_start_);
	printf("buffer end: %f\n", buffer_end_);
	printf("time: %f\n", time);

	double former_play_time = play_time_;

	if (time >= buffer_start_ && time <= buffer_end_) {
	    buffer_ = buffer_end_ - time;//---------------------------
	    play_time_ = time; 
	} else {
	    buffer_start_ = buffer_end_ = time;
	    buffer_ = 0;
	    seqno_ = (int)(time/seg_len_);
	    if (seqno_ < 1) seqno_ = 1;
	    play_time_ = seqno_ -1;
	}

	Tcl& tcl = Tcl::instance();
	double now = Scheduler::instance().clock();
	tcl.evalf("puts $fpt_ \"%f %f\"", now, play_time_);
	tcl.evalf("puts $fbu_ \"%f %f\"", now, buffer_);
	
	tcl.evalf("puts $fpb_ \"%f %f jump_to %f\"", now, former_play_time, play_time_);
	//double now = Scheduler::instance().clock();
	//tcl.evalf("puts $fub_ \"System time: %f\n\"", now);
}

void DashAppClient::switchChannel (const char* channel) {
	strcpy(former_play_channel_, play_channel_);
	strcpy(play_channel_, channel);
	swch_flag_ = true;
}

void DashAppClient::switchChannelAction () {
	//stop();
	//strcpy(play_channel_, channel);

	Tcl& tcl = Tcl::instance();
	double now = Scheduler::instance().clock();
	tcl.evalf("puts $fpb_ \"%f %f switch_channel_to %s\"", now, play_time_, play_channel_);
	//tcl.evalf("puts $fub_ \"Former channel: %s, Current channel: %s\"", former_play_channel_, play_channel_);
	//tcl.evalf("puts $fub_ \"System time: %f\n\"", now);

	printf("function switch channel\n");
	//Tcl& tcl = Tcl::instance();
	tcl.evalf("source %s", play_channel_);
	start();
}

void DashAppClient::readBuffer (double interval) {

	if (swch_flag_ && tran_flag_ == TRAN_OVER) {
	    switchChannelAction();
	    return;
	}else if (swch_flag_) {
	    // Stop reading buffer until the
	    // segments from the new channel
	    // arrives
	    return;
	}

	Tcl& tcl = Tcl::instance();
	double now = Scheduler::instance().clock(); 	

	// check if play pause
	if (tran_flag_ == TRAN_PAUSE || seqno_ <= 1) {
	    tcl.evalf("puts $fpt_ \"%f %f\"", now, play_time_);
	    tcl.evalf("puts $fbu_ \"%f %f\"", now, buffer_);
	    return;
	}

	// In TRAN_OVER state, data transmission is over, 
	// the player needs only to read the buffer.
	if (buffer_ >= threshold_ || tran_flag_ == TRAN_OVER) {
		
	    // play is over
	    /*if (buffer_ <= 0) {
		printf("buffer is empty\n");
		return;
	    }*/
	    
	    if (rebuffer_flag_)
	    	tcl.evalf("puts $fpb_ \"%f %f rebuffering_end\"", now, play_time_);
	    rebuffer_flag_ = false;

	    if (buffer_ < interval) {
		play_time_ += buffer_;
		buffer_ = 0;
	    } else {
		play_time_ += interval;
		buffer_ -= interval; 
	    }
	    //printf("play time: %f, buffer size: %f\n", play_time_, buffer_);
	    //printf("interval: %f\n", interval);
	    //printf("current buffer size: %f\n", buffer_);
	    tcl.evalf("puts $fpt_ \"%f %f\"", now, play_time_);
	    tcl.evalf("puts $fbu_ \"%f %f\"", now, buffer_);
/*	    
	    if (play_time_ >= 120.0) {
		switchChannel("dash-content1");
	    }
*/	    
	    // play is over, stop the simulation
	    if (play_time_ >= (double)(seg_num_ * seg_len_)) {
	    	printf("play time: %f\n", play_time_);
		stop(false);
	    	//printf("play is over\n");
		//tcl.evalf("$ns_ at %f \"$BS(0) reset\"", now);
		//tcl.evalf("stop");
		//tcl.evalf("$ns_ halt");
	    }
	    // chance of scrubbing or change channels
	    // --------------------------------------

	    /*if (play_time_ >= 120.0) {
		switchChannel("dash-content1");
	    }*/
	} else { // in rebuffering state
	    printf("play in buffer state, seq no: %d\n", seqno_);
	    if (!rebuffer_flag_) { // one rebuffering happen
		//rebuffer_count_++;
		rebuffer_flag_ = true;
		rebuffer_start_time_ = now;
		// record the start of the rebuffering event
		tcl.evalf("puts $fpb_ \"%f %f rebuffering_happen\"", now, play_time_);
	    } else { // continue in the rebuffering state
		double rebu_dur = now - rebuffer_start_time_;
		if (rebu_dur <= rebu_thre_ && pause_count_ < pause_thre_) {
		    // pause for 0~5 second
		    pauseplay();
		    rebuffer_flag_ = false;
		    tcl.evalf("puts $fpb_ \"%f %f rebuffering_end\"", now, play_time_);
		}
		//if (pause_count_ > 3) {
		else {
		    // pause for more than 3 times
		    // abandon the video
		    // ------------------------
		    /*double judge = Random::uniform();
		    
		    if (judge >= 0.1) { // abdndon
			printf("stop function\n");
			stop();
		    }
		    else { // switch channel
			printf("switch channel function\n");
			stop();
			//switchChannel("dash-content1");
		    }*/
		    printf("stop function\n");
		    stop(true);
		}
	    }
	    tcl.evalf("puts $fbu_ \"%f %f\"", now, buffer_);
	}
}

// request for the mpd file
void DashAppClient::request_mpd() {

	Tcl& tcl = Tcl::instance();
	tcl.evalf("$server set dataType 0"); // request for mpd
	data_type_ = 0;	
	
	send_time = Scheduler::instance().clock();
	int header_len = 100; // get the length of the header
	printf("request mpd file, time: %f\n", send_time);
	send(header_len);
}

void DashAppClient::recv(int nbytes) {
	
	Application::recv(nbytes);
	
	//if (seqno_ <= seg_num_) {
	//printf("time: recv: %f send: %f\n", recv_time, send_time);
	if (data_type_ == 0) { // get mpd
		
		double now_time = Scheduler::instance().clock();
		printf("recv mpd file, time: %f\n", now_time);

		data_type_ = 1; 
		Tcl& tcl = Tcl::instance();
		tcl.evalf("$server set dataType 1");
		buffer_start_ = 0;
		buffer_end_ = 0;
		swch_flag_ = false;
		request_first_seg();// start with requesting the lowest bitrate
	
		tcl.evalf("puts $fpb_ \"Duration: %f\"", seg_num_ * seg_len_);
	}
	else {
	    	// switch channel
		/*if (swch_flag_) {
		    switchChannelAction("dash-content2");
		    return;
		}*/

		if (seqno_ > seg_num_)	return;	
		if (seqno_ >= seg_num_) tran_flag_ = TRAN_OVER; // end transmitting
	    
		bytes_ += nbytes;

		revBytes_ += nbytes;      
		if (revBytes_ < reqBytes_)
		    return; 
		else{
		    printf("receive seq number: %d, bytes: %d\n", seqno_, revBytes_);
		}

		// switch channel
		if (swch_flag_) {
		    switchChannelAction();
		    return;
		}	


		recv_time = Scheduler::instance().clock();
		//printf("recvive time: %f\n", recv_time);
		double interval = recv_time - send_time;
		buffer_ += seg_len_;
		buffer_end_ += seg_len_;

		// get a reasonable bitrate
		int bitrate_ = qoe_cal(interval, revBytes_);
		//if (seqno_ <= 2)
		revBytes_ = 0;
		seqno_++;
		if (seqno_ <= seg_num_)
		    request_next_seg(seqno_, bitrate_);
	}
	//seqno_++;
	//}
}
void DashAppClient::process_mpd() {
}

int DashAppClient::qoe_cal(double interval, int nbytes) {
	if (buffer_ <= 0)
		return bitrate_list.at(0);

	// calculate resonable bitrate
	double throughput = (double)nbytes * 8 / interval;	
	printf("bytes: %d and interval: %f\n", nbytes, interval);
	printf("the throughput: %f\n", throughput);
	int level = bitrate_list.size()-1;
	for (int i = 0; i < bitrate_list.size(); i++) {
		if (bitrate_list.at(i) > (int)throughput) {
			level = i > 0 ? i-1:0;
			break;
		}	
	}
	//printf("bitrate level: %d\n", level);
	//int value = bitrate_list.at(level);
	return bitrate_list.at(level);
}
void DashAppClient::request_first_seg() {
	//printf("send first\n");
	request_next_seg(1, bitrate_list.at(0));
	
	tran_flag_ = TRAN_BEGIN; // start transmitting
}
void DashAppClient::request_next_seg(int seqno, int bitrate_) {

	printf("---------request a new segment------------\n");

	if (seqno > 1 && bitrate_ != former_bitrate_) {
		Tcl& tcl = Tcl::instance();
		double now = Scheduler::instance().clock();
		tcl.evalf("puts $fpb_ \"%f %f bitrate_switch %d %d\"", 
			now, play_time_, former_bitrate_, bitrate_);
	}
	former_bitrate_ = bitrate_;

	if (seqno <= seg_num_) {	// start from 0
		Tcl& tcl = Tcl::instance();
		tcl.evalf("$server set segmentNo %d",seqno);
		tcl.evalf("$server set bitrate %d",bitrate_);

		reqBytes_ = bitrate_ * seg_len_ / 8;

		send_time = Scheduler::instance().clock();
		printf("ask for segment, bitrate: %d\n", bitrate_);
		send(100);// send request for next segment (request length)
		//printf("reqire time: %f\n", send_time);
		printf("%d and %d\n", seqno, seg_num_);
	}
	//else {
	/*
	if (seqno >= seg_num_) {
		//getAgent()->close();
		switchChannel("dash-content2");
	}*/
}

static class DashAppServerClass : public TclClass {
public:
	DashAppServerClass() : TclClass("Application/DashAppServer") {}
	TclObject* create(int argc, const char*const* argv) {
		return (new DashAppServer);
	}
} class_appserver_dash;

DashAppServer::DashAppServer() :
	seqno_(0), bitrate_(200), data_type_(0),seg_num_(0), seg_len_(1.0), flag(0)
{
	bind("segnumber", &seg_num_);
	bind("segmentNo", &seqno_);
	bind("bitrate", &bitrate_);
	bind("dataType", &data_type_);
	bind("seglength", &seg_len_);
}

int DashAppServer::command(int argc, const char*const* argv) {
	//Tcl&tcl = Tcl::instance();

	if (strcmp(argv[1], "bitratelist") == 0) {
		// set the bitrate list; start from
		// the lowest bitrate.
		if (argc >= 3) {
			for (int i = 2; i < argc-1; i++) {
				bitrate_list.push_back(atoi(argv[i]));
			}
			return (TCL_OK);
		}		
	}
	return (Application::command(argc, argv));
}

void DashAppServer::init_mpd() {
	
}

void DashAppServer::recv(int nbytes) {

//	printf("the flag is %d\n",flag);	
//	if (flag) return;

	Application::recv(nbytes);

	if (data_type_ == 0)
		send_mpd();
	else
		send_seg(bitrate_);
	
	if (seqno_ >= seg_num_)
	    flag = 1;
}
// send mpd file
void DashAppServer::send_mpd() {
    	double now_time = Scheduler::instance().clock();
	printf("send mpd file, time: %f\n", now_time);
	Tcl& tcl = Tcl::instance();
	tcl.evalf("$client set segnumber %d",seg_num_);
	tcl.evalf("$client set seglength %f",seg_len_);

	send(10);// length of mpd file
}
// send segment
void DashAppServer::send_seg(int bitrate_) {
	//printf("send segemnt order: %d\n",seqno_);
	//printf("bitrate: %d and data type: %d\n", bitrate_, data_type_);
	int segsize = bitrate_ * seg_len_ / 8;
	//printf("send segment, size: %d\n", segsize);	
	send(segsize);	// length of segment file 
	
}
