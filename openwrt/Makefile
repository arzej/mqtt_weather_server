#
# Copyright (C) 2014 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=mqttws

PKG_VERSION=1
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/arzej/mqtt_weather_server.git
PKG_SOURCE_VERSION:=cf1b06c5f734d864384956dd03282d666236c96b

PKG_SOURCE_SUBDIR:=$(PKG_NAME)-$(PKG_VERSION)
PKG_SOURCE:=$(PKG_NAME)-$(PKG_VERSION).tar.gz

PKG_INSTALL:=1
CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/mqtt
	SECTION:=net
	CATEGORY:=Network
	TITLE:=mqttws
	DEPENDS:=+glib2 +librt +libpthread +libstdcpp +libsoup +glib2 +libmosquittopp +libmosquitto
endef

define Package/mqttws/description
        mqtt
endef

define Package/mqttws/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/mqttws $(1)/usr/bin/mqttws
endef

$(eval $(call BuildPackage,mqttws))
