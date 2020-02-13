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

#pragma once


#include <ovito/core/Core.h>

extern "C" {
	struct AVFormatContext;
	struct AVOutputFormat;
	struct AVCodec;
	struct AVStream;
	struct AVCodecContext;
	struct AVFrame;
	struct SwsContext;
};

namespace Ovito {

/**
 * \brief Wrapper class for the ffmpeg video encoding library.
 */
class OVITO_CORE_EXPORT VideoEncoder : public QObject
{
public:

	/**
	 * Describes an output format supported by the video encoding engine.
	 */
	class Format {
	public:
		QByteArray name;
		QString longName;
		QStringList extensions;
		AVOutputFormat* avformat;
	};

public:

	/// Constructor.
	VideoEncoder(QObject* parent = nullptr);

	/// Destructor.
	virtual ~VideoEncoder() { closeFile(); }

	/// Opens a video file for writing.
	void openFile(const QString& filename, int width, int height, int ticksPerFrame, VideoEncoder::Format* format = nullptr);

	/// Writes a single frame into the video file.
	void writeFrame(const QImage& image);

	/// This closes the written video file.
	void closeFile();

	/// Returns the list of supported output formats.
	static QList<Format> supportedFormats();

private:

	/// Initializes libavcodec, and register all codecs and formats.
	static void initCodecs();

	/// Returns the error string for the given error code.
	static QString errorMessage(int errorCode);

	std::shared_ptr<AVFormatContext> _formatContext;
	std::unique_ptr<quint8[]> _pictureBuf;
	std::vector<quint8> _outputBuf;
	std::shared_ptr<AVFrame> _frame;
	AVStream* _videoStream = nullptr;
	AVCodec* _codec = nullptr;
	std::shared_ptr<AVCodecContext> _codecContext;
	SwsContext* _imgConvertCtx = nullptr;
	bool _isOpen = false;
	int _numFrames = 0;
	int _frameDuplication = 1;

	/// The list of supported video formats.
	static QList<Format> _supportedFormats;

	Q_OBJECT
};

}	// End of namespace
