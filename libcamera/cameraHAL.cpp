/*
 * Copyright (C) 2012 ShenduOS Team
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#define LOG_TAG "***CameraHAL***"
#define LOG_NDEBUG 0

#include "CameraHardwareInterface.h"
#include <hardware/hardware.h>
#include <hardware/camera.h>
#include <binder/IMemory.h>
#include <camera/CameraParameters.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <hardware/gralloc.h>
#include <ui/Overlay.h>

using android::sp;
using android::Overlay;
using android::String8;
using android::IMemory;
using android::IMemoryHeap;
using android::CameraParameters;
using android::CameraInfo;
using android::HAL_getCameraInfo;
using android::HAL_getNumberOfCameras;
using android::HAL_openCameraHardware;
using android::CameraHardwareInterface;

#define LOG_FUNCTION_NAME           ALOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define NO_ERROR 0

extern "C" android::sp<android::CameraHardwareInterface> HAL_openCameraHardware(int cameraId);
extern "C" int HAL_getNumberOfCameras();
extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo* cameraInfo);

int camera_device_open(const hw_module_t* module, const char* name,
                        hw_device_t** device);
int camera_get_camera_info(int camera_id, struct camera_info *info);
int camera_get_number_of_cameras(void);

static hw_module_methods_t camera_module_methods = {
    open: camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
        id: CAMERA_HARDWARE_MODULE_ID,
        name: "Camera HAL for P970 ICS",
        author: "ChenLei",
        methods: &camera_module_methods,
        dso: NULL,
        reserved: {0},
    },
    get_number_of_cameras: camera_get_number_of_cameras,
    get_camera_info: camera_get_camera_info,
};

static inline struct legacy_camera_device * to_lcdev(struct camera_device *dev)
{
    return reinterpret_cast<struct legacy_camera_device *>(dev);
}


struct legacy_camera_device {
    camera_device_t device;
    android::sp<android::CameraHardwareInterface> hwif;
    int id;

    camera_notify_callback         notify_callback;
    camera_data_callback           data_callback;
    camera_data_timestamp_callback data_timestamp_callback;
    camera_request_memory          request_memory;
    void                          *user;

    preview_stream_ops *window;
    gralloc_module_t const *gralloc;
    camera_memory_t *clientData;

    sp<Overlay> overlay;
    int preview_width;
    int preview_height;
};

static struct {
    int type;
    const char *text;
} msg_map[] = {
    {0x0001, "CAMERA_MSG_ERROR"},
    {0x0002, "CAMERA_MSG_SHUTTER"},
    {0x0004, "CAMERA_MSG_FOCUS"},
    {0x0008, "CAMERA_MSG_ZOOM"},
    {0x0010, "CAMERA_MSG_PREVIEW_FRAME"},
    {0x0020, "CAMERA_MSG_VIDEO_FRAME"},
    {0x0040, "CAMERA_MSG_POSTVIEW_FRAME"},
    {0x0080, "CAMERA_MSG_RAW_IMAGE"},
    {0x0100, "CAMERA_MSG_COMPRESSED_IMAGE"},
    {0x0200, "CAMERA_MSG_RAW_IMAGE_NOTIFY"},
    {0x0400, "CAMERA_MSG_PREVIEW_METADATA"},
    {0x0000, "CAMERA_MSG_ALL_MSGS"}, //0xFFFF
    {0x0000, "NULL"},
};

static void dump_msg(const char *tag, int msg_type)
{
    int i;
    for (i = 0; msg_map[i].type; i++) {
        if (msg_type & msg_map[i].type) {
            ALOGD("%s: %s", tag, msg_map[i].text);
        }
    }
}

/*******************************************************************
 * overlay hook
 *******************************************************************/

static void wrap_set_fd_hook(void *data, int fd)
{
    legacy_camera_device* dev = NULL;
    if(!data)
        return;
    dev = (legacy_camera_device*) data;
}

static void wrap_set_crop_hook(void *data,
                               uint32_t x, uint32_t y,
                               uint32_t w, uint32_t h)
{
    legacy_camera_device* dev = NULL;
    if(!data)
        return;
    dev = (legacy_camera_device*) data;
}

