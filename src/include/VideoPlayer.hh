#pragma once
#include <string>
#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

// FFmpeg is C, so we need extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h> 
#include <libswscale/swscale.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>          
}
#include "miniaudio.h"

class VideoPlayer {
public:
	void run_audio(AVPacket* packet);


	VideoPlayer() = default;
	~VideoPlayer();

	
	bool open(const std::string& filepath);
	
	
	bool decodeNextFrame();

	const uint8_t* getFrameData() const { return rgbaBuffer; }

	static void audio_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
	int getWidth() const { return width; }
	int getHeight() const { return height; }

	double getFrameRate() const { return frameRate; }
	double getFrameDuration() const { return 1.0 / frameRate; }
	bool isFinished() const { return finished; }
	
	double get_system_time() const {
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now().time_since_epoch()
		).count() / 1000000.0;
	}

	double getAudioClock() const {
		
		double base = audioClockBase.load();
		double last_update = audioClockUpdateSystemTime.load();

		if (!isplaying) return base;

		double elapsed = get_system_time() - last_update;
		return base + elapsed;
	}

	double getVideoClock() const { return videoClock; }

	bool isPlaying() const { return isplaying; }
    
	bool isForward5Seconds() const { return isfroward5seconds; }
	bool isBackward5Seconds() const { return isbackward5seconds; }
	void seekToSecond(double second);
	void playpause();
	void forward5Seconds();
	void backward5Seconds();
	
	double getProgressBar();       // returns current position in seconds
	void setProgressBar(double seconds); // seeks to the given second
	double getTotalDuration() const; // total video duration in seconds
	void restart();

private:
	void cleanup();

	AVFormatContext* formatCtx = nullptr;
	const AVCodec* codec = nullptr;
	const AVCodec* audioCodec = nullptr;
	AVCodecContext* codecCtx = nullptr;
	AVCodecContext* audioCodecCtx = nullptr;
	AVFrame* frame = nullptr;      
	AVFrame* rgbaFrame = nullptr;   
	AVPacket* packet = nullptr;
	SwsContext* swsCtx = nullptr;
	SwrContext* swrCtx = nullptr;
	ma_device audioDevice;
    ma_decoder audioDecoder;
	double pts_seconds = 0.0;
	std::vector<uint8_t> audioBuffer;
	mutable std::mutex audioMutex;

	std::atomic<double> audioClock{0.0};
	std::atomic<double> audioClockBase{ 0.0 };
	std::atomic<double> audioClockUpdateSystemTime{ 0.0 };

	double videoClock = 0.0;
	double seekTargetTime = 0.0;

	uint8_t* rgbaBuffer = nullptr;
	int videoStreamIndex = -1;
	int audioStreamIndex = -1;

	int width = 0;
	int height = 0;
	double frameRate = 30.0;
	bool finished = false;

	bool isplaying = true;
	bool isfroward5seconds = false;
	bool isbackward5seconds = false;

	bool audioInitialized = false;
};
