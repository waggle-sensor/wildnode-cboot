################################### tell Emacs this is a -*- makefile-gmake -*-
#
# Copyright (c) 2014-2018, NVIDIA CORPORATION.  All rights reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.  Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
###############################################################################

# Build version format: BB.bb.MM.DD.YYYY.WW
# BB.bb   - Branch (rel<BB>r<bb>)
# MM.DD   - Month.Day
# YYYY.WW - Year.Week of updating the version
BUILD_BRANCH := $(shell $(LKROOT)/build/get_branch_name.py)
BUILD_DATE := $(shell date +"%m.%d.%Y.%U")
BUILD_VERSION := $(BUILD_BRANCH).$(BUILD_DATE)
VERSION_MAX_LEN := 64
