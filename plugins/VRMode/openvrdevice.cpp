/*
 * openvrdevice.cpp
 *
 *  Created on: Dec 18, 2015
 *      Author: Chris Denham
 *
 */

#include "openvrdevice.h"
#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif

#include <osg/Geometry>
#include <osgViewer/GraphicsWindow>

#ifndef GL_TEXTURE_MAX_LEVEL
#define GL_TEXTURE_MAX_LEVEL 0x813D
#endif

static const OSG_GLExtensions* getGLExtensions(const osg::State &state)
{
#if (OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0))

	return state.get<osg::GLExtensions>();
#else

	return osg::FBOExtensions::instance(state.getContextID(), true);
#endif
}

static const OSG_Texture_Extensions* getTextureExtensions(const osg::State &state)
{
#if (OSG_VERSION_GREATER_OR_EQUAL(3, 4, 0))

	return state.get<osg::GLExtensions>();
#else

	return osg::Texture::getExtensions(state.getContextID(), true);
#endif
}

static osg::Matrix  convertMatrix34(const vr::HmdMatrix34_t &mat34)
{
	osg::Matrix  matrix(
		mat34.m[0][0], mat34.m[1][0], mat34.m[2][0], 0.0,
		mat34.m[0][1], mat34.m[1][1], mat34.m[2][1], 0.0,
		mat34.m[0][2], mat34.m[1][2], mat34.m[2][2], 0.0,
		mat34.m[0][3], mat34.m[1][3], mat34.m[2][3], 1.0f
		);

	return matrix;
}

static osg::Matrix  convertMatrix44(const vr::HmdMatrix44_t &mat44)
{
	osg::Matrix  matrix(
		mat44.m[0][0], mat44.m[1][0], mat44.m[2][0], mat44.m[3][0],
		mat44.m[0][1], mat44.m[1][1], mat44.m[2][1], mat44.m[3][1],
		mat44.m[0][2], mat44.m[1][2], mat44.m[2][2], mat44.m[3][2],
		mat44.m[0][3], mat44.m[1][3], mat44.m[2][3], mat44.m[3][3]
		);

	return matrix;
}

void  OpenVRPreDrawCallback::operator()(osg::RenderInfo &renderInfo) const
{
	m_textureBuffer->onPreRender(renderInfo);
}

void  OpenVRPostDrawCallback::operator()(osg::RenderInfo &renderInfo) const
{
	m_textureBuffer->onPostRender(renderInfo);
}

/* Public functions */
OpenVRTextureBuffer::OpenVRTextureBuffer(osg::ref_ptr<osg::State> state, int width, int height, int samples):
	m_Resolve_FBO(0),
	m_Resolve_ColorTex(0),
	m_MSAA_FBO(0),
	m_MSAA_ColorTex(0),
	m_MSAA_DepthTex(0),
	m_width(width),
	m_height(height),
	m_samples(samples)
{
	const OSG_GLExtensions *fbo_ext = getGLExtensions(*state);

	// We don't want to support MIPMAP so, ensure only level 0 is allowed.
	const int  maxTextureLevel = 0;

	// Create an FBO for secondary render target ready for application of lens distortion shader.
	fbo_ext->glGenFramebuffers(1, &m_Resolve_FBO);

	glGenTextures(1, &m_Resolve_ColorTex);
	glBindTexture(GL_TEXTURE_2D, m_Resolve_ColorTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, maxTextureLevel);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	// Create an FBO for primary render target.
	fbo_ext->glGenFramebuffers(1, &m_MSAA_FBO);

	const OSG_Texture_Extensions *extensions = getTextureExtensions(*state);

	// Create MSAA colour buffer
	glGenTextures(1, &m_MSAA_ColorTex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_MSAA_ColorTex);
	extensions->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_RGBA, m_width, m_height, false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_ARB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_ARB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, maxTextureLevel);

	// Create MSAA depth buffer
	glGenTextures(1, &m_MSAA_DepthTex);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_MSAA_DepthTex);
	extensions->glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_samples, GL_DEPTH_COMPONENT, m_width, m_height, false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, maxTextureLevel);

	// check FBO status
	GLenum  status = fbo_ext->glCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		osg::notify(osg::WARN) << "Error setting up frame buffer object." << std::endl;
	}
}

