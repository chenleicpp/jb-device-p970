# Copyright (C) 2012 The CyanogenMod Project
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

## Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Release name
PRODUCT_RELEASE_NAME := P970
# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit device configuration
$(call inherit-product, device/lge/p970/full_p970.mk)

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := p970
PRODUCT_NAME := cm_p970
PRODUCT_BRAND := LGE
PRODUCT_MODEL := LG Optimus Black

# Set build fingerprint
PRODUCT_BUILD_PROP_OVERRIDES += PRODUCT_NAME=lge_bproj BUILD_FINGERPRINT=lge/lge_bproj/bproj_262-XXX:2.3.4/GRJ22/LG-P970-V20o.47B5196A:user/release-keys PRIVATE_BUILD_DESC="lge_bproj-user 2.3.4 GRJ22 LG-P970-V20o.47B5196A release-keys"
