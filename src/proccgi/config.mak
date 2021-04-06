#
#  This file is part of yi-hack-v5 (https://github.com/TheCrypt0/yi-hack-v5).
#  Copyright (c) 2018-2019 Davide Maggioni.
# 
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, version 3.
# 
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <http://www.gnu.org/licenses/>.
#

CC=arm-hisiv300-linux-uclibcgnueabi-gcc
USER_CFLAGS=-march=armv5te -mcpu=arm926ej-s -I/opt/arm-hisiv300-linux/target/usr/include -L/opt/arm-hisiv300-linux/target/usr/lib
USER_LDFLAGS=
AR=arm-hisiv300-linux-uclibcgnueabi-ar
RANLIB=arm-hisiv300-linux-uclibcgnueabi-ranlib
STRIP=arm-hisiv300-linux-uclibcgnueabi-strip