static void wrap_queue_buffer_hook(void *data, void* buffer)
{
    sp<IMemoryHeap> heap;
    legacy_camera_device* dev = NULL;
    preview_stream_ops* window = NULL;
    //LOGI("%s+++: %p", __FUNCTION__,data);

    if(!data)
        return;

    dev = (legacy_camera_device*) data;

    window = dev->window;

    if(window == 0)
		return;

    heap =  dev->hwif->getPreviewHeap();
    if(heap == 0)
		return;

    int offset = (int)buffer;
    char *frame = (char *)(heap->base()) + offset;

    int stride;
    void *vaddr;
    buffer_handle_t *buf_handle;

    int width = dev->preview_width;
    int height = dev->preview_height;
    if (0 != window->dequeue_buffer(window, &buf_handle, &stride)) {
        ALOGE("%s: could not dequeue gralloc buffer", __FUNCTION__);
        goto skipframe;
    }
    if (0 == dev->gralloc->lock(dev->gralloc, *buf_handle,
                                GRALLOC_USAGE_SW_WRITE_MASK,
                                0, 0, width, height, &vaddr)) {
        // the code below assumes YUV, not RGB
        memcpy(vaddr, frame, width * height * 3 / 2);
        //LOGI("%s: copy frame to gralloc buffer", __FUNCTION__);
    } else {
        ALOGE("%s: could not lock gralloc buffer", __FUNCTION__);
        goto skipframe;
    }

    dev->gralloc->unlock(dev->gralloc, *buf_handle);

    if (0 != window->enqueue_buffer(window, buf_handle)) {
        ALOGE("%s: could not dequeue gralloc buffer", __FUNCTION__);
        goto skipframe;
    }
skipframe:
    ALOGI("%s---: ", __FUNCTION__);

    return;
}

/*******************************************************************
 * camera interface callback
 *******************************************************************/

camera_memory_t * wrap_memory_data(const android::sp<android::IMemory> &dataPtr,
                        void *user)
{
    LOG_FUNCTION_NAME
    ssize_t          offset;
    size_t           size;
    camera_memory_t *clientData = NULL;
    struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;
    android::sp<android::IMemoryHeap> mHeap = dataPtr->getMemory(&offset, &size);

    clientData = lcdev->request_memory(-1, size, 1, lcdev->user);
    if (clientData != NULL) {
        memcpy(clientData->data, (char *)(mHeap->base()) + offset, size);
    } else {
        ALOGE("wrap_memory_data: ERROR allocating memory from client\n");
    }
    return clientData;
}

void wrap_notify_callback(int32_t msg_type, int32_t ext1,
                   int32_t ext2, void *user)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;
    ALOGD("%s: msg_type:%d ext1:%d ext2:%d user:%p\n",__FUNCTION__,
        msg_type, ext1, ext2, user);
    dump_msg(__FUNCTION__, msg_type);
    if (lcdev->notify_callback != NULL) {
        lcdev->notify_callback(msg_type, ext1, ext2, lcdev->user);
    }
}

void wrap_data_callback(int32_t msg_type, const android::sp<android::IMemory>& dataPtr,
                 void *user)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;

    ALOGD("%s: msg_type:%d user:%p\n",__FUNCTION__,msg_type, user);
    dump_msg(__FUNCTION__, msg_type);

    if (lcdev->data_callback != NULL && lcdev->request_memory != NULL) {
      /* Make sure any pre-existing heap is released */
        if (lcdev->clientData != NULL) {
            lcdev->clientData->release(lcdev->clientData);
            lcdev->clientData = NULL;
        }
        lcdev->clientData = wrap_memory_data(dataPtr, lcdev);
        if (lcdev->clientData != NULL) {
            lcdev->data_callback(msg_type, lcdev->clientData, 0, NULL, lcdev->user);
        }
   }
}

void wrap_data_callback_timestamp(nsecs_t timestamp, int32_t msg_type,
                   const android::sp<android::IMemory>& dataPtr, void *user)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;

    ALOGD("%s: timestamp:%lld msg_type:%d user:%p\n",__FUNCTION__,
        timestamp /1000, msg_type, user);
    dump_msg(__FUNCTION__, msg_type);

    if (lcdev->data_callback != NULL && lcdev->request_memory != NULL) {
        camera_memory_t *clientData = wrap_memory_data(dataPtr, lcdev);
        if (clientData != NULL) {
            ALOGD("wrap_data_callback_timestamp: Posting data to client timestamp:%lld\n",
              systemTime());
            lcdev->data_timestamp_callback(timestamp, msg_type, clientData, 0, lcdev->user);
            lcdev->hwif->releaseRecordingFrame(dataPtr);
        } else {
            ALOGD("wrap_data_callback_timestamp: ERROR allocating memory from client\n");
      }
   }
}

/*******************************************************************
 * implementation of camera_device_ops_t functions
 *******************************************************************/

