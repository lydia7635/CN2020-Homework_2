/* 

How to build :
$ export LD_LIBRARY_PATH=ffmpeg-lib/lib
$ gcc -I ffmpeg-lib/inc ffmpeg.c -L ffmpeg-lib/lib -lavdevice -lavfilter -lavformat -lavcodec -lswresample -lswscale -lavutil -o ffmpeg.out

Usage:
$ ./ffmpeg.out /your/path/to/tmp.mpg

*/

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// print out the steps and errors
static void logging(const char *fmt, ...);
// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame);
// save a frame into a .pgm file
static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename);

int main(int argc, const char *argv[])
{
  if (argc < 2)
  {
    printf("You need to specify a media file.\n");
    return -1;
  }
  logging("initializing all the containers, codecs and protocols.");
  av_register_all();

  // Preparing AVFormatContext
  // AVFormatContext holds the header information from video
  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (!pFormatContext)
  {
    logging("ERROR could not allocate memory for Format Context");
    return -1;
  }
  logging("opening the input file (%s) and loading format (container) header", argv[1]);

  // Open the file and read its header
  if (avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0)
  {
    logging("ERROR could not open the file");
    return -1;
  }

  // Check stream info is properly
  if (avformat_find_stream_info(pFormatContext, NULL) < 0)
  {
    logging("ERROR could not get the stream info");
    return -1;
  }

  // It's the codec, the component that knows how to encode and decode the stream
  AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters = NULL;

  // Loop though all the streams and print its main information
  // We want to fine video stream Codec and Parameters!!
  for (int i = 0; i < pFormatContext->nb_streams; i++)
  {
    AVCodecParameters *pLocalCodecParameters = NULL;

    // Find Parameters
    pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

    // Finds Codec from a codec ID
    logging("finding the proper decoder (CODEC)");
    AVCodec *pLocalCodec = NULL;
    pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
    if (pLocalCodec == NULL)
    {
      logging("ERROR unsupported codec!");
      return -1;
    }

    // When the stream is a audio
    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      // You may not care about audio stream.
    }
    // When the stream is a video
    else if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      // We fine video stream Codec and Parameters!!
      pCodec = pLocalCodec;
      pCodecParameters = pLocalCodecParameters;
      logging("Video Codec: resolution %d x %d", pLocalCodecParameters->width, pLocalCodecParameters->height);
    }
    // Print its name, id and bitrate
    logging("\tCodec %s ID %d bit_rate %lld", pLocalCodec->name, pLocalCodec->id, pLocalCodecParameters->bit_rate);
  }

  // In Server, fill AVCodecContext with Codec and Parameters (THE FIRST COMPONENT you should use Socket to transmit)
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  if (!pCodecContext)
  {
    logging("failed to allocated memory for AVCodecContext");
    return -1;
  }
  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
  {
    logging("failed to copy codec params to codec context");
    return -1;
  }
  if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
  {
    logging("failed to open codec through avcodec_open2");
    return -1;
  }

  // In Server, prepare AVPacket (THE SECOND COMPONENT you should use Socket to transmit)
  AVPacket *pPacket = av_packet_alloc();
  if (!pPacket)
  {
    logging("failed to allocated memory for AVPacket");
    return -1;
  }

  // In client, Prepare AVFrame
  AVFrame *pFrame = av_frame_alloc();
  if (!pFrame)
  {
    logging("failed to allocated memory for AVFrame");
    return -1;
  }

  int response = 0;
  int how_many_packets_to_process = 10;
  // Fill the AVPacket (pPacket) with data from the Stream
  while (av_read_frame(pFormatContext, pPacket) >= 0)
  {
    // So far, We have filled with two component, AVPacket (pPacket) and AVCodecContext (pCodecContext)
    // You should use socket to transmis these two component to client.

    /*
    Assume you have already transmitted ~~~~~~~~~~~~
    */

    // In clinet, we get the AVPacket and AVCodecContext, and we have prepared the AVFrame
    // So we have "AVPacket", "AVCodecContext", "AVFrame"
    // Use function "decode_packet" decode the packet to the frame
    response = decode_packet(pPacket, pCodecContext, pFrame);
    if (response < 0)
      break;
    // stop it, otherwise we'll be saving hundreds of frames
    if (--how_many_packets_to_process <= 0)
      break;

    av_packet_unref(pPacket);
  }

  logging("releasing all the resources");

  avformat_close_input(&pFormatContext);
  av_packet_free(&pPacket);
  av_frame_free(&pFrame);
  avcodec_free_context(&pCodecContext);
  return 0;
}

static void logging(const char *fmt, ...)
{
  va_list args;
  fprintf(stderr, "LOG: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame)
{
  // Supply raw packet data as input to a decoder
  int response = avcodec_send_packet(pCodecContext, pPacket);

  if (response < 0)
  {
    logging("Error while sending a packet to the decoder: %s", av_err2str(response));
    return response;
  }

  while (response >= 0)
  {
    // Return decoded output data (into a frame) from a decoder
    response = avcodec_receive_frame(pCodecContext, pFrame);
    if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
    {
      break;
    }
    else if (response < 0)
    {
      logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
      return response;
    }

    if (response >= 0)
    {
      logging(
          "Frame %d (type=%c, size=%d bytes) pts %d key_frame %d [DTS %d]",
          pCodecContext->frame_number,
          av_get_picture_type_char(pFrame->pict_type),
          pFrame->pkt_size,
          pFrame->pts,
          pFrame->key_frame,
          pFrame->coded_picture_number);

      char frame_filename[1024];
      snprintf(frame_filename, sizeof(frame_filename), "%s-%d.pgm", "frame", pCodecContext->frame_number);
      // save a grayscale frame into a .pgm file
      save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);
    }
  }
  return 0;
}

static void save_gray_frame(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
  FILE *f;
  int i;
  f = fopen(filename, "w");
  // writing the minimal required header for a pgm file format
  fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

  // writing line by line
  for (i = 0; i < ysize; i++)
    fwrite(buf + i * wrap, 1, xsize, f);
  fclose(f);
}