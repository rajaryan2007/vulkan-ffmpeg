#define MINIAUDIO_IMPLEMENTATION
#include "VideoPlayer.hh"
#include "Logger.h"
#include <stdexcept>

void VideoPlayer::run_audio(AVPacket *packet) {
  int ret = avcodec_send_packet(audioCodecCtx, packet);
  if (ret < 0)
    return;

  while (avcodec_receive_frame(audioCodecCtx, frame) == 0) {
    // Prepare output buffer for resampling
    uint8_t *outData = nullptr;
    // calculate max possible samples out
    int outSamples = (int)av_rescale_rnd(swr_get_delay(swrCtx, frame->sample_rate) + frame->nb_samples, 48000,frame->sample_rate, AV_ROUND_UP);
    av_samples_alloc(&outData, NULL, 2, outSamples, AV_SAMPLE_FMT_S16, 0);

	// convert audio to stereo 16-bit
    int actualOut = swr_convert(swrCtx, &outData, outSamples, (const uint8_t **)frame->data,frame->nb_samples);

    // Push to buffer (Locked so the audio thread doesn't read while we write)
    size_t sizeInBytes = actualOut * 2 * 2;
    std::lock_guard<std::mutex> lock(audioMutex);
    audioBuffer.insert(audioBuffer.end(), outData, outData + sizeInBytes);

    av_freep(&outData);
  }
}

VideoPlayer::~VideoPlayer() { cleanup(); }

void VideoPlayer::cleanup() {
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
  if (swrCtx) {
    swr_free(&swrCtx);
    swrCtx = nullptr;
  }
  if (audioCodecCtx) {
    avcodec_free_context(&audioCodecCtx);
    audioCodecCtx = nullptr;
  }
  if (audioInitialized) {
    ma_device_uninit(&audioDevice);
    audioInitialized = false;
  }
}

bool VideoPlayer::open(const std::string &filepath) {
  cleanup();
  finished = false;
  audioClock.store(0.0);
  videoClock = 0.0;

  if (avformat_open_input(&formatCtx, filepath.c_str(), nullptr, nullptr) !=
      0) {
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

  audioStreamIndex = -1;
  for (unsigned int i = 0; i < formatCtx->nb_streams; i++) {
    if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
      audioStreamIndex = i;
      break;
    }
  }

  if (audioStreamIndex < 0) {
    LOG("FFmpeg: No audio stream found");
    cleanup();
    return false;
  }

  if (videoStreamIndex < 0) {
    LOG("FFmpeg: No video stream found");
    cleanup();
    return false;
  }

  AVStream *videoStream = formatCtx->streams[videoStreamIndex];
  AVStream *audioStream = formatCtx->streams[audioStreamIndex];
  AVCodecParameters *codecParams = videoStream->codecpar;
  AVCodecParameters *audioCodecParams = audioStream->codecpar;

  audioCodec = avcodec_find_decoder(audioCodecParams->codec_id);
  if (!audioCodec) {
    LOG("FFmpeg: Unsupported audio codec");
    cleanup();
    return false;
  }

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
  } else if (videoStream->r_frame_rate.den != 0 && videoStream->r_frame_rate.num != 0) 
  {
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
  rgbaBuffer = (uint8_t *)av_malloc(numBytes);

  // set up the RGBA frame to point at our buffer
  av_image_fill_arrays(rgbaFrame->data, rgbaFrame->linesize, rgbaBuffer,
                       AV_PIX_FMT_RGBA, width, height, 1);

  // create the scaler context (YUV -> RGBA)
  swsCtx = sws_getContext(width, height, codecCtx->pix_fmt, width, height, AV_PIX_FMT_RGBA, SWS_BILINEAR, nullptr, nullptr, nullptr);

  if (!swsCtx) {
    LOG("FFmpeg: Failed to create sws context");
    cleanup();
    return false;
  }

  LOG("FFmpeg: Opened video " + filepath + " (" + std::to_string(width) + "x" +
      std::to_string(height) + " @ " + std::to_string(frameRate) + " fps)");
  audioCodecCtx = avcodec_alloc_context3(audioCodec);
  avcodec_parameters_to_context(audioCodecCtx, audioCodecParams);
  avcodec_open2(audioCodecCtx, audioCodec, nullptr);

  // Init Swr once
  AVChannelLayout out_ch_layout;
  av_channel_layout_default(&out_ch_layout, 2);

  int swr_ret =
      swr_alloc_set_opts2(&swrCtx, &out_ch_layout, AV_SAMPLE_FMT_S16, 48000,&audioCodecCtx->ch_layout, audioCodecCtx->sample_fmt, audioCodecCtx->sample_rate, 0, NULL);
  if (swr_ret < 0) {
    LOG("FFmpeg: Failed to allocate swr context");
    cleanup();
    return false;
  }
  swr_init(swrCtx);

  //  atart miniaudio 
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_s16;
  config.playback.channels = 2;
  config.sampleRate = 48000;
  config.dataCallback = audio_callback;
  config.pUserData = this;

  ma_device_init(NULL, &config, &audioDevice);
  ma_device_start(&audioDevice);
  audioInitialized = true;

  pts_seconds = frame->pts * av_q2d(audioStream->time_base);
  return true;
}

bool VideoPlayer::decodeNextFrame() {
  if (finished || !formatCtx || !codecCtx)
    return false;

  while (true) {
    int ret = av_read_frame(formatCtx, packet);
    if (ret < 0) {
      // end of file or error — loop the video
      finished = true;
      return false;
    }

    if (packet->stream_index == audioStreamIndex) {

      run_audio(packet);
      av_packet_unref(packet);
      continue;
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

    if (frame->best_effort_timestamp != AV_NOPTS_VALUE) {
      videoClock = frame->best_effort_timestamp * av_q2d(formatCtx->streams[videoStreamIndex]->time_base);
    } else if (frame->pts != AV_NOPTS_VALUE) {
      videoClock = frame->pts * av_q2d(formatCtx->streams[videoStreamIndex]->time_base);
    }

    // convert YUV -> RGBA
    sws_scale(swsCtx, frame->data, frame->linesize, 0, height, rgbaFrame->data,rgbaFrame->linesize);

    return true; // frame ready!
  }
}

void VideoPlayer::audio_callback(ma_device *pDevice, void *pOutput,const void *pInput, ma_uint32 frameCount) {
  VideoPlayer *player = (VideoPlayer *)pDevice->pUserData;
  if (!player)
    return;

  std::lock_guard<std::mutex> lock(player->audioMutex);

  size_t bytesNeeded = frameCount * 2 * 2; // frames * channels * bytes_per_sample
  size_t bytesToCopy = (std::min)(bytesNeeded, player->audioBuffer.size());

  if (bytesToCopy > 0) {
    memcpy(pOutput, player->audioBuffer.data(), bytesToCopy);
    // remove the data we just played from the front of the vector
    player->audioBuffer.erase(player->audioBuffer.begin(),player->audioBuffer.begin() + bytesToCopy);
  }

  // advance audio clock based on samples played
  player->audioClock.store(player->audioClock.load() + (double)frameCount / 48000.0);

  // if we have no more data, miniaudio handles the silence automatically
}

void VideoPlayer::restart() {
  if (formatCtx) {
    av_seek_frame(formatCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);
    if (audioCodecCtx) avcodec_flush_buffers(audioCodecCtx);
    finished = false;
    audioClock.store(0.0);
    videoClock = 0.0;
    
    std::lock_guard<std::mutex> lock(audioMutex);
    audioBuffer.clear();
  }
}