void  OpenVRTextureBuffer::onPreRender(osg::RenderInfo &renderInfo)
{
	osg::State             &state   = *renderInfo.getState();
	const OSG_GLExtensions *fbo_ext = getGLExtensions(state);

	fbo_ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, m_MSAA_FBO);
	fbo_ext->glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D_MULTISAMPLE, m_MSAA_ColorTex, 0);
	fbo_ext->glFramebufferTexture2D(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D_MULTISAMPLE, m_MSAA_DepthTex, 0);
}

void  OpenVRTextureBuffer::onPostRender(osg::RenderInfo &renderInfo)
{
	osg::State             &state   = *renderInfo.getState();
	const OSG_GLExtensions *fbo_ext = getGLExtensions(state);

	fbo_ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, m_MSAA_FBO);
	fbo_ext->glFramebufferTexture2D(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D_MULTISAMPLE, m_MSAA_ColorTex, 0);
	fbo_ext->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);

	fbo_ext->glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, m_Resolve_FBO);
	fbo_ext->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_Resolve_ColorTex, 0);
	fbo_ext->glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);

	// Copy MSAA_FBO texture to Resolve_FBO
	fbo_ext->glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	fbo_ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
}

void  OpenVRTextureBuffer::destroy(osg::GraphicsContext *gc)
{
	const OSG_GLExtensions *fbo_ext = getGLExtensions(*gc->getState());

	if (fbo_ext)
	{
		fbo_ext->glDeleteFramebuffers(1, &m_MSAA_FBO);
		fbo_ext->glDeleteFramebuffers(1, &m_Resolve_FBO);
	}
}

OpenVRMirrorTexture::OpenVRMirrorTexture(osg::ref_ptr<osg::State> state, GLint width, GLint height):
	m_width(width),
	m_height(height)
{
	const OSG_GLExtensions *fbo_ext = getGLExtensions(*state);

	// Configure the mirror FBO
	fbo_ext->glGenFramebuffers(1, &m_mirrorFBO);
	fbo_ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, m_mirrorFBO);

	glGenTextures(1, &m_mirrorTex);
	glBindTexture(GL_TEXTURE_2D, m_mirrorTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	fbo_ext->glFramebufferTexture2D(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_mirrorTex, 0);
	fbo_ext->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);
	fbo_ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);
}

void  OpenVRMirrorTexture::blitTexture(osg::GraphicsContext *gc, OpenVRTextureBuffer *leftEye, OpenVRTextureBuffer *rightEye)
{
	const OSG_GLExtensions *fbo_ext = getGLExtensions(*(gc->getState()));

	fbo_ext->glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, m_mirrorFBO);
	fbo_ext->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_mirrorTex, 0);
	fbo_ext->glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);

	glClearColor(1, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// --------------------------------
	// Copy left eye image to mirror
	fbo_ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, leftEye->m_Resolve_FBO);
	fbo_ext->glFramebufferTexture2D(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, leftEye->m_Resolve_ColorTex, 0);
	fbo_ext->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);

	fbo_ext->glBlitFramebuffer(0, 0, leftEye->m_width, leftEye->m_height,
	                           0, 0, m_width / 2, m_height,
	                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
	// --------------------------------
	// Copy right eye image to mirror
	fbo_ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, rightEye->m_Resolve_FBO);
	fbo_ext->glFramebufferTexture2D(GL_READ_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, rightEye->m_Resolve_ColorTex, 0);
	fbo_ext->glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0);

	fbo_ext->glBlitFramebuffer(0, 0, rightEye->m_width, rightEye->m_height,
	                           m_width / 2, 0, m_width, m_height,
	                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
	// ---------------------------------

	fbo_ext->glBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);

	// Blit mirror texture to back buffer
	fbo_ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, m_mirrorFBO);
	fbo_ext->glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, 0);
	GLint  w = m_width;
	GLint  h = m_height;
	fbo_ext->glBlitFramebuffer(0, 0, w, h,
	                           0, 0, w, h,
	                           GL_COLOR_BUFFER_BIT, GL_NEAREST);
	fbo_ext->glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, 0);
}

