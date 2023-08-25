################################################################################
# 
# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2020 Sky
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
bin_PROGRAMS = rdkledmanager

AM_CPPFLAGS = -I$(top_srcdir)/../CcspCommonLibrary/source/ccsp/components/include \
        -I$(top_srcdir)/source/LogComponent \
        -I$(top_srcdir)/../CcspCommonLibrary/source/debug_api/include \
        -I$(top_srcdir)/../hal/include \
        -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)

if FEATURE_LEDMGR_WEBCONFIG
AM_CPPFLAGS += -I$(top_srcdir)/source/TR-181/middle_layer_src
endif

rdkledmanager_DEPENDENCIES = \
        $(EXTRA_DEPENDENCIES)

if FEATURE_LEDMGR_WEBCONFIG
EXTRA_DEPENDENCIES= \
        ${top_builddir}/source/TR-181/middle_layer_src/libCcspLedManager_middle_layer_src.la
endif

rdkledmanager_CFLAGS = -Werror -D_ANSC_LINUX -D_ANSC_USER -D_ANSC_LITTLE_ENDIAN_ -DFEATURE_SUPPORT_RDKLOG $(DBUS_CFLAGS) $(SYSTEMD_CFLAGS)
rdkledmanager_SOURCES = led_manager.c led_manager_events.c led_manager_utils.c led_manager_dbus_utils.c
rdkledmanager_LDFLAGS = -lccsp_common -lrdkloggers -ljson-c $(DBUS_LIBS) $(SYSTEMD_LDFLAGS) -lsysevent -lhal_platform -lledmanager -lsky_hub
if FEATURE_LEDMGR_WEBCONFIG
rdkledmanager_SOURCES += ledmgr_webconfig.c
rdkledmanager_LDFLAGS += -lpthread -lrbus -lsyscfg -lmsgpackc -lwebconfig_framework -lcjson
endif
rdkledmanager_LDADD =  $(rdkledmanager_DEPENDENCIES)
