#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "fmt-conversion.h"
#include "mp_core.h"
#include "mp_msg.h"
#include "help_mp.h"
#include "video_out.h"
#include "video_out_internal.h"
#include "subopt-helper.h"
#include "libavcodec/avcodec.h"

#define BUFLENGTH 512

static const vo_info_t info =
{
	"ZPUino RGB panel stream",
	"zpuino",
	"Alvaro Lopes <alvieboy@alvie.com>",
	""
};

const LIBVO_EXTERN(zpuino)

static int framenum = 0;
static char *zpuino_dev;

static int
config(uint32_t width, uint32_t height, uint32_t d_width, uint32_t d_height, uint32_t flags, char *title, uint32_t format)
{
    if (width!=96 || height!=64)
        return -1;
    return 0;
}


static uint32_t draw_image(mp_image_t* mpi){

    AVFrame pic;
    int buffersize;
    int res;
    char buf[100];
    FILE *outfile;

    // if -dr or -slices then do nothing:
    if(mpi->flags&(MP_IMGFLAG_DIRECT|MP_IMGFLAG_DRAW_CALLBACK)) return VO_TRUE;

    printf("\nDraw frame: %d (%d x %d)\n", framenum, mpi->w, mpi->h);
    framenum++;

    /*
        mpi->w;
        mpi->h;
        mpi->planes[0];
        mpi->stride[0];
        mpi->w * mpi->h * 8;
    if (outbuffer_size < buffersize) {
        av_freep(&outbuffer);
        outbuffer = av_malloc(buffersize);
        outbuffer_size = buffersize;
    }
    res = avcodec_encode_video(avctx, outbuffer, outbuffer_size, &pic);

    if(res < 0){
 	    mp_msg(MSGT_VO,MSGL_WARN, MSGTR_LIBVO_PNG_ErrorInCreatePng);
            fclose(outfile);
	    return 1;
    }

    fwrite(outbuffer, res, 1, outfile);
    fclose(outfile);
    */
    return VO_TRUE;
}

static void draw_osd(void){}

static void flip_page (void){}

static int draw_frame(uint8_t * src[])
{
    return -1;
}

static int draw_slice( uint8_t *src[],int stride[],int w,int h,int x,int y )
{
    return -1;
}

static int
query_format(uint32_t format)
{
    switch(format){
    case IMGFMT_RGB24:
        return VFCAP_CSP_SUPPORTED|VFCAP_CSP_SUPPORTED_BY_HW|VFCAP_ACCEPT_STRIDE;
    }
    return 0;
}

static void uninit(void){
}

static void check_events(void){}

static int int_zero_to_nine(void *value)
{
    int *sh = value;
    return *sh >= 0 && *sh <= 9;
}

static const opt_t subopts[] = {
    {"dev",      OPT_ARG_MSTRZ,  &zpuino_dev,           NULL},
    {NULL}
};

static int preinit(const char *arg)
{
    if (subopt_parse(arg, subopts) != 0) {
        return -1;
    }
//    avcodec_register_all();
    return 0;
}

static int control(uint32_t request, void *data)
{
  switch (request) {
  case VOCTRL_DRAW_IMAGE:
    return draw_image(data);
  case VOCTRL_QUERY_FORMAT:
    return query_format(*((uint32_t*)data));
  }
  return VO_NOTIMPL;
}
