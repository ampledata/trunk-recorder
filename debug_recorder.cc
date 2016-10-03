
#include "debug_recorder.h"
using namespace std;

bool debug_recorder::logging = false;

debug_recorder_sptr make_debug_recorder(Source *src)
{
	return gnuradio::get_initial_sptr(new debug_recorder(src));
}

debug_recorder::debug_recorder(Source *src)
	: gr::hier_block2 ("debug_recorder",
	                   gr::io_signature::make  (1, 1, sizeof(gr_complex)),
	                   gr::io_signature::make  (0, 0, sizeof(float)))
{
    source = src;
	freq = source->get_center();
	center = source->get_center();
	samp_rate = source->get_rate();
	talkgroup = 0;
	num = 0;
	active = false;


	starttime = time(NULL);

	float offset = 0; //have to flip for 3.7

	int samp_per_sym = 10;
	double decim = floor(samp_rate / 100000);
	float xlate_bandwidth = 25000; //14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = samp_rate/decim;



	lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 5000, gr::filter::firdes::WIN_BLACKMAN);

	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim,
	            lpf_taps,
	            offset,
	            samp_rate);
	unsigned int d = GCD(channel_rate, pre_channel_rate);
	channel_rate = floor(channel_rate  / d);
	pre_channel_rate = floor(pre_channel_rate / d);
	resampler_taps = design_filter(channel_rate, pre_channel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(channel_rate, pre_channel_rate, resampler_taps);
	valve = gr::blocks::copy::make(sizeof(gr_complex));
	valve->set_enabled(false);

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/debug";

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%9.0f.raw", path_stream.str().c_str(),talkgroup,starttime,freq);
	raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), filename);




		connect(self(),0, valve,0);
		connect(valve,0, prefilter,0);
		connect(prefilter, 0, downsample_sig, 0);
		connect(downsample_sig, 0, raw_sink, 0);
}

debug_recorder::~debug_recorder() {

}


bool debug_recorder::is_active() {
	return active;
}

long debug_recorder::get_talkgroup() {
	return talkgroup;
}

double debug_recorder::get_freq() {
	return freq;
}

Source *debug_recorder::get_source() {
    return source;
}

void debug_recorder::tune_offset(double f) {
	freq = f;
	long offset_amount = (f - center);
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
}
void debug_recorder::deactivate() {
	BOOST_LOG_TRIVIAL(info) << "debug_recorder.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";


	raw_sink->close();



	active = false;
	valve->set_enabled(false);




}

void debug_recorder::activate(Call *call, int n) {

	starttime = time(NULL);

	talkgroup = call->get_talkgroup();
	freq = call->get_freq();
    num = n;

	BOOST_LOG_TRIVIAL(info) << "debug_recorder.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]";


	prefilter->set_center_freq(freq - center); // have to flip for 3.7
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/debug";

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,starttime,freq);


	raw_sink->open(filename);


	active = true;
	valve->set_enabled(true);
}