void  OpenVRMirrorTexture::destroy(osg::GraphicsContext *gc)
{
	const OSG_GLExtensions *fbo_ext = getGLExtensions(*gc->getState());

	if (fbo_ext)
	{
		fbo_ext->glDeleteFramebuffers(1, &m_mirrorFBO);
	}
}

OpenVRDevice::OpenVRDevice(float nearClip, float farClip, const float worldUnitsPerMetre, const int samples):
	m_vrSystem(nullptr),
	m_vrRenderModels(nullptr),
	m_worldUnitsPerMetre(worldUnitsPerMetre),
	m_mirrorTexture(nullptr),
	m_position(osg::Vec3(0.0f, 0.0f, 0.0f)),
	m_leftControllerPosition(osg::Vec3(0.0f, 0.0f, 0.0f)),
	m_rightControllerPosition(osg::Vec3(0.0f, 0.0f, 0.0f)),
	m_orientation(osg::Quat(0.0f, 0.0f, 0.0f, 1.0f)),
	m_leftOrientation(osg::Quat(0.0f, 0.0f, 0.0f, 1.0f)),
	m_rightOrientation(osg::Quat(0.0f, 0.0f, 0.0f, 1.0f)),
	m_touchpadTouchPosition(osg::Vec2(0.0f, 0.0f)),
	m_touchpadPreTouchPosition(osg::Vec2(0.0f, 0.0f)),
	m_nearClip(nearClip), m_farClip(farClip),
	m_samples(samples)
{
	for (int i = 0; i < 2; i++)
	{
		m_textureBuffer[i] = nullptr;
	}

	trySetProcessAsHighPriority();

	// Loading the SteamVR Runtime
	vr::EVRInitError  eError = vr::VRInitError_None;
	m_vrSystem = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		m_vrSystem = nullptr;
		osg::notify(osg::WARN)
		  << "Error: Unable to initialize the OpenVR library.\n"
		  << "Reason: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << std::endl;

		return;
	}

	if (!vr::VRCompositor())
	{
		m_vrSystem = nullptr;
		vr::VR_Shutdown();
		osg::notify(osg::WARN) << "Error: Compositor initialization failed" << std::endl;

		return;
	}

	m_vrRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);

	if (m_vrRenderModels == nullptr)
	{
		m_vrSystem = nullptr;
		vr::VR_Shutdown();
		osg::notify(osg::WARN)
		  << "Error: Unable to get render model interface!\n"
		  << "Reason: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError) << std::endl;

		return;
	}

	std::string  driverName         = GetDeviceProperty(vr::Prop_TrackingSystemName_String);
	std::string  deviceSerialNumber = GetDeviceProperty(vr::Prop_SerialNumber_String);

	osg::notify(osg::NOTICE) << "HMD driver name: " << driverName << std::endl;
	osg::notify(osg::NOTICE) << "HMD device serial number: " << deviceSerialNumber << std::endl;
}

std::string  OpenVRDevice::GetDeviceProperty(vr::TrackedDeviceProperty prop)
{
	uint32_t  bufferLen = m_vrSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, prop, NULL, 0);

	if (bufferLen == 0)
	{
		return "";
	}

	char *buffer = new char[bufferLen];
	bufferLen = m_vrSystem->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, prop, buffer, bufferLen);
	std::string  result = buffer;
	delete[] buffer;

	return result;
}

