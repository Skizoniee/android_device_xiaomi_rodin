/*
 * Copyright (C) 2022 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.rodin"

#include <aidl/android/hardware/biometrics/fingerprint/BnFingerprint.h>
#include <android-base/logging.h>
#include <android-base/unique_fd.h>

#include <poll.h>
#include <sys/ioctl.h>
#include <fstream>
#include <thread>
#include <cstring>

#include "UdfpsHandler.h"
#include "mi_disp.h"

#define COMMAND_NIT 10
#define PARAM_NIT_FOD 1
#define PARAM_NIT_NONE 0

#define COMMAND_FOD_PRESS_STATUS 1
#define COMMAND_FOD_PRESS_X 2
#define COMMAND_FOD_PRESS_Y 3
#define PARAM_FOD_PRESSED 1
#define PARAM_FOD_RELEASED 0

#define FOD_STATUS_PATH "/sys/class/touch/touch_dev/fod_press_status"
#define FOD_STATUS_OFF 0
#define FOD_STATUS_ON 1

#define DISP_FEATURE_PATH "/dev/mi_display/disp_feature"

#define FINGERPRINT_ACQUIRED_VENDOR 7

// Touch device constants
#define TOUCH_DEV_PATH "/dev/xiaomi-touch"
#define TOUCH_ID 0
#define IOCTL_SET_TOUCH_ID 0x40005403
#define IOCTL_TOUCH_OPERATION 0xc4085400

// Touch modes
#define Touch_Fod_Enable 10

// Touch command types
#define CMD_SET_CUR_VALUE 0

using ::aidl::android::hardware::biometrics::fingerprint::AcquiredInfo;

namespace {

// Touch ioctl data structure
struct CommonDataPacket {
    uint8_t touchId;
    uint8_t command;
    uint16_t mode;
    uint16_t length;
    uint16_t reserved;
    int buffer[256];

    CommonDataPacket() : touchId(0), command(0), mode(0),
                         length(0), reserved(0) {
        memset(buffer, 0, sizeof(buffer));
    }
};

// Global touch device file descriptor
static int touchDeviceFd = -1;

/**
 * Initialize touch device for FOD operations
 */
static bool initTouchDevice() {
    if (touchDeviceFd >= 0) {
        return true;  // Already initialized
    }

    touchDeviceFd = open(TOUCH_DEV_PATH, O_RDWR);
    if (touchDeviceFd < 0) {
        LOG(ERROR) << "Failed to open touch device: " << strerror(errno);
        return false;
    }

    // Set touch device ID
    int result = ioctl(touchDeviceFd, IOCTL_SET_TOUCH_ID, (unsigned long)TOUCH_ID);
    if (result < 0) {
        LOG(ERROR) << "Failed to set touch ID: " << strerror(errno);
        close(touchDeviceFd);
        touchDeviceFd = -1;
        return false;
    }

    LOG(DEBUG) << "Touch device initialized successfully";
    return true;
}

/**
 * Set touch FOD mode using xiaomi_touch_ioctl
 */
static void setTouchFodMode(int value) {
    if (!initTouchDevice()) {
        LOG(ERROR) << "Touch device not initialized, cannot set FOD mode";
        return;
    }
    CommonDataPacket packet;
    packet.touchId = TOUCH_ID;
    packet.command = CMD_SET_CUR_VALUE;
    packet.mode = Touch_Fod_Enable;
    packet.length = 1;
    packet.buffer[0] = value;
    ioctl(touchDeviceFd, IOCTL_TOUCH_OPERATION, &packet);
}

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

static bool readBool(int fd) {
    char c;
    int rc;

    rc = lseek(fd, 0, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "failed to seek fd, err: " << rc;
        return false;
    }

    rc = read(fd, &c, sizeof(char));
    if (rc != 1) {
        LOG(ERROR) << "failed to read bool from fd, err: " << rc;
        return false;
    }

    return c != '0';
}

static disp_event_resp* parseDispEvent(int fd) {
    static char event_data[1024] = {0};
    ssize_t size = read(fd, event_data, sizeof(event_data));

    if (size < 0) {
        LOG(ERROR) << "read fod event failed";
        return nullptr;
    }
    if (size < sizeof(struct disp_event)) {
        LOG(ERROR) << "Invalid event size " << size << ", expect at least "
                   << sizeof(struct disp_event);
        return nullptr;
    }

    return (struct disp_event_resp*)&event_data[0];
}

}  // anonymous namespace

