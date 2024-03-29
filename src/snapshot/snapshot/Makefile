OBJECTS = imggrabber.o convert2jpg.o add_water.o water_mark.o cap_pic.o
FFMPEG = ffmpeg-6.0
FFMPEG_DIR = ./$(FFMPEG)
INC_FF = -I$(FFMPEG_DIR)
LIB_FF = $(FFMPEG_DIR)/libavcodec/libavcodec.a $(FFMPEG_DIR)/libavutil/libavutil.a -lpthread -lm -lrt
JPEGLIB = jpeg-9e
JPEGLIB_DIR = ./$(JPEGLIB)
JPEGSRC = jpegsrc.v9e
INC_J = -I$(JPEGLIB_DIR)
LIB_J = $(JPEGLIB_DIR)/.libs/libjpeg.a

all: imggrabber

framefinder.o: framefinder.c $(HEADERS)
	$(CC) -c $< -fPIC -O2 -o $@

imggrabber.o: imggrabber.c $(HEADERS)
	@$(build_ffmpeg)
	@$(build_jpeglib)
	$(CC) -c $< $(INC_J) $(INC_FF) -fPIC -O2 -o $@

convert2jpg.o: convert2jpg.c $(HEADERS)
	$(CC) -c $< $(INC_J) -fPIC -O2 -o $@

add_water.o: add_water.c $(HEADERS)
	$(CC) -c $< $(INC_J) -fPIC -O2 -o $@

water_mark.o: water_mark.c $(HEADERS)
	$(CC) -c $< $(INC_J) -fPIC -O2 -o $@

cap_pic.o: cap_pic.c $(HEADERS)
	$(CC) -c $< $(INC_J) -fPIC -O2 -o $@

imggrabber: $(OBJECTS)
	$(CC) $(OBJECTS) $(LIB_J) $(LIB_FF) -fPIC -O2 -o $@
	$(STRIP) $@

.PHONY: clean

clean:
	rm -f framefinder imggrabber
	rm -f $(OBJECTS)

distclean: clean
	rm -rf SDK
	rm -rf $(JPEGLIB)

define build_ffmpeg
    # get archive
    if [ ! -f SDK/ffmpeg.tar.bz2 ]; then \
        mkdir -p SDK; \
        wget -O ./SDK/ffmpeg.tar.bz2.tmp "http://ffmpeg.org/releases/$(FFMPEG).tar.bz2"; \
        mv ./SDK/ffmpeg.tar.bz2.tmp ./SDK/ffmpeg.tar.bz2; \
    fi

    # untar
    if [ ! -f $(FFMPEG)/README.md ]; then \
         tar jxvf ./SDK/ffmpeg.tar.bz2; \
    fi

   # build
    if [ ! -f $(FFMPEG)/libavcodec/libavcodec.a ] || [ ! -f $(FFMPEG)/libavutil/libavutil.a ]; then \
         cd $(FFMPEG); \
        ./configure --enable-cross-compile --cross-prefix=$(CROSSPREFIX) --arch=armel --target-os=linux --prefix=$(CROSSPATH) --disable-ffplay --disable-ffprobe --disable-doc  --disable-decoders --enable-decoder=h264 --disable-encoders --disable-demuxers --enable-demuxer=h264 --disable-muxers --disable-protocols --disable-parsers --enable-parser=h264 --disable-filters --disable-bsfs --disable-indevs --disable-outdevs && \
         make; \
         cd ..;\
    fi
endef

define build_jpeglib

    # get archive
    if [ ! -f SDK/jpeg.tar.gz ]; then \
        mkdir -p SDK; \
        wget -O ./SDK/jpeg.tar.gz.tmp "https://www.ijg.org/files/$(JPEGSRC).tar.gz"; \
        mv ./SDK/jpeg.tar.gz.tmp ./SDK/jpeg.tar.gz; \
    fi

    # untar
    if [ ! -f $(JPEGLIB)/README ]; then \
         tar zxvf ./SDK/jpeg.tar.gz; \
    fi

   # build
    if [ ! -f $(JPEGLIB)/.libs/libjpeg.a ]; then \
         cd $(JPEGLIB); \
        ./configure --host=$(CROSS) --enable-shared=yes && \
         make; \
         cd ..;\
    fi
endef
