#include "VideoPlayer.hh"
#include "Logger.h"
#include <stdexcept>

VideoPlayer::~VideoPlayer()
{
	cleanup();
}

void VideoPlayer::cleanup()
{
	if (rgbaBuffer) {
		av_free(rgbaBuffer);
		rgbaBuffer = nullptr;
	}
	if (swsCtx) {
		sws_freeContext(swsCtx);
		swsCtx = nullptr;
	}
	if (rgbaFrame) {
		av_frame_free(&rgbaFrame);
		rgbaFrame = nullptr;
	}
	if (frame) {
		av_frame_free(&frame);
		frame = nullptr;
	}
	if (packet) {
		av_packet_free(&packet);
		packet = nullptr;
	}
	if (codecCtx) {
		avcodec_free_context(&codecCtx);
		codecCtx = nullptr;
	}
	if (formatCtx) {
		avformat_close_input(&formatCtx);
		formatCtx = nullptr;
	}
}

bool VideoPlayer::open(const std::string& filepath)
{
	cleanup();
	finished = false;

	
	if (avformat_open_input(&formatCtx, filepath.c_str(), nullptr, nullptr) != 0) {
		LOG("FFmpeg: Failed to open file: " + filepath);
		return false;
	}

	
	if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
		LOG("FFmpeg: Failed to find stream info");
		cleanup();
		return false;
	}

	
	videoStreamIndex = -1;
	for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
		if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStreamIndex = i;
			break;
		}
	}

	if (videoStreamIndex < 0) {
		LOG("FFmpeg: No video stream found");
		cleanup();
		return false;
	}

	AVStream* videoStream = formatCtx->streams[videoStreamIndex];
	AVCodecParameters* codecParams = videoStream->codecpar;

	// find decoder
	codec = avcodec_find_decoder(codecParams->codec_id);
	if (!codec) {
		LOG("FFmpeg: Unsupported codec");
		cleanup();
		return false;
	}

	// create codec context
	codecCtx = avcodec_alloc_context3(codec);
	if (!codecCtx) {
		LOG("FFmpeg: Failed to allocate codec context");
		cleanup();
		return false;
	}

	if (avcodec_parameters_to_context(codecCtx, codecParams) < 0) {
		LOG("FFmpeg: Failed to copy codec parameters");
		cleanup();
		return false;
	}

	// open the codec
	if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
		LOG("FFmpeg: Failed to open codec");
		cleanup();
		return false;
	}

	width = codecCtx->width;
	height = codecCtx->height;

	// xalculate frame rate
	if (videoStream->avg_frame_rate.den != 0 && videoStream->avg_frame_rate.num != 0) {
		frameRate = av_q2d(videoStream->avg_frame_rate);
	} else if (videoStream->r_frame_rate.den != 0 && videoStream->r_frame_rate.num != 0) {
		frameRate = av_q2d(videoStream->r_frame_rate);
	} else {
		frameRate = 30.0; // fallback
	}

	// allocate frames
	frame = av_frame_alloc();
	rgbaFrame = av_frame_alloc();
	packet = av_packet_alloc();

	if (!frame || !rgbaFrame || !packet) {
		LOG("FFmpeg: Failed to allocate frames/packet");
		cleanup();
		return false;
	}

	// allocate RGBA buffer
	int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
	rgbaBuffer = (uint8_t*)av_malloc(numBytes);

	// set up the RGBA frame to point at our buffer
	av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, rgbaBuffer,
		AV_PIX_FMT_RGBA, width, height, 1);

	// create the scaler context (YUV -> RGBA)
	swsCtx = sws_getContext(
		width, height, codecCtx->pix_fmt,     
		width, height, AV_PIX_FMT_RGBA,       
		SWS_BILINEAR, nullptr, nullptr, nullptr
	);

	if (!swsCtx) {
		LOG("FFmpeg: Failed to create sws context");
		cleanup();
		return false;
	}

	LOG("FFmpeg: Opened video " + filepath + " (" +
		std::to_string(width) + "x" + std::to_string(height) +
		" @ " + std::to_string(frameRate) + " fps)");

	return true;
}

bool VideoPlayer::decodeNextFrame()
{
	if (finished || !formatCtx || !codecCtx) return false;

	while (true) {
		int ret = av_read_frame(formatCtx, packet);
		if (ret < 0) {
			// end of file or error — loop the video
			finished = true;
			return false;
		}

		// skip non-video packets (e.g. audio)
		if (packet->stream_index != videoStreamIndex) {
			av_packet_unref(packet);
			continue;
		}

		// send packet to decoder
		ret = avcodec_send_packet(codecCtx, packet);
		av_packet_unref(packet);

		if (ret < 0) {
			continue; // skip bad packets
		}

		// receive decoded frame
		ret = avcodec_receive_frame(codecCtx, frame);
		if (ret == AVERROR(EAGAIN)) {
			continue; // need more packets
		}
		if (ret < 0) {
			finished = true;
			return false;
		}

		// convert YUV -> RGBA
		sws_scale(swsCtx,
			frame->data, frame->linesize, 0, height,
			rgbaFrame->data, rgbaFrame->linesize);

		return true; // frame ready!
	}
}

void VideoPlayer::restart()
{
	if (formatCtx) {
		av_seek_frame(formatCtx, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
		avcodec_flush_buffers(codecCtx);
		finished = false;
	}
}