void  OpenVRDevice::createRenderBuffers(osg::ref_ptr<osg::State> state)
{
	uint32_t  renderWidth  = 0;
	uint32_t  renderHeight = 0;

	m_vrSystem->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);

	for (int i = 0; i < 2; i++)
	{
		m_textureBuffer[i] = new OpenVRTextureBuffer(state, renderWidth, renderHeight, m_samples);
	}

	int  mirrorWidth  = 800;
	int  mirrorHeight = 450;
	m_mirrorTexture = new OpenVRMirrorTexture(state, mirrorWidth, mirrorHeight);
}

void  OpenVRDevice::init()
{
	calculateEyeAdjustment();
	calculateProjectionMatrices();
}

bool  OpenVRDevice::hmdPresent()
{
	return vr::VR_IsHmdPresent();
}

bool  OpenVRDevice::hmdInitialized() const
{
	return m_vrSystem != nullptr && m_vrRenderModels != nullptr;
}

osg::Matrix  OpenVRDevice::projectionMatrixCenter() const
{
	osg::Matrix  projectionMatrixCenter;

	projectionMatrixCenter = m_leftEyeProjectionMatrix.operator*(0.5) + m_rightEyeProjectionMatrix.operator*(0.5);

	return projectionMatrixCenter;
}

osg::Matrix  OpenVRDevice::projectionMatrixLeft() const
{
	return m_leftEyeProjectionMatrix;
}

osg::Matrix  OpenVRDevice::projectionMatrixRight() const
{
	return m_rightEyeProjectionMatrix;
}

osg::Matrix  OpenVRDevice::projectionOffsetMatrixLeft() const
{
	osg::Matrix  projectionOffsetMatrix;
	float        offset = m_leftEyeProjectionMatrix(2, 0);

	projectionOffsetMatrix.makeTranslate(osg::Vec3(-offset, 0.0, 0.0));

	return projectionOffsetMatrix;
}

osg::Matrix  OpenVRDevice::projectionOffsetMatrixRight() const
{
	osg::Matrix  projectionOffsetMatrix;
	float        offset = m_rightEyeProjectionMatrix(2, 0);

	projectionOffsetMatrix.makeTranslate(osg::Vec3(-offset, 0.0, 0.0));

	return projectionOffsetMatrix;
}

osg::Matrix  OpenVRDevice::viewMatrixLeft() const
{
	osg::Matrix  viewMatrix;

	viewMatrix.makeTranslate(-m_leftEyeAdjust);

	return viewMatrix;
}

osg::Matrix  OpenVRDevice::viewMatrixRight() const
{
	osg::Matrix  viewMatrix;

	viewMatrix.makeTranslate(-m_rightEyeAdjust);

	return viewMatrix;
}

void  OpenVRDevice::resetSensorOrientation() const
{
	m_vrSystem->ResetSeatedZeroPose();
}

void  OpenVRDevice::updatePose()
{
	vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseSeated);

	for (int i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		poses[i].bPoseIsValid = false;
	}

	vr::VRCompositor()->WaitGetPoses(poses, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	// Not sure why, but the openvr hellovr_opengl example only seems interested in the
	// pose transform from the first pose tracking device in the array.
	// i.e. this seems to be the only one that is used to affect the view transform matrix.
	// So, here we do the same.
	const vr::TrackedDevicePose_t &pose = poses[vr::k_unTrackedDeviceIndex_Hmd];

	if (pose.bPoseIsValid)
	{
		osg::Matrix  matrix        = convertMatrix34(pose.mDeviceToAbsoluteTracking);
		osg::Matrix  poseTransform = osg::Matrix::inverse(matrix);
		m_position    = poseTransform.getTrans() * m_worldUnitsPerMetre;
		m_orientation = poseTransform.getRotate();
		getControllerPose();
		// std::cout << m_position.x() << "," << m_position.y() << "," << m_position.z() << "," << std::endl;
	}
}