void camera_fixup_params(android::CameraParameters &camParams)
{
    //this is for test compare from cm7 camera setting params
    const char *preview_size = "640x480";
    const char *picture_size_values = "2592x1944,2048x1536,1600x1200,1280x960,1280x720,640x480,512x384,320x240";
    //const char *picture_size = "2592x1944";
    //const char *preview_fps_range_values = "(10000,10000),(15000,15000),(24000,24000),(30000,30000),(8000,33000)";
    //const char *preview_fps_range = "30000,30000";
    const char *preview_frame_rate_values = "10,15,24,30";
    const char *preview_frame_rate = "30";
    //camParams.set(android::CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV422I);
    //camParams.setPreviewFormat(android::CameraParameters::PIXEL_FORMAT_YUV422I);
    camParams.set(android::CameraParameters::KEY_PREVIEW_SIZE,preview_size);   
    camParams.set(android::CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,picture_size_values);
    //camParams.set(android::CameraParameters::KEY_PICTURE_SIZE,picture_size);
    //camParams.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,preview_fps_range_values);
    //camParams.set(android::CameraParameters::KEY_PREVIEW_FPS_RANGE,preview_fps_range);
    camParams.set(android::CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,preview_frame_rate_values);
    camParams.set(android::CameraParameters::KEY_PREVIEW_FRAME_RATE,preview_frame_rate);
}

int camera_get_camera_info(int camera_id, struct camera_info *info)
{
	LOG_FUNCTION_NAME
	int rv = 0;
   	android::CameraInfo cam_info;
   	android::HAL_getCameraInfo(camera_id, &cam_info);
   	info->facing = cam_info.facing;
   	info->orientation = cam_info.orientation;
   	ALOGD("%s: id:%i faceing:%i orientation: %i", __FUNCTION__,
        camera_id, info->facing, info->orientation);
   	return rv;
}

int camera_get_number_of_cameras(void)
{
	LOG_FUNCTION_NAME
	int num_cameras = HAL_getNumberOfCameras();
    ALOGD("%s: number:%i", __FUNCTION__, num_cameras);
    return num_cameras;
}

int camera_set_preview_window(struct camera_device * device,
                           struct preview_stream_ops *window)
{
    LOG_FUNCTION_NAME
   	int rv = -EINVAL;
    int min_bufs = -1;
    int kBufferCount = 6;
    struct legacy_camera_device *lcdev = to_lcdev(device);

    ALOGV("%s : Window :%p\n",__FUNCTION__, window);
    if (device == NULL) {
        ALOGE("camera_set_preview_window : Invalid device.\n");
        return -EINVAL;
    }

    if (lcdev->window == window) {
        return 0;
    }

    lcdev->window = window;

    if (!window) {
        ALOGD("%s: window is NULL", __FUNCTION__);
        return -EINVAL;
    }

    ALOGV("%s : OK window is %p", __FUNCTION__, window);

    if (!lcdev->gralloc) {
        hw_module_t const* module;
        int err = 0;
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module) == 0) {
            lcdev->gralloc = (const gralloc_module_t *)module;
        } else {
            ALOGE("%s: Fail on loading gralloc HAL", __FUNCTION__);
        }
    }


    ALOGV("%s: OK on loading gralloc HAL", __FUNCTION__);
    if (window->get_min_undequeued_buffer_count(window, &min_bufs)) {
        ALOGE("%s: could not retrieve min undequeued buffer count", __FUNCTION__);
        return -1;
    }
    ALOGV("%s: OK get_min_undequeued_buffer_count", __FUNCTION__);

    ALOGV("%s: bufs:%i", __FUNCTION__, min_bufs);
    if (min_bufs >= kBufferCount) {
        ALOGE("%s: min undequeued buffer count %i is too high (expecting at most %i)",
          __FUNCTION__, min_bufs, kBufferCount - 1);
    }

    ALOGV("%s: setting buffer count to %i", __FUNCTION__, kBufferCount);
    if (window->set_buffer_count(window, kBufferCount)) {
        ALOGE("%s: could not set buffer count", __FUNCTION__);
        return -1;
    }

    int w, h;
    android::CameraParameters params(lcdev->hwif->getParameters());
    params.getPreviewSize(&w, &h);
    int hal_pixel_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
    const char *str_preview_format = params.getPreviewFormat();
    ALOGV("%s: preview format %s", __FUNCTION__, str_preview_format);

    if (window->set_usage(window, GRALLOC_USAGE_SW_WRITE_MASK)) {
        ALOGE("%s: could not set usage on gralloc buffer", __FUNCTION__);
        return -1;
    }

    if (window->set_buffers_geometry(window, w, h, hal_pixel_format)) {
        ALOGE("%s: could not set buffers geometry to %s",
          __FUNCTION__, str_preview_format);
      return -1;
    }

    lcdev->preview_width = w;
    lcdev->preview_height = h;

    lcdev->overlay =  new Overlay(wrap_set_fd_hook,
                                wrap_set_crop_hook,
                                wrap_queue_buffer_hook,
                                (void *)lcdev);
    lcdev->hwif->setOverlay(lcdev->overlay);

    return NO_ERROR;
}