class XiaomiRodinUdfpsHandler : public UdfpsHandler {
  public:
    void init(fingerprint_device_t* device) {
        mDevice = device;
        disp_fd_ = android::base::unique_fd(open(DISP_FEATURE_PATH, O_RDWR));

        // Initialize touch device
        initTouchDevice();

        // Thread to listen for fod ui changes
        std::thread([this]() {
            int fd = open(DISP_FEATURE_PATH, O_RDWR);
            if (fd < 0) {
                LOG(ERROR) << "failed to open " << DISP_FEATURE_PATH << " , err: " << fd;
                return;
            }

            // Register for FOD events
            disp_event_req req;
            req.base.flag = 0;
            req.base.disp_id = MI_DISP_PRIMARY;
            req.type = MI_DISP_EVENT_FOD;
            ioctl(fd, MI_DISP_IOCTL_REGISTER_EVENT, &req);

            struct pollfd dispEventPoll = {
                    .fd = fd,
                    .events = POLLIN,
                    .revents = 0,
            };

            while (true) {
                int rc = poll(&dispEventPoll, 1, -1);
                if (rc < 0) {
                    LOG(ERROR) << "failed to poll " << DISP_FEATURE_PATH << ", err: " << rc;
                    continue;
                }

                struct disp_event_resp* response = parseDispEvent(fd);
                if (response == nullptr) {
                    continue;
                }

                if (response->base.type != MI_DISP_EVENT_FOD) {
                    LOG(ERROR) << "unexpected display event: " << response->base.type;
                    continue;
                }

                int value = response->data[0];
                LOG(DEBUG) << "received data: " << std::bitset<8>(value);

                bool localHbmUiReady = value & LOCAL_HBM_UI_READY;

                mDevice->extCmd(mDevice, COMMAND_NIT,
                                localHbmUiReady ? PARAM_NIT_FOD : PARAM_NIT_NONE);
            }
        }).detach();
    }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) {
        setTouchFodMode(1);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_PRESSED);
        // Request HBM
        disp_local_hbm_req req;
        req.base.flag = 0;
        req.base.disp_id = MI_DISP_PRIMARY;
        req.local_hbm_value = LHBM_TARGET_BRIGHTNESS_WHITE_1000NIT;
        ioctl(disp_fd_.get(), MI_DISP_IOCTL_SET_LOCAL_HBM, &req);
    }

    void onFingerUp() {
        setTouchFodMode(0);
        mDevice->extCmd(mDevice, COMMAND_FOD_PRESS_STATUS, PARAM_FOD_RELEASED);
        disp_local_hbm_req req;
        req.base.flag = 0;
        req.base.disp_id = MI_DISP_PRIMARY;
        req.local_hbm_value = LHBM_TARGET_BRIGHTNESS_OFF_FINGER_UP;
        ioctl(disp_fd_.get(), MI_DISP_IOCTL_SET_LOCAL_HBM, &req);
        setFodStatus(FOD_STATUS_OFF);
    }

    void onAcquired(int32_t result, int32_t vendorCode) {
        LOG(DEBUG) << __func__ << " result: " << result << " vendorCode: " << vendorCode;
        if (result != FINGERPRINT_ACQUIRED_VENDOR) {
            switch (static_cast<AcquiredInfo>(result)) {
                case AcquiredInfo::GOOD:
                case AcquiredInfo::PARTIAL:
                case AcquiredInfo::INSUFFICIENT:
                case AcquiredInfo::SENSOR_DIRTY:
                case AcquiredInfo::TOO_SLOW:
                case AcquiredInfo::TOO_FAST:
                case AcquiredInfo::TOO_DARK:
                case AcquiredInfo::TOO_BRIGHT:
                case AcquiredInfo::IMMOBILE:
                case AcquiredInfo::LIFT_TOO_SOON:
                    onFingerUp();
                    break;
                default:
                    break;
            }
        } else if (vendorCode == 21 || vendorCode == 23) {
            /*
             * vendorCode = 21 waiting for fingerprint authentication
             * vendorCode = 23 waiting for fingerprint enroll
             */
            setFodStatus(FOD_STATUS_ON);
        }
    }

    void onAuthenticationSucceeded() { onFingerUp(); }

    void onAuthenticationFailed() { onFingerUp(); }

  private:
    fingerprint_device_t* mDevice;
    android::base::unique_fd disp_fd_;

    void setFodStatus(int value) {
        setTouchFodMode(value);
        set(FOD_STATUS_PATH, value);
    }
};

static UdfpsHandler* create() {
    return new XiaomiRodinUdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
        .create = create,
        .destroy = destroy,
};