void  OpenVRDevice::getControllerPose()
{
	for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice)
	{
		if (!m_vrSystem->IsTrackedDeviceConnected(unTrackedDevice))
		{
			continue;
		}

		if (m_vrSystem->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
		{
			continue;
		}

		// 判断左右手控制器
		vr::ETrackedControllerRole  controllerRole = m_vrSystem->GetControllerRoleForTrackedDeviceIndex(unTrackedDevice);

		if (controllerRole == vr::TrackedControllerRole_RightHand)
		{
			if (poses[unTrackedDevice].bPoseIsValid)
			{
				osg::Matrix  matrix        = convertMatrix34(poses[unTrackedDevice].mDeviceToAbsoluteTracking);
				osg::Matrix  poseTransform = osg::Matrix::inverse(matrix);
				m_rightControllerPosition = poseTransform.getTrans() * m_worldUnitsPerMetre;
				m_rightOrientation        = poseTransform.getRotate();
				vr::HmdVector3_t  m_rightVelocity        = poses[unTrackedDevice].vVelocity;
				vr::HmdVector3_t  m_rightAngularVelocity = poses[unTrackedDevice].vAngularVelocity;

				// 存储最近100帧的pose数据
				if (frameIndex < 100)
				{
					controller_poses->push_back(m_rightControllerPosition);
					frameIndex++;
				}
				else
				{
					frameIndex = 0;
					double  x_total = 0, y_total = 0, z_total = 0;

					// 计算当前100帧的平均位置
					for (uint32_t i = 0; i < controller_poses->getNumElements(); i++)
					{
						osg::Vec3f  pose = controller_poses->at(i);
						x_total += pose.x();
						y_total += pose.y();
						z_total += pose.z();
						averagePosition.set(x_total / 100, y_total / 100, z_total / 100);
					}
				}

				// std::cout << m_rightControllerPosition.x() << "," << m_rightControllerPosition.y() << "," << m_rightControllerPosition.z() << std::endl;
			}
		}

		m_iTrackedControllerCount += 1;

		if (!poses[unTrackedDevice].bPoseIsValid)
		{
			continue;
		}
	}
}

osg::Camera * OpenVRDevice::createRTTCamera(OpenVRDevice::Eye eye, osg::Transform::ReferenceFrame referenceFrame, const osg::Vec4 &clearColor,
                                            osg::GraphicsContext *gc) const
{
	OpenVRTextureBuffer       *buffer = m_textureBuffer[eye];
	osg::ref_ptr<osg::Camera>  camera = new osg::Camera();

	camera->setClearColor(clearColor);
	camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
	camera->setRenderOrder(osg::Camera::PRE_RENDER, eye);
	camera->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	camera->setAllowEventFocus(false);
	camera->setReferenceFrame(referenceFrame);
	camera->setViewport(0, 0, buffer->textureWidth(), buffer->textureHeight());
	camera->setGraphicsContext(gc);

	// Here we avoid doing anything regarding OSG camera RTT attachment.
	// Ideally we would use automatic methods within OSG for handling RTT but in this
	// case it seemed simpler to handle FBO creation and selection within this class.

	// This initial draw callback is used to disable normal OSG camera setup which
	// would undo our RTT FBO configuration.
	camera->setInitialDrawCallback(new OpenVRInitialDrawCallback());

	camera->setPreDrawCallback(new OpenVRPreDrawCallback(camera.get(), buffer));
	camera->setFinalDrawCallback(new OpenVRPostDrawCallback(camera.get(), buffer));

	return camera.release();
}