void camera_set_callbacks(struct camera_device * device,
                      camera_notify_callback notify_cb,
                      camera_data_callback data_cb,
                      camera_data_timestamp_callback data_cb_timestamp,
                      camera_request_memory get_memory, 
                      void *user)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    ALOGV("%s: notify_cb: %p, data_cb: %p "
         "data_cb_timestamp: %p, get_memory: %p, user :%p",__FUNCTION__,
         notify_cb, data_cb, data_cb_timestamp, get_memory, user);

    lcdev->notify_callback = notify_cb;
    lcdev->data_callback = data_cb;
    lcdev->data_timestamp_callback = data_cb_timestamp;
    lcdev->request_memory = get_memory;
    lcdev->user = user;

    lcdev->hwif->setCallbacks(wrap_notify_callback, wrap_data_callback,wrap_data_callback_timestamp, (void *) lcdev);
}

void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    ALOGV("%s: msg_type:%d\n",__FUNCTION__,msg_type);
    dump_msg(__FUNCTION__, msg_type);
    lcdev->hwif->enableMsgType(msg_type);
}

void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    ALOGV("camera_disable_msg_type: msg_type:%d\n", msg_type);
    dump_msg(__FUNCTION__, msg_type);
    lcdev->hwif->disableMsgType(msg_type);
}

int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    ALOGV("camera_msg_type_enabled: msg_type:%d\n", msg_type);
    dump_msg(__FUNCTION__, msg_type);
    return lcdev->hwif->msgTypeEnabled(msg_type);
}

int camera_start_preview(struct camera_device * device)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    ALOGV("camera_start_preview: Enabling CAMERA_MSG_PREVIEW_FRAME\n");
    if (!lcdev->hwif->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        lcdev->hwif->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
    }
    return lcdev->hwif->startPreview();
}

void camera_stop_preview(struct camera_device * device)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    ALOGV("camera_stop_preview:\n");
    if(lcdev->hwif->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)){
        lcdev->hwif->disableMsgType(CAMERA_MSG_PREVIEW_FRAME);
    }
    lcdev->hwif->stopPreview();
    return;
}

int camera_preview_enabled(struct camera_device * device)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    int ret = lcdev->hwif->previewEnabled();
    ALOGV("%s: %d\n",__FUNCTION__,ret);
    return ret;
}

int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

int camera_start_recording(struct camera_device * device)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    lcdev->hwif->enableMsgType(CAMERA_MSG_VIDEO_FRAME);
    lcdev->hwif->startRecording();
    return NO_ERROR;
}

void camera_stop_recording(struct camera_device * device)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    lcdev->hwif->disableMsgType(CAMERA_MSG_VIDEO_FRAME);
    lcdev->hwif->stopRecording();
}

int camera_recording_enabled(struct camera_device * device)
{
	LOG_FUNCTION_NAME
	struct legacy_camera_device *lcdev = to_lcdev(device);
    return (int)lcdev->hwif->recordingEnabled();
}

void camera_release_recording_frame(struct camera_device * device,
                                const void *opaque)
{
	LOG_FUNCTION_NAME
}

int camera_auto_focus(struct camera_device * device)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    lcdev->hwif->autoFocus();
	return NO_ERROR;
}

int camera_cancel_auto_focus(struct camera_device * device)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    lcdev->hwif->cancelAutoFocus();
    return NO_ERROR;
}

int camera_take_picture(struct camera_device * device)
{
	LOG_FUNCTION_NAME
	struct legacy_camera_device *lcdev = to_lcdev(device);
    lcdev->hwif->takePicture();
    return NO_ERROR;
}

int camera_cancel_picture(struct camera_device * device)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    lcdev->hwif->cancelPicture();
    return NO_ERROR; 
}

int camera_set_parameters(struct camera_device * device, const char *params)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    android::String8 s(params);
    android::CameraParameters p(s);
    lcdev->hwif->setParameters(p);
    return NO_ERROR;
}

