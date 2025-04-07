/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose.
**
** V4l2RTSPServer.cpp
** 
** V4L2 RTSP server
**
** -------------------------------------------------------------------------*/

#include <dirent.h>

#include <sstream>

#include "logger.h"
#include "V4l2Capture.h"
#include "V4l2Output.h"
#include "DeviceSourceFactory.h"
#include "V4l2RTSPServer.h"
#include "VideoCaptureAccess.h"

#ifdef HAVE_ALSA
#include "ALSACapture.h"
#endif

StreamReplicator* V4l2RTSPServer::CreateVideoReplicator( 
					const V4L2DeviceParameters& inParam,
					int queueSize, V4L2DeviceSource::CaptureMode captureMode, int repeatConfig,
					const std::string& outputFile, V4l2IoType ioTypeOut, V4l2Output*& out) {

	StreamReplicator* videoReplicator = NULL;
    std::string videoDev(inParam.m_devName);
	if (!videoDev.empty())
	{
		// Init video capture
		LOG(NOTICE) << "Create V4L2 Source..." << videoDev;
		
		V4l2Capture* videoCapture = V4l2Capture::create(inParam);
		if (videoCapture)
		{
			int outfd = -1;
			
			if (!outputFile.empty())
			{
				V4L2DeviceParameters outparam(outputFile.c_str(), videoCapture->getFormat(), videoCapture->getWidth(), videoCapture->getHeight(), 0, ioTypeOut, inParam.m_verbose);
				out = V4l2Output::create(outparam);
				if (out != NULL)
				{
					outfd = out->getFd();
					LOG(INFO) << "Output fd:" << outfd << " " << outputFile;
				} else {
					LOG(WARN) << "Cannot open output:" << outputFile;
				}
			}
			
			std::string rtpVideoFormat(BaseServerMediaSubsession::getVideoRtpFormat(videoCapture->getFormat()));
			if (rtpVideoFormat.empty()) {
				LOG(FATAL) << "No Streaming format supported for device " << videoDev;
				delete videoCapture;
			} else {
				videoReplicator = DeviceSourceFactory::createStreamReplicator(this->env(), videoCapture->getFormat(), new VideoCaptureAccess(videoCapture), queueSize, captureMode, outfd, repeatConfig);
				if (videoReplicator == NULL) 
				{
					LOG(FATAL) << "Unable to create source for device " << videoDev;
					delete videoCapture;
				}
			}
		}
	}
	return videoReplicator;
}

std::string getVideoDeviceName(const std::string & devicePath)
{
	std::string deviceName(devicePath);
	size_t pos = deviceName.find_last_of('/');
	if (pos != std::string::npos) {
		deviceName.erase(0,pos+1);
	}
	return deviceName;
}
