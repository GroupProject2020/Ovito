////////////////////////////////////////////////////////////////////////////////////////
//
//  Copyright 2019 Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify it either under the
//  terms of the GNU General Public License version 3 as published by the Free Software
//  Foundation (the "GPL") or, at your option, under the terms of the MIT License.
//  If you do not alter this notice, a recipient may use your version of this
//  file under either the GPL or the MIT License.
//
//  You should have received a copy of the GPL along with this program in a
//  file LICENSE.GPL.txt.  You should have received a copy of the MIT License along
//  with this program in a file LICENSE.MIT.txt
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND,
//  either express or implied. See the GPL or the MIT License for the specific language
//  governing rights and limitations.
//
////////////////////////////////////////////////////////////////////////////////////////

#include <ovito/core/Core.h>
#include <ovito/core/dataset/animation/TimeInterval.h>
#include "VideoEncoder.h"

extern "C" {
#include <libavutil/mathematics.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
};

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/// The list of supported video formats.
QList<VideoEncoder::Format> VideoEncoder::_supportedFormats;

/******************************************************************************
* Constructor
******************************************************************************/
VideoEncoder::VideoEncoder(QObject* parent) : QObject(parent)
{
	initCodecs();
}

/******************************************************************************
* Initializes libavcodec, and register all codecs and formats.
******************************************************************************/
void VideoEncoder::initCodecs()
{
	static std::once_flag initFlag;
	std::call_once(initFlag, []() {
		::av_register_all();
		::avcodec_register_all();
	});
}

/******************************************************************************
* Returns the error string for the given error code.
******************************************************************************/
QString VideoEncoder::errorMessage(int errorCode)
{
	char errbuf[512];
	if(::av_strerror(errorCode, errbuf, sizeof(errbuf)) < 0) {
		return QString("Unknown FFMPEG error.");
	}
	return QString::fromLocal8Bit(errbuf);
}

/******************************************************************************
* Returns the list of supported output formats.
******************************************************************************/
QList<VideoEncoder::Format> VideoEncoder::supportedFormats()
{
	if(!_supportedFormats.empty())
		return _supportedFormats;

	initCodecs();

	AVOutputFormat* fmt = nullptr;
	while((fmt = ::av_oformat_next(fmt))) {

		if(fmt->flags & AVFMT_NOFILE || fmt->flags & AVFMT_NEEDNUMBER)
			continue;

		Format format;
		format.name = fmt->name;
		format.longName = QString::fromLocal8Bit(fmt->long_name);
		format.extensions = QString::fromLocal8Bit(fmt->extensions).split(',');
		format.avformat = fmt;

		if(format.name != "mov" && format.name != "mp4" && format.name != "avi" && format.name != "gif")
			continue;

		_supportedFormats.push_back(format);
	}

	return _supportedFormats;
}

