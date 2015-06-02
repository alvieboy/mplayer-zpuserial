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
#include "zpuino/sysdeps.h"
#include "zpuino/hdlc.h"

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
static char *zpuino_dev = NULL; //"/dev/ttyUSB1";
connection_t conn;

static int
config(uint32_t width, uint32_t height, uint32_t d_width, uint32_t d_height, uint32_t flags, char *title, uint32_t format)
{
    if (width!=96 || height!=64)
        return -1;
    return 0;
}

void buffer_free(buffer_t *b)
{
	if (b) {
		if (b->buf)
			free(b->buf);
		free(b);
	}
}

static unsigned char txbuf[65535];


static int sendframe(connection_t fd, const unsigned char *buffer, size_t size)
{
	unsigned char *txptr = &txbuf[0];

	size_t i;

        *txptr++=HDLC_frameFlag;
        *txptr++=0x01;
//        writeEscaped(buffer[i],&txptr);

	for (i=0;i<size;i++) {
		writeEscaped(buffer[i],&txptr);
	}
	*txptr++= HDLC_frameFlag;

	if(0) {
		struct timeval tv;
		gettimeofday(&tv,NULL);
		printf("[%d.%06d] Tx:",tv.tv_sec,tv.tv_usec
			  );
		for (i=0; i<txptr-(&txbuf[0]); i++) {
			printf(" 0x%02x", txbuf[i]);
		}
		printf("\n");
        }

        int wsize=txptr-(&txbuf[0]);
        txptr = &txbuf[0];
        do {
            int csize=conn_write(fd, txptr, wsize);
            if (csize<0) {
                if (errno!=EAGAIN) {
                    printf("Short write.... %d != %d\n",csize,wsize);
                }
            } else {
                wsize-=csize;
                txptr+=csize;
            }
        } while (wsize);
}


static uint32_t draw_image(mp_image_t* mpi){

    AVFrame pic;
    int buffersize;
    int res;
    FILE *outfile;

    static char *outbuf=NULL;
    char *optr;

    // if -dr or -slices then do nothing:
    if(mpi->flags&(MP_IMGFLAG_DIRECT|MP_IMGFLAG_DRAW_CALLBACK)) return VO_TRUE;

    if ((mpi->w != 96) || (mpi->h != 64)) {

        return VO_NOTIMPL;
    }

    framenum++;

    if (outbuf==NULL) {
        outbuf = av_malloc(96*64*3);
    }

    optr = outbuf;

    int x;
    int stride=mpi->stride[0];
    uint8_t *plane =mpi->planes[0];

    for (x=0;x<mpi->h;x++) {

        //memcpy(opt, sendframe(conn, dbuf, 96*64*3);

        memcpy(optr, plane + (x*stride), mpi->w * 3);
        optr+=(mpi->w*3);
    }

    sendframe(conn, outbuf, 96*64*3);

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

static char *stripaddress=NULL;
static char *strport=NULL;
static int port=8193;
static struct sockaddr_in sock;

static int preinit(const char *arg)
{
    char tempip[32];
    int iplen;
    if (subopt_parse(arg, subopts) != 0) {
        return -1;
    }
    if (zpuino_dev==NULL) {
        return -1;
    }
    fprintf(stderr,"Device: %s\n",zpuino_dev);

    if (strncmp(zpuino_dev,"tcp-",4)==0) {
        zpuino_dev+=4;
        strport=strchr(zpuino_dev,'-');
        if (strport) {
            iplen = strport-zpuino_dev;
            strport++;
            port=atoi(strport);
            strncpy(tempip, zpuino_dev, iplen);
            tempip[iplen]='\0';
        } else {
            strncpy(tempip, zpuino_dev, sizeof(tempip));
        }
        sock.sin_family = AF_INET;
        sock.sin_port = htons(port);
        sock.sin_addr.s_addr = inet_addr(tempip);
        conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (conn<0) {
            perror("socket");
            return -1;
        }
        if (connect(conn, (struct sockaddr*)&sock, sizeof(sock))<0) {
            perror("connect");
            return -1;
        }
        //fprintf(stderr,"IP address: '%s', port %d\n", tempip, port);
    } else {
        if (conn_open(zpuino_dev, B3000000, &conn)<0) {
            return -1;
        }
    }


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