char* camera_get_parameters(struct camera_device * device)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    char *rc = NULL;
    android::CameraParameters params(lcdev->hwif->getParameters());
    camera_fixup_params(params);
    rc = strdup((char *)params.flatten().string());
    ALOGV("%s: returning rc:%p :%s\n",__FUNCTION__,
        rc, (rc != NULL) ? rc : "EMPTY STRING");
    return rc;
}

void camera_put_parameters(struct camera_device *device, char *params)
{
    LOG_FUNCTION_NAME
    ALOGV("%s: params:%p %s",__FUNCTION__ , params, params);
    free(params);
}

int camera_send_command(struct camera_device * device, int32_t cmd,
                        int32_t arg0, int32_t arg1)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    ALOGV("camera_send_command: cmd:%d arg0:%d arg1:%d\n",
        cmd, arg0, arg1);
    return lcdev->hwif->sendCommand(cmd, arg0, arg1);
}

void camera_release(struct camera_device * device)
{
	LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    lcdev->hwif->release();
}

int camera_dump(struct camera_device * device, int fd)
{
    LOG_FUNCTION_NAME
    struct legacy_camera_device *lcdev = to_lcdev(device);
    android::Vector<android::String16> args;
    return lcdev->hwif->dump(fd, args);
}

int camera_device_close(hw_device_t* device)
{
    LOG_FUNCTION_NAME
    struct camera_device * hwdev = reinterpret_cast<struct camera_device *>(device);
    struct legacy_camera_device *lcdev = to_lcdev(hwdev);
    int rc = -EINVAL;
    ALOGV("camera_device_close\n");
    if (lcdev != NULL) {
        camera_device_ops_t *camera_ops = lcdev->device.ops;
        if (camera_ops) {
            free(camera_ops);
            camera_ops = NULL;
        }
    free(lcdev);
    rc = NO_ERROR;
    }
    return rc;

}

int camera_device_open(const hw_module_t* module, const char* name,
                   hw_device_t** device)
{
	LOG_FUNCTION_NAME
   	int ret;
   	struct legacy_camera_device *lcdev;
   	camera_device_t* camera_device;
   	camera_device_ops_t* camera_ops;

   	if (name == NULL)
      	return 0;

   	int cameraId = atoi(name);

   	ALOGD("%s: name:%s device:%p cameraId:%d\n",__FUNCTION__,
        name, device, cameraId);

   	lcdev = (struct legacy_camera_device *)calloc(1, sizeof(*lcdev));
   	camera_ops = (camera_device_ops_t *)malloc(sizeof(*camera_ops));
   	memset(camera_ops, 0, sizeof(*camera_ops));

   	lcdev->device.common.tag               = HARDWARE_DEVICE_TAG;
   	lcdev->device.common.version           = 0;
   	lcdev->device.common.module            = (hw_module_t *)(module);
   	lcdev->device.common.close             = camera_device_close;
   	lcdev->device.ops                      = camera_ops;

   	camera_ops->set_preview_window         = camera_set_preview_window;
   	camera_ops->set_callbacks              = camera_set_callbacks;
   	camera_ops->enable_msg_type            = camera_enable_msg_type;
   	camera_ops->disable_msg_type           = camera_disable_msg_type;
   	camera_ops->msg_type_enabled           = camera_msg_type_enabled;
   	camera_ops->start_preview              = camera_start_preview;
   	camera_ops->stop_preview               = camera_stop_preview;
   	camera_ops->preview_enabled            = camera_preview_enabled;
   	camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
   	camera_ops->start_recording            = camera_start_recording;
   	camera_ops->stop_recording             = camera_stop_recording;
   	camera_ops->recording_enabled          = camera_recording_enabled;
   	camera_ops->release_recording_frame    = camera_release_recording_frame;
   	camera_ops->auto_focus                 = camera_auto_focus;
   	camera_ops->cancel_auto_focus          = camera_cancel_auto_focus;
   	camera_ops->take_picture               = camera_take_picture;
   	camera_ops->cancel_picture             = camera_cancel_picture;
   	camera_ops->set_parameters             = camera_set_parameters;
   	camera_ops->get_parameters             = camera_get_parameters;
   	camera_ops->put_parameters             = camera_put_parameters;
   	camera_ops->send_command               = camera_send_command;
   	camera_ops->release                    = camera_release;
   	camera_ops->dump                       = camera_dump;

   	lcdev->id = cameraId;
   	lcdev->hwif = HAL_openCameraHardware(cameraId);
   	if (lcdev->hwif == NULL) {
       	ret = -EIO;
       	goto err_create_camera_hw;
   	}
   	*device = &lcdev->device.common;
   	return NO_ERROR;

err_create_camera_hw:
   	free(lcdev);
   	free(camera_ops);
   	return ret;
}