/******************************************************************************
* Opens a video file for writing.
******************************************************************************/
void VideoEncoder::openFile(const QString& filename, int width, int height, int ticksPerFrame, VideoEncoder::Format* format)
{
	int errCode;

	// Make sure previous file is closed.
	closeFile();

	// For reasons not known to the author, MPEG4 and MOV videos with frame rates 2,4,8 and 16 turn out
	// invalid and don't play in QuickTime Player on macOS. Thus, we should avoid producing videos with these FPS values.
	// As a workaround, we instead resort to one of the valid playback rates being is an integer multiple of the selected frame rate.
	// We then have to output N identicial copies of each rendered frame to mimic the desired frame rate.
	
	if(ticksPerFrame == (TICKS_PER_SECOND / 2)) _frameDuplication = 5; // Change 2 fps to 10 fps.
	else if(ticksPerFrame == (TICKS_PER_SECOND / 4)) _frameDuplication = 3; // Change 4 fps to 12 fps.
	else if(ticksPerFrame == (TICKS_PER_SECOND / 8)) _frameDuplication = 3; // Change 8 fps to 24 fps.
	else if(ticksPerFrame == (TICKS_PER_SECOND / 16)) _frameDuplication = 3; // Change 16 fps to 48 fps.
	else _frameDuplication = 1;
	ticksPerFrame /= _frameDuplication; 

	AVOutputFormat* outputFormat;
	if(format == nullptr) {
		// Auto detect the output format from the file name.
		outputFormat = ::av_guess_format(nullptr, qPrintable(filename), nullptr);
		if(!outputFormat)
			throw Exception(tr("Could not deduce video output format from file extension."));
	}
	else outputFormat = format->avformat;

#if LIBAVFORMAT_VERSION_MAJOR >= 58
	// Allocate the output media context.
	AVFormatContext* formatContext = nullptr;
	if((errCode = ::avformat_alloc_output_context2(&formatContext, outputFormat, nullptr, qPrintable(filename))) < 0 || !formatContext)
		throw Exception(tr("Failed to create video format context: %1").arg(errorMessage(errCode)));
	_formatContext.reset(formatContext, &av_free);
#else
	// Allocate the output media context.
	_formatContext.reset(::avformat_alloc_context(), &av_free);
	if(!_formatContext)
		throw Exception(tr("Failed to allocate output media context."));

	_formatContext->oformat = outputFormat;
	qstrncpy(_formatContext->filename, qPrintable(filename), sizeof(_formatContext->filename) - 1);
#endif

	if(outputFormat->video_codec == AV_CODEC_ID_NONE)
		throw Exception(tr("No video codec available."));

	// Find the video encoder.
	_codec = ::avcodec_find_encoder(outputFormat->video_codec);
	if(!_codec)
		throw Exception(tr("Video codec not found."));

	// Add the video stream using the default format codec and initialize the codec.
	_videoStream = ::avformat_new_stream(_formatContext.get(), _codec);
	if(!_videoStream)
		throw Exception(tr("Failed to create video stream."));
	_videoStream->id = 0;

	// Create the codec context.
#if LIBAVCODEC_VERSION_MAJOR >= 57
	_codecContext.reset(::avcodec_alloc_context3(_codec), [](AVCodecContext* ctxt) { ::avcodec_free_context(&ctxt); });
    if(!_codecContext)
		throw Exception(tr("Failed to allocate a video encoding context."));
#else
	_codecContext.reset(_videoStream->codec, [](AVCodecContext*) {});
#endif

	_codecContext->codec_id = outputFormat->video_codec;
	_codecContext->codec_type = AVMEDIA_TYPE_VIDEO;
	_codecContext->qmin = 3;
	_codecContext->qmax = 3;
	_codecContext->bit_rate = 0;
	_codecContext->width = width;
	_codecContext->height = height;
	_codecContext->time_base.num = _videoStream->time_base.num = ticksPerFrame;
	_codecContext->time_base.den = _videoStream->time_base.den = TICKS_PER_SECOND;
	_codecContext->gop_size = 12;	// Emit one intra frame every twelve frames at most.
	_codecContext->framerate = av_inv_q(_codecContext->time_base);
	_videoStream->avg_frame_rate = av_inv_q(_codecContext->time_base);

	// Be sure to use the correct pixel format (e.g. RGB, YUV).
	if(_codec->pix_fmts)
		_codecContext->pix_fmt = _codec->pix_fmts[0];
	else
		_codecContext->pix_fmt = AV_PIX_FMT_YUV422P;

	// Some formats want stream headers to be separate.
	if(_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
#ifdef AV_CODEC_FLAG_GLOBAL_HEADER
		_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
#else
		_codecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
#endif
	}

	// Open the codec.
	if((errCode = ::avcodec_open2(_codecContext.get(), _codec, nullptr)) < 0)
		throw Exception(tr("Could not open video codec: %1").arg(errorMessage(errCode)));

#if LIBAVCODEC_VERSION_MAJOR >= 57
	// Copy the stream parameters to the muxer.
	if((errCode = ::avcodec_parameters_from_context(_videoStream->codecpar, _codecContext.get())) < 0)
		throw Exception(tr("Could not copy the video stream parameters: %1").arg(errorMessage(errCode)));
#endif

	// Allocate and init a video frame data structure.
#if LIBAVCODEC_VERSION_MAJOR >= 56
	_frame.reset(::av_frame_alloc(), [](AVFrame* frame) { ::av_frame_free(&frame); });
#else
	_frame.reset(::avcodec_alloc_frame(), &::av_free);
#endif
	if(!_frame)
		throw Exception(tr("Could not allocate video frame buffer."));

#if LIBAVCODEC_VERSION_MAJOR >= 57
	_frame->format = _codecContext->pix_fmt;
	_frame->width  = _codecContext->width;
	_frame->height = _codecContext->height;

	// Allocate the buffers for the frame data.
	if((errCode = ::av_frame_get_buffer(_frame.get(), 32)) < 0)
		throw Exception(tr("Could not allocate video frame encoding buffer: %1").arg(errorMessage(errCode)));
#else
	// Allocate memory.
	int size = ::avpicture_get_size(_codecContext->pix_fmt, _codecContext->width, _codecContext->height);
	_pictureBuf = std::make_unique<quint8[]>(size);

	// Set up the planes.
	::avpicture_fill(reinterpret_cast<AVPicture*>(_frame.get()), _pictureBuf.get(), _codecContext->pix_fmt, _codecContext->width, _codecContext->height);

	// Allocate memory for encoded frame.
	_outputBuf.resize(width * height * 3);
#endif

	// Open output file (if needed).
	if(!(outputFormat->flags & AVFMT_NOFILE)) {
		if((errCode = ::avio_open(&_formatContext->pb, qPrintable(filename), AVIO_FLAG_WRITE)) < 0)
			throw Exception(tr("Failed to open output video file '%1': %2").arg(filename).arg(errorMessage(errCode)));
	}

	// Write stream header, if any.
	if((errCode = ::avformat_write_header(_formatContext.get(), nullptr)) < 0)
		throw Exception(tr("Failed to write video file header: %1").arg(errorMessage(errCode)));

	::av_dump_format(_formatContext.get(), 0, qPrintable(filename), 1);

	// Success.
	_isOpen = true;
	_numFrames = 0;
}