bool  OpenVRDevice::submitFrame()
{
	vr::Texture_t           leftEyeTexture  = { (void *)m_textureBuffer[0]->getTexture(), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	vr::Texture_t           rightEyeTexture = { (void *)m_textureBuffer[1]->getTexture(), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
	vr::EVRCompositorError  lError          = vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
	vr::EVRCompositorError  rError          = vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);

	return lError == vr::VRCompositorError_None && rError == vr::VRCompositorError_None;
}

void  OpenVRDevice::blitMirrorTexture(osg::GraphicsContext *gc)
{
	m_mirrorTexture->blitTexture(gc, m_textureBuffer[0], m_textureBuffer[1]);
}

osg::GraphicsContext::Traits * OpenVRDevice::graphicsContextTraits() const
{
	osg::GraphicsContext::WindowingSystemInterface *wsi = osg::GraphicsContext::getWindowingSystemInterface();

	if (!wsi)
	{
		osg::notify(osg::NOTICE) << "Error, no WindowSystemInterface available, cannot create windows." << std::endl;

		return 0;
	}

	// Get the screen identifiers set in environment variable DISPLAY
	osg::GraphicsContext::ScreenIdentifier  si;
	si.readDISPLAY();

	// If displayNum has not been set, reset it to 0.
	if (si.displayNum < 0)
	{
		si.displayNum = 0;
		osg::notify(osg::INFO) << "Couldn't get display number, setting to 0" << std::endl;
	}

	// If screenNum has not been set, reset it to 0.
	if (si.screenNum < 0)
	{
		si.screenNum = 0;
		osg::notify(osg::INFO) << "Couldn't get screen number, setting to 0" << std::endl;
	}

	unsigned int  width, height;
	wsi->getScreenResolution(si, width, height);

	osg::ref_ptr<osg::GraphicsContext::Traits>  traits = new osg::GraphicsContext::Traits;
	traits->hostName         = si.hostName;
	traits->screenNum        = si.screenNum;
	traits->displayNum       = si.displayNum;
	traits->windowDecoration = true;
	traits->x                = 50;
	traits->y                = 50;
	traits->width            = 800;
	traits->height           = 450;
	traits->doubleBuffer     = true;
	traits->sharedContext    = nullptr;
	traits->vsync            = false; // VSync should always be disabled for because HMD submit handles the timing of the swap.

	return traits.release();
}

void  OpenVRDevice::shutdown(osg::GraphicsContext *gc)
{
	// Delete mirror texture
	if (m_mirrorTexture.valid())
	{
		m_mirrorTexture->destroy(gc);
		m_mirrorTexture = nullptr;
	}

	// Delete texture and depth buffers
	for (int i = 0; i < 2; i++)
	{
		if (m_textureBuffer[i].valid())
		{
			m_textureBuffer[i]->destroy(gc);
			m_textureBuffer[i] = nullptr;
		}
	}

	if (m_vrSystem != nullptr)
	{
		vr::VR_Shutdown();
		m_vrSystem = nullptr;
	}
}

// -----------------------------------------------------------------------------
// Purpose: Processes a single VR event
// -----------------------------------------------------------------------------
void  OpenVRDevice::ProcessVREvent(const vr::VREvent_t &event)
{
	vr::ETrackedControllerRole  controllerRole = m_vrSystem->GetControllerRoleForTrackedDeviceIndex(event.trackedDeviceIndex);

	prevState = state;

	m_vrSystem->GetControllerState(event.trackedDeviceIndex, &state, sizeof(state));

	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceActivated:
	{
		printf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		printf("Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		printf("Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_ButtonPress:
	{
		// 缩放 touchpad 按下上下位置
		// 使用touchpad 左右位置 进行 左右 旋转操作
		// 直接返回touchpad的坐标
		if (event.data.controller.button == vr::k_EButton_SteamVR_Touchpad)
		{
			m_vrSystem->GetControllerState(event.trackedDeviceIndex, &state, sizeof(state));

			if ((m_touchpadPreTouchPosition.x() != state.rAxis->x) || (m_touchpadPreTouchPosition.y() != state.rAxis->y))
			{
				m_touchpadPreTouchPosition.set(m_touchpadTouchPosition.x(), m_touchpadTouchPosition.y());
				m_touchpadTouchPosition.set(state.rAxis->x, state.rAxis->y);
				printf("touchPad previous position %d,%d", m_touchpadPreTouchPosition.x(), m_touchpadPreTouchPosition.y());
			}

			controllerEventResult = OpenVRDevice::Touchpad_Press;
		}

		/*else if (event.data.controller.button == vr::k_EButton_Grip)
		   {
		   controllerEventResult = 6;
		   }*/
		else if (event.data.controller.button == vr::EVRButtonId::k_EButton_SteamVR_Trigger)
		{
			// 主要用右手
			if (controllerRole == vr::TrackedControllerRole_RightHand)
			{
				controllerEventResult = Trigger_Press;
			}
		}
	}
	break;
	// 直到按键弹起后，其他按键才有效
	case vr::VREvent_ButtonUnpress:
	{
		// 右手
		if (controllerRole == vr::TrackedControllerRole_RightHand)
		{
			// touchpad 松开，旋转结束
			if (event.data.controller.button == vr::k_EButton_SteamVR_Touchpad)
			{
				printf("TrackedControllerRole_RightHand unpress.\n", event.trackedDeviceIndex);

				controllerEventResult = OpenVRDevice::Touchpad_Unpress;
			}
			else if (event.data.controller.button == vr::k_EButton_SteamVR_Trigger)
			{
				controllerEventResult = OpenVRDevice::Trigger_Unpress;
			}
		}
	}
	break;
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void  OpenVRDevice::HandleInput()
{
	// Process SteamVR events
	vr::VREvent_t  event;

	while (m_vrSystem->PollNextEvent(&event, sizeof(event)))
	{
		ProcessVREvent(event);
	}
}

/* Protected functions */
OpenVRDevice::~OpenVRDevice()
{
	// shutdown(gc);
}

void  OpenVRDevice::calculateEyeAdjustment()
{
	vr::HmdMatrix34_t  mat;

	mat              = m_vrSystem->GetEyeToHeadTransform(vr::Eye_Left);
	m_leftEyeAdjust  = convertMatrix34(mat).getTrans();
	mat              = m_vrSystem->GetEyeToHeadTransform(vr::Eye_Right);
	m_rightEyeAdjust = convertMatrix34(mat).getTrans();

	// Display IPD
	float  ipd = (m_leftEyeAdjust - m_rightEyeAdjust).length();
	osg::notify(osg::ALWAYS) << "Interpupillary Distance (IPD): " << ipd * 1000.0f << " mm" << std::endl;

	// Scale to world units
	m_leftEyeAdjust  *= m_worldUnitsPerMetre;
	m_rightEyeAdjust *= m_worldUnitsPerMetre;
}

void  OpenVRDevice::calculateProjectionMatrices()
{
	vr::HmdMatrix44_t  mat;

	mat                       = m_vrSystem->GetProjectionMatrix(vr::Eye_Left, m_nearClip, m_farClip);
	m_leftEyeProjectionMatrix = convertMatrix44(mat);

	mat                        = m_vrSystem->GetProjectionMatrix(vr::Eye_Right, m_nearClip, m_farClip);
	m_rightEyeProjectionMatrix = convertMatrix44(mat);
}

void  OpenVRDevice::trySetProcessAsHighPriority() const
{
	// Require at least 4 processors, otherwise the process could occupy the machine.
	if (OpenThreads::GetNumberOfProcessors() >= 4)
	{
#ifdef _WIN32
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
	}
}

void  OpenVRRealizeOperation::operator()(osg::GraphicsContext *gc)
{
	if (!m_realized)
	{
		OpenThreads::ScopedLock<OpenThreads::Mutex>  lock(_mutex);
		gc->makeCurrent();

		if (osgViewer::GraphicsWindow *window = dynamic_cast<osgViewer::GraphicsWindow *>(gc))
		{
			// Run wglSwapIntervalEXT(0) to force VSync Off
			window->setSyncToVBlank(false);
		}

		osg::ref_ptr<osg::State>  state = gc->getState();
		m_device->createRenderBuffers(state);
		// Init the openvr system
		m_device->init();
	}

	m_realized = true;
}

void  OpenVRSwapCallback::swapBuffersImplementation(osg::GraphicsContext *gc)
{
	// Submit rendered frame to compositor
	m_device->submitFrame();

	// Blit mirror texture to backbuffer
	m_device->blitMirrorTexture(gc);

	// Run the default system swapBufferImplementation
	gc->swapBuffersImplementation();

	// Update poses from HMD
	m_device->updatePose();

	m_device->HandleInput();
}
