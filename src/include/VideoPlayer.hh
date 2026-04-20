#pragma once
#include <string>
#include <cstdint>
#include <vector>

// FFmpeg is C, so we need extern "C"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include "miniaudio.h"

class VideoPlayer {
public:
	VideoPlayer() = default;
	~VideoPlayer();

	
	bool open(const std::string& filepath);

	
	bool decodeNextFrame();

	const uint8_t* getFrameData() const { return rgbaBuffer; }

	
	int getWidth() const { return width; }
	int getHeight() const { return height; }

	double getFrameRate() const { return frameRate; }
	double getFrameDuration() const { return 1.0 / frameRate; }
	bool isFinished() const { return finished; }

	
	void restart();

private:
	void cleanup();

	AVFormatContext* formatCtx = nullptr;
	const AVCodec* codec = nullptr;
	AVCodecContext* codecCtx = nullptr;
	AVFrame* frame = nullptr;      
	AVFrame* rgbaFrame = nullptr;   
	AVPacket* packet = nullptr;
	SwsContext* swsCtx = nullptr;

	uint8_t* rgbaBuffer = nullptr;
	int videoStreamIndex = -1;
	int width = 0;
	int height = 0;
	double frameRate = 30.0;
	bool finished = false;
};