/******************************************************************************
* This closes the written video file.
******************************************************************************/
void VideoEncoder::closeFile()
{
	if(!_formatContext) {
		OVITO_ASSERT(!_isOpen);
		return;
	}

	// Write stream trailer.
	if(_isOpen) {

#if LIBAVCODEC_VERSION_MAJOR >= 57
		// Flush encoder.
		int errCode;
		if((errCode = ::avcodec_send_frame(_codecContext.get(), nullptr)) < 0)
			qWarning() << "Error while submitting an image frame for video encoding:" << errorMessage(errCode);

		do {
			AVPacket pkt = {0};
			::av_init_packet(&pkt);

			errCode = ::avcodec_receive_packet(_codecContext.get(), &pkt);
			if(errCode < 0 && errCode != AVERROR(EAGAIN) && errCode != AVERROR_EOF) {
				qWarning() << "Error while encoding video frame:" << errorMessage(errCode);
				break;
			}

			if(errCode >= 0) {
				::av_packet_rescale_ts(&pkt, _codecContext->time_base, _videoStream->time_base);
				pkt.stream_index = _videoStream->index;
				// Write the compressed frame to the media file.
				if((errCode = ::av_interleaved_write_frame(_formatContext.get(), &pkt)) < 0) {
					qWarning() << "Error while writing encoded video frame:" << errorMessage(errCode);
				}
			}
		}
		while(errCode >= 0);
#endif
		::avcodec_flush_buffers(_codecContext.get());
		::av_write_trailer(_formatContext.get());
	}

	// Close codec.
	if(_codecContext)
		::avcodec_close(_codecContext.get());

#if LIBAVCODEC_VERSION_MAJOR < 57
	// Free streams.
	if(_formatContext) {
		for(size_t i = 0; i < _formatContext->nb_streams; i++) {
			::av_freep(&_formatContext->streams[i]->codec);
			::av_freep(&_formatContext->streams[i]);
		}
	}
#endif

	// Close the output file.
	if(_formatContext->pb)
		::avio_close(_formatContext->pb);

	// Cleanup.
	_pictureBuf.reset();
	_frame.reset();
	_videoStream = nullptr;
	_codecContext.reset();
	_outputBuf.clear();
	_formatContext.reset();
	_isOpen = false;
}

