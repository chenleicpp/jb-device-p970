# Copyright (C) 2011 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# The gps config appropriate for this device
$(call inherit-product, device/common/gps/gps_us_supl.mk)

PRODUCT_TAGS += dalvik.gc.type-precise
$(call inherit-product, frameworks/base/build/phone-hdpi-512-dalvik-heap.mk)

PRODUCT_LOCALES += hdpi

DEVICE_PACKAGE_OVERLAYS += device/lge/p970/overlay


ifeq ($(TARGET_PREBUILT_KERNEL),)
	LOCAL_KERNEL := device/lge/p970/kernel
else
	LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel

## Dummy file to help RM identify the model
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/dummy-rm:root/bootimages/ON_480x800_08fps_0000.rle

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/init.p970.rc:root/init.rc \
    $(LOCAL_PATH)/prebuilt/init:root/init \
    $(LOCAL_PATH)/init.p970.usb.rc:root/init.p970.usb.rc \
    $(LOCAL_PATH)/ueventd.p970.rc:root/ueventd.lge.rc \
    $(LOCAL_PATH)/configs/vold.fstab:system/etc/vold.fstab \
    $(LOCAL_PATH)/prebuilt/g-recovery:root/sbin/g-recovery

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_charging_01.rle:root/chargerimages/battery_charging_01.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_charging_02.rle:root/chargerimages/battery_charging_02.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_charging_03.rle:root/chargerimages/battery_charging_03.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_charging_04.rle:root/chargerimages/battery_charging_04.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_charging_05.rle:root/chargerimages/battery_charging_05.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_charging_06.rle:root/chargerimages/battery_charging_06.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_very_low.rle:root/chargerimages/battery_very_low.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_wait_01.rle:root/chargerimages/battery_wait_01.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/battery_wait_02.rle:root/chargerimages/battery_wait_02.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/black_bg.rle:root/chargerimages/black_bg.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/charger_background.rle:root/chargerimages/charger_background.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/dummy_battery.rle:root/chargerimages/dummy_battery.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/lockscreen_charge_ani_no_battery.rle:root/chargerimages/lockscreen_charge_ani_no_battery.rle \
    $(LOCAL_PATH)/chargemode/chargerimages/white_lg_logo.rle:root/chargerimages/white_lg_logo.rle \
    $(LOCAL_PATH)/chargemode/chargerlogo:root/sbin/chargerlogo

# Recovery
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/recovery/postrecoveryboot.sh:recovery/root/sbin/postrecoveryboot.sh
    
# Permission files
PRODUCT_COPY_FILES += \
    frameworks/base/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/base/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/base/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/base/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/base/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/base/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/base/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
    frameworks/base/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/base/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml

# RIL stuffs
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/ipc_channels.config:system/etc/ipc_channels.config \
    $(LOCAL_PATH)/init.vsnet:system/bin/init.vsnet

## GPS
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/gps_brcm_conf.xml:system/etc/gps_brcm_conf.xml

## Wifi

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifimac/wlan-precheck:system/bin/wlan-precheck \
    $(LOCAL_PATH)/prebuilt/wireless.ko:system/lib/modules/wireless.ko \
    $(LOCAL_PATH)/prebuilt/scsi_wait_scan.ko:system/lib/modules/scsi_wait_scan.ko \
    $(LOCAL_PATH)/configs/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    $(LOCAL_PATH)/configs/nvram.txt:system/etc/wifi/nvram.txt \
    $(LOCAL_PATH)/configs/dhcpcd.conf:system/etc/dhcpcd/dhcpcd.conf

## Alsa configs
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/asound.conf:system/etc/asound.conf \
    $(LOCAL_PATH)/configs/alsa/alsa.conf:system/usr/share/alsa/alsa.conf \
    $(LOCAL_PATH)/configs/alsa/cards/aliases.conf:system/usr/share/alsa/cards/aliases.conf \
    $(LOCAL_PATH)/configs/alsa/init/00main:system/usr/share/alsa/init/00main \
    $(LOCAL_PATH)/configs/alsa/init/default:system/usr/share/alsa/init/default \
    $(LOCAL_PATH)/configs/alsa/init/hda:system/usr/share/alsa/init/hda \
    $(LOCAL_PATH)/configs/alsa/init/help:system/usr/share/alsa/init/help \
    $(LOCAL_PATH)/configs/alsa/init/info:system/usr/share/alsa/init/info \
    $(LOCAL_PATH)/configs/alsa/init/test:system/usr/share/alsa/init/test \
    $(LOCAL_PATH)/configs/alsa/pcm/center_lfe.conf:system/usr/share/alsa/pcm/center_lfe.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/default.conf:system/usr/share/alsa/pcm/default.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/dmix.conf:system/usr/share/alsa/pcm/dmix.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/dpl.conf:system/usr/share/alsa/pcm/dpl.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/dsnoop.conf:system/usr/share/alsa/pcm/dsnoop.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/front.conf:system/usr/share/alsa/pcm/front.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/iec958.conf:system/usr/share/alsa/pcm/iec958.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/modem.conf:system/usr/share/alsa/pcm/modem.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/rear.conf:system/usr/share/alsa/pcm/rear.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/side.conf:system/usr/share/alsa/pcm/side.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/surround40.conf:system/usr/share/alsa/pcm/surround40.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/surround41.conf:system/usr/share/alsa/pcm/surround41.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/surround50.conf:system/usr/share/alsa/pcm/surround50.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/surround51.conf:system/usr/share/alsa/pcm/surround51.conf \
    $(LOCAL_PATH)/configs/alsa/pcm/surround71.conf:system/usr/share/alsa/pcm/surround71.conf

## Keylayout
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/keylayout/hub_synaptics_touch.kl:system/usr/keylayout/hub_synaptics_touch.kl \
    $(LOCAL_PATH)/keylayout/twl4030_pwrbutton.kl:system/usr/keylayout/twl4030_pwrbutton.kl \
    $(LOCAL_PATH)/keylayout/TWL4030_Keypad.kl:system/usr/keylayout/TWL4030_Keypad.kl

## Touchscreen Calibration
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/idc/hub_synaptics_touch.idc:system/usr/idc/hub_synaptics_touch.idc
PRODUCT_PACKAGES += \
    libskiahw \
    libmemalloc \
    liboverlay \
    prb \
    wifimac
	
# ICS sound
PRODUCT_PACKAGES += \
	hcitool \
	hciattach \
	hcidump \
	libaudioutils \
	audio.a2dp.default \
	libaudiohw_legacy

# OMX components
PRODUCT_PACKAGES += \
    libstagefrighthw \
    libdivxdrmdecrypt \
    libOmxVdec \
    libOmxVenc \
    libOmxAacEnc \
    libOmxAmrEnc \
    libmm-omxcore \
    libOmxCore
    
# HWComposer
PRODUCT_PACKAGES += hwcomposer.default

#Camera
PRODUCT_PACKAGES += camera.p970

# OpenMAX IL configuration
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/prebuilt/policytable.tbl:system/etc/policytable.tbl \
    $(LOCAL_PATH)/media_profiles.xml:system/etc/media_profiles.xml

PRODUCT_PACKAGES += \
    libomap_mm_library_jni

FRAMEWORKS_BASE_SUBDIRS += \
    $(addsuffix /java, omapmmlib )

$(call inherit-product, build/target/product/full.mk)

# See comment at the top of this file. This is where the other
# half of the device-specific product definition file takes care
# of the aspects that require proprietary drivers that aren't
# commonly available
$(call inherit-product-if-exists, vendor/lge/p970/p970-vendor.mk)