/******************************************************************************
* Writes a single frame into the video file.
******************************************************************************/
void VideoEncoder::writeFrame(const QImage& image)
{
	OVITO_ASSERT(_isOpen);
	if(!_isOpen)
		return;

	for(int frameCopy = 0; frameCopy < _frameDuplication; frameCopy++) {

		// Make sure the frame data is writable.
		int errCode;
		if((errCode = ::av_frame_make_writable(_frame.get())) < 0)
			throw Exception(tr("Making video frame buffer writable failed: %1").arg(errorMessage(errCode)));
		_frame->pts = _numFrames++;

		// Check if the image size matches.
		int videoWidth = _codecContext->width;
		int videoHeight = _codecContext->height;
		OVITO_ASSERT(image.width() == videoWidth && image.height() == videoHeight);
		if(image.width() != videoWidth || image.height() != videoHeight)
			throw Exception(tr("Frame has wrong dimensions."));

		// Make sure bit format of image is correct.
		QImage finalImage = image.convertToFormat(QImage::Format_RGB32);

		// Create conversion context.
		_imgConvertCtx = ::sws_getCachedContext(_imgConvertCtx, videoWidth, videoHeight, AV_PIX_FMT_BGRA,
				videoWidth, videoHeight, _codecContext->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
		if(!_imgConvertCtx)
			throw Exception(tr("Cannot initialize SWS conversion context to convert video frame."));

		// Convert image to codec pixel format.
		uint8_t *srcplanes[3];
		srcplanes[0] = (uint8_t*)finalImage.bits();
		srcplanes[1] = nullptr;
		srcplanes[2] = nullptr;

		int srcstride[3];
		srcstride[0] = finalImage.bytesPerLine();
		srcstride[1] = 0;
		srcstride[2] = 0;

		::sws_scale(_imgConvertCtx, srcplanes, srcstride, 0, videoHeight, _frame->data, _frame->linesize);

#if LIBAVCODEC_VERSION_MAJOR >= 57

		if((errCode = ::avcodec_send_frame(_codecContext.get(), _frame.get())) < 0)
			throw Exception(tr("Error while submitting an image frame for video encoding: %1").arg(errorMessage(errCode)));

		do {
			AVPacket pkt = {0};
			::av_init_packet(&pkt);

			errCode = ::avcodec_receive_packet(_codecContext.get(), &pkt);
			if(errCode < 0 && errCode != AVERROR(EAGAIN) && errCode != AVERROR_EOF)
				throw Exception(tr("Error while encoding video frame: %1").arg(errorMessage(errCode)));

			if(errCode >= 0) {
				::av_packet_rescale_ts(&pkt, _codecContext->time_base, _videoStream->time_base);
				pkt.stream_index = _videoStream->index;
				// Write the compressed frame to the media file.
				if((errCode = ::av_interleaved_write_frame(_formatContext.get(), &pkt)) < 0) {
					throw Exception(tr("Error while writing encoded video frame: %1").arg(errorMessage(errCode)));
				}
			}
		}
		while(errCode >= 0);

#elif !defined(FF_API_OLD_ENCODE_VIDEO) && LIBAVCODEC_VERSION_MAJOR < 55
		int out_size = ::avcodec_encode_video(_codecContext.get(), _outputBuf.data(), _outputBuf.size(), _frame.get());
		// If zero size, it means the image was buffered.
		if(out_size > 0) {
			AVPacket pkt;
			::av_init_packet(&pkt);
			if(_codecContext->coded_frame->pts != AV_NOPTS_VALUE)
				pkt.pts = ::av_rescale_q(_codecContext->coded_frame->pts, _codecContext->time_base, _videoStream->time_base);
			if(_codecContext->coded_frame->key_frame)
				pkt.flags |= AV_PKT_FLAG_KEY;

			pkt.stream_index = _videoStream->index;
			pkt.data = _outputBuf.data();
			pkt.size = out_size;
			if(::av_interleaved_write_frame(_formatContext.get(), &pkt) < 0) {
				::av_free_packet(&pkt);
				throw Exception(tr("Error while writing video frame."));
			}
			::av_free_packet(&pkt);
		}
#else
		AVPacket pkt = {0};
		::av_init_packet(&pkt);

		int got_packet_ptr;
		if((errCode = ::avcodec_encode_video2(_codecContext.get(), &pkt, _frame.get(), &got_packet_ptr)) < 0)
			throw Exception(tr("Error while encoding video frame: %1").arg(errorMessage(errCode)));

		if(got_packet_ptr && pkt.size) {
			pkt.stream_index = _videoStream->index;
			if((errCode = ::av_write_frame(_formatContext.get(), &pkt)) < 0) {
				::av_free_packet(&pkt);
				throw Exception(tr("Error while writing video frame: %1").arg(errorMessage(errCode)));
			}
			::av_free_packet(&pkt);
		}
#endif
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
