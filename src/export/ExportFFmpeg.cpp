/**********************************************************************

   Audacity: A Digital Audio Editor

   ExportFFmpeg.cpp

   Audacity(R) is copyright (c) 1999-2009 Audacity Team.
   License: GPL v2.  See License.txt.

   LRN

******************************************************************//**

\class ExportFFmpeg
\brief Controlling class for FFmpeg exporting.  Creates the options
dialog of the appropriate type, adds tags and invokes the export
function.

*//*******************************************************************/


#include "../Audacity.h"   // keep ffmpeg before wx because they interact // for USE_* macros

#include "../FFmpeg.h"     // and Audacity.h before FFmpeg for config*.h

#include <wx/choice.h>
#include <wx/intl.h>
#include <wx/timer.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/window.h>
#include <wx/spinctrl.h>
#include <wx/combobox.h>

#include "../FileFormats.h"
#include "../Mix.h"
#include "../ProjectSettings.h"
#include "../Tags.h"
#include "../Track.h"
#include "../widgets/AudacityMessageBox.h"
#include "../widgets/ProgressDialog.h"
#include "../wxFileNameWrapper.h"

#include "Export.h"

#include "ExportFFmpegDialogs.h"

#if defined(WIN32) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#if defined(USE_FFMPEG)

// Define this to automatically resample audio to the nearest supported sample rate
#define FFMPEG_AUTO_RESAMPLE 1

namespace {

bool CheckFFmpegPresence(bool quiet = false)
{
   bool result = true;
   PickFFmpegLibs();
   if (!FFmpegLibsInst()->ValidLibsLoaded())
   {
      if (!quiet)
      {
         AudacityMessageBox(XO(
            "Properly configured FFmpeg is required to proceed.\nYou can configure it at Preferences > Libraries."));
      }
      result = false;
   }
   DropFFmpegLibs();
   return result;
}

int AdjustFormatIndex(int format)
{
   int subFormat = -1;
   for (int i = 0; i <= FMT_OTHER; i++)
   {
      if (ExportFFmpegOptions::fmts[i].compiledIn) subFormat++;
      if (subFormat == format || i == FMT_OTHER)
      {
         subFormat = i;
         break;
      }
   }
   return subFormat;
}

//----------------------------------------------------------------------------
// ExportFFmpeg
//----------------------------------------------------------------------------

class ExportFFmpeg final : public ExportPlugin
{
public:

   ExportFFmpeg();
   ~ExportFFmpeg() override;

   /// Callback, called from GetFilename
   bool CheckFileName(wxFileName &filename, int format = 0) override;

   /// Format initialization
   bool Init(const char *shortname, AudacityProject *project, const Tags *metadata, int subformat);

   /// Codec initialization
   bool InitCodecs(AudacityProject *project);

   /// Writes metadata
   bool AddTags(const Tags *metadata);

   /// Sets individual metadata values
   void SetMetadata(const Tags *tags, const char *name, const wxChar *tag);

   /// Encodes audio
   bool EncodeAudioFrame(int16_t *pFrame, size_t frameSize);

   /// Flushes audio encoder
   bool Finalize();

   void FreeResources();

   /// Creates options panel
   ///\param format - index of export type
   void OptionsCreate(ShuttleGui &S, int format) override;

   /// Check whether or not current project sample rate is compatible with the export codec
   bool CheckSampleRate(int rate, int lowrate, int highrate, const int *sampRates);

   /// Asks user to resample the project or cancel the export procedure
   int  AskResample(int bitrate, int rate, int lowrate, int highrate, const int *sampRates);

   /// Exports audio
   ///\param project Audacity project
   ///\param fName output file name
   ///\param selectedOnly true if exporting only selected audio
   ///\param t0 audio start time
   ///\param t1 audio end time
   ///\param mixerSpec mixer
   ///\param metadata tags to write into file
   ///\param subformat index of export type
   ///\return true if export succeeded
   ProgressResult Export(AudacityProject *project,
      std::unique_ptr<ProgressDialog> &pDialog,
      unsigned channels,
      const wxFileNameWrapper &fName,
      bool selectedOnly,
      double t0,
      double t1,
      MixerSpec *mixerSpec = nullptr,
      const Tags *metadata = nullptr,
      int subformat = 0) override;

private:

   AVOutputFormat  *   mEncFormatDesc{};       // describes our output file to libavformat
   int               default_frame_size{};
   AVStream        *   mEncAudioStream{};      // the output audio stream (may remain NULL)
   int               mEncAudioFifoOutBufSiz{};

   wxFileNameWrapper mName;

   int               mSubFormat{};
   int               mBitRate{};
   int               mSampleRate{};
   unsigned          mChannels{};
   bool              mSupportsUTF8{};

   int64_t           mSamplesWritten;

   // Smart pointer fields, their order is the reverse in which they are reset in FreeResources():
   AVFifoBufferHolder   mEncAudioFifo;          // FIFO to write incoming audio samples into
   AVMallocHolder<int16_t> mEncAudioFifoOutBuf;  // buffer to read _out_ of the FIFO into
   AVFormatContextHolder mEncFormatCtx;        // libavformat's context for our output file
   UFileHolder          mUfileCloser;
   AVCodecContextHolder mEncAudioCodecCtx;    // the encoder for the output audio stream
};

ExportFFmpeg::ExportFFmpeg()
:  ExportPlugin()
{
   mEncFormatDesc = nullptr;      // describes our output file to libavformat
   mEncAudioStream = nullptr;     // the output audio stream (may remain NULL)
   #define MAX_AUDIO_PACKET_SIZE (128 * 1024)
   mEncAudioFifoOutBufSiz = 0;

   mSampleRate = 0;
   mSupportsUTF8 = true;

   PickFFmpegLibs(); // DropFFmpegLibs() call is in ExportFFmpeg destructor
   int avfver = FFmpegLibsInst()->ValidLibsLoaded() ? avformat_version() : 0;
   int newfmt;
   // Adds export types from the export type list
   for (newfmt = 0; newfmt < FMT_LAST; newfmt++)
   {
      wxString shortname(ExportFFmpegOptions::fmts[newfmt].shortname);
      //Don't hide export types when there's no av-libs, and don't hide FMT_OTHER
      if (newfmt < FMT_OTHER && FFmpegLibsInst()->ValidLibsLoaded())
      {
         // Format/Codec support is compiled in?
         AVOutputFormat *avoformat = av_guess_format(shortname.mb_str(), nullptr, nullptr);
         AVCodec *avcodec = avcodec_find_encoder(ExportFFmpegOptions::fmts[newfmt].codecid);
         if (avoformat == nullptr || avcodec == nullptr)
         {
            ExportFFmpegOptions::fmts[newfmt].compiledIn = false;
            continue;
         }
      }
      int fmtindex = AddFormat() - 1;
      SetFormat(ExportFFmpegOptions::fmts[newfmt].name,fmtindex);
      AddExtension(ExportFFmpegOptions::fmts[newfmt].extension,fmtindex);
      // For some types add other extensions
      switch(newfmt)
      {
      case FMT_M4A:
         AddExtension(wxT("3gp"),fmtindex);
         AddExtension(wxT("m4r"),fmtindex);
         AddExtension(wxT("mp4"),fmtindex);
         break;
      case FMT_WMA2:
         AddExtension(wxT("asf"),fmtindex);
         AddExtension(wxT("wmv"),fmtindex);
         break;
      default:
         break;
      }

      SetMaxChannels(ExportFFmpegOptions::fmts[newfmt].maxchannels,fmtindex);
      SetDescription(ExportFFmpegOptions::fmts[newfmt].description, fmtindex);

      int canmeta = ExportFFmpegOptions::fmts[newfmt].canmetadata;
      if (canmeta && (canmeta == AV_CANMETA || canmeta <= avfver))
      {
         SetCanMetaData(true,fmtindex);
      }
      else
      {
         SetCanMetaData(false,fmtindex);
      }
   }
}

ExportFFmpeg::~ExportFFmpeg()
{
   DropFFmpegLibs();
}

bool ExportFFmpeg::CheckFileName(wxFileName & WXUNUSED(filename), int WXUNUSED(format))
{
   bool result = true;

   // Show "Locate FFmpeg" dialog
   if (!CheckFFmpegPresence(true))
   {
      FFmpegLibsInst()->FindLibs(nullptr);
      FFmpegLibsInst()->FreeLibs();
      return LoadFFmpeg(true);
   }

   return result;
}

bool ExportFFmpeg::Init(const char *shortname, AudacityProject *project, const Tags *metadata, int subformat)
{
   // This will undo the acquisition of resources along any early exit path:
   auto deleter = [](ExportFFmpeg *This) {
      if (This)
         This->FreeResources();
   };
   std::unique_ptr<ExportFFmpeg, decltype(deleter)> cleanup{ this, deleter };

   int err;
   //FFmpegLibsInst()->LoadLibs(NULL,true); //Loaded at startup or from Prefs now

   if (!FFmpegLibsInst()->ValidLibsLoaded())
      return false;

   av_log_set_callback(av_log_wx_callback);

   // See if libavformat has modules that can write our output format. If so, mEncFormatDesc
   // will describe the functions used to write the format (used internally by libavformat)
   // and the default video/audio codecs that the format uses.
   const auto path = mName.GetFullPath();
   if ((mEncFormatDesc = av_guess_format(shortname, OSINPUT(path), nullptr)) == nullptr)
   {
      AudacityMessageBox(
         XO(
"FFmpeg : ERROR - Can't determine format description for file \"%s\".")
            .Format( path ),
         XO("FFmpeg Error"),
         wxOK|wxCENTER|wxICON_EXCLAMATION );
      return false;
   }

   // mEncFormatCtx is used by libavformat to carry around context data re our output file.
   mEncFormatCtx.reset(avformat_alloc_context());
   if (!mEncFormatCtx)
   {
      AudacityMessageBox(
         XO("FFmpeg : ERROR - Can't allocate output format context."),
         XO("FFmpeg Error"),
         wxOK|wxCENTER|wxICON_EXCLAMATION);
      return false;
   }

   // Initialise the output format context.
   mEncFormatCtx->oformat = mEncFormatDesc;

   if (mEncFormatCtx->url) av_free(mEncFormatCtx->url);

   const auto path_size = strlen(OSINPUT(path)) + 1;
   mEncFormatCtx->url = static_cast<char *>(av_malloc(path_size));
   memcpy(mEncFormatCtx->url, OSINPUT(path), path_size);

   // Open the output file.
   if (!(mEncFormatDesc->flags & AVFMT_NOFILE))
   {
      if ((err = ufile_fopen(&mEncFormatCtx->pb, path, AVIO_FLAG_WRITE)) < 0)
      {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Can't open output file \"%s\" to write. Error code is %d.")
               .Format( path, err ),
            XO("FFmpeg Error"),
            wxOK|wxCENTER|wxICON_EXCLAMATION);
         return false;
      }
      // Give mUfileCloser responsibility
      mUfileCloser.reset(mEncFormatCtx->pb);
   }

   // Open the audio stream's codec and initialise any stream related data.
   if (!InitCodecs(project))
      return false;

   // At the moment Audacity can export only one audio stream
   if ((mEncAudioStream = avformat_new_stream(mEncFormatCtx.get(), mEncAudioCodecCtx->codec)) == nullptr)
   {
      AudacityMessageBox(
         XO("FFmpeg : ERROR - Can't add audio stream to output file \"%s\".")
         .Format(path),
         XO("FFmpeg Error"),
         wxOK | wxCENTER | wxICON_EXCLAMATION);
      return false;
   }


   // Documentation for avformat_new_stream says
   // "User is required to call avcodec_close() and avformat_free_context() to clean
   // up the allocation by avformat_new_stream()."

   // We use smart pointers that ensure these cleanups either in their destructors or
   // sooner if they are reset.  These are std::unique_ptr with nondefault deleter
   // template parameters.

   // mEncFormatCtx takes care of avformat_free_context(), so
   // mEncAudioStream can be a plain pointer.

   mEncAudioStream->id = mEncFormatCtx->nb_streams - 1;

   mEncAudioStream->time_base = mEncAudioCodecCtx->time_base;

   { // Scope
   /* copy the stream parameters to the muxer */
      auto rc = avcodec_parameters_from_context(mEncAudioStream->codecpar, mEncAudioCodecCtx.get());
      if (rc < 0) {
         AudacityMessageBox(
            XO("FFmpeg : Could not copy the stream parameters."),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
      }
   }

   if (metadata == nullptr)
      metadata = &Tags::Get( *project );

   // Add metadata BEFORE writing the header.
   // At the moment that works with ffmpeg-git and ffmpeg-0.5 for MP4.
   if (GetCanMetaData(subformat))
   {
      mSupportsUTF8 = ExportFFmpegOptions::fmts[mSubFormat].canutf8;
      AddTags(metadata);
   }

   // Write headers to the output file.
   if ((err = avformat_write_header(mEncFormatCtx.get(), nullptr)) < 0)
   {
      AudacityMessageBox(
         XO("FFmpeg : ERROR - Can't write headers to output file \"%s\". Error code is %d.")
            .Format( path, err ),
         XO("FFmpeg Error"),
         wxOK|wxCENTER|wxICON_EXCLAMATION);
      return false;
   }

   // Only now, we can keep all the resources until after Finalize().
   // Cancel the local cleanup.
   (void)cleanup.release();

   return true;
}

bool ExportFFmpeg::CheckSampleRate(int rate, int lowrate, int highrate, const int *sampRates)
{
   if (lowrate && highrate)
   {
      if (rate < lowrate || rate > highrate)
      {
         return false;
      }
   }

   if (sampRates)
   {
      for (int i = 0; sampRates[i] > 0; i++)
      {
         if (rate == sampRates[i])
         {
            return true;
         }
      }
   }

   return false;
}

int set_dict_int(AVDictionary** dict, const char* key, int val)
{
   char val_str[256];
   snprintf(val_str, sizeof(val_str), "%d", val);
   return av_dict_set(dict, key, val_str, 0);
}

bool ExportFFmpeg::InitCodecs(AudacityProject *project)
{
   const auto &settings = ProjectSettings::Get( *project );
   AVCodec *codec = nullptr;
   AVDictionary *options = nullptr;
   AVDictionaryCleanup cleanup{ &options };

   // Get the sample rate from the passed settings if we haven't set it before.
   // Doing this only when not set allows us to carry the sample rate from one
   // iteration of ExportMultiple to the next.  This prevents multiple resampling
   // dialogs in the event the codec can't support the specified rate.
   if (!mSampleRate)
   {
      mSampleRate = static_cast<int>(settings.GetRate());
   }

   if (mSubFormat == FMT_OTHER)
   {
      codec = avcodec_find_encoder_by_name(gPrefs->Read(wxT("/FileFormats/FFmpegCodec")).ToUTF8());
   
      if (codec && codec->id == AV_CODEC_ID_AC3)
      {
         // As of Jan 4, 2011, the default AC3 encoder only accept SAMPLE_FMT_FLT samples.
         // But, currently, Audacity only supports SAMPLE_FMT_S16.  So, for now, look for the
         // "older" AC3 codec.  this is not a proper solution, but will suffice until other
         // encoders no longer support SAMPLE_FMT_S16.
         codec = avcodec_find_encoder_by_name("ac3_fixed");
      }
   }

   if (!codec)
      codec = avcodec_find_encoder(ExportFFmpegOptions::fmts[mSubFormat].codecid);

   // Is the required audio codec compiled into libavcodec?
   if (codec == nullptr)
   {
      AudacityMessageBox(
         XO(
            /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
            "FFmpeg cannot find audio codec 0x%x.\nSupport for this codec is probably not compiled in.")
         .Format(static_cast<unsigned int>(mEncAudioCodecCtx->codec_id)),
         XO("FFmpeg Error"),
         wxOK | wxCENTER | wxICON_EXCLAMATION);
      return false;
   }

   // Configure the audio stream's codec context.

   mEncAudioCodecCtx.reset(avcodec_alloc_context3(codec));

   mEncAudioCodecCtx->global_quality = -99999; //quality mode is off by default;

   // Each export type has its own settings
   switch (mSubFormat)
   {
   case FMT_M4A:
   {
      int q = gPrefs->Read(wxT("/FileFormats/AACQuality"),-99999);
      mEncAudioCodecCtx->global_quality = q;
      q = wxClip( q, 98 * mChannels, 160 * mChannels);
      // Set bit rate to between 98 kbps and 320 kbps (if two channels)
      mEncAudioCodecCtx->bit_rate = q * 1000;
      mEncAudioCodecCtx->profile = FF_PROFILE_AAC_LOW;
      mEncAudioCodecCtx->cutoff = 0;
      break;
   }
   case FMT_AC3:
      mEncAudioCodecCtx->bit_rate = gPrefs->Read(wxT("/FileFormats/AC3BitRate"), 192000);
      if (!CheckSampleRate(mSampleRate,ExportFFmpegAC3Options::iAC3SampleRates[0], ExportFFmpegAC3Options::iAC3SampleRates[2], &ExportFFmpegAC3Options::iAC3SampleRates[0]))
         mSampleRate = AskResample(mEncAudioCodecCtx->bit_rate,mSampleRate, ExportFFmpegAC3Options::iAC3SampleRates[0], ExportFFmpegAC3Options::iAC3SampleRates[2], &ExportFFmpegAC3Options::iAC3SampleRates[0]);
      break;
   case FMT_AMRNB:
      mSampleRate = 8000;
      mEncAudioCodecCtx->bit_rate = gPrefs->Read(wxT("/FileFormats/AMRNBBitRate"), 12200);
      break;
   case FMT_OPUS:
      av_dict_set(&options, "b", gPrefs->Read(wxT("/FileFormats/OPUSBitRate"), wxT("128000")).ToUTF8(), 0);
      av_dict_set(&options, "vbr", gPrefs->Read(wxT("/FileFormats/OPUSVbrMode"), wxT("on")).ToUTF8(), 0);
      av_dict_set(&options, "compression_level", gPrefs->Read(wxT("/FileFormats/OPUSCompression"), wxT("10")).ToUTF8(), 0);
      av_dict_set(&options, "frame_duration", gPrefs->Read(wxT("/FileFormats/OPUSFrameDuration"), wxT("20")).ToUTF8(), 0);
      av_dict_set(&options, "application", gPrefs->Read(wxT("/FileFormats/OPUSApplication"), wxT("audio")).ToUTF8(), 0);
      av_dict_set(&options, "cutoff", gPrefs->Read(wxT("/FileFormats/OPUSCutoff"), wxT("0")).ToUTF8(), 0);
      av_dict_set(&options, "mapping_family", mChannels <= 2 ? "0" : "255", 0);
      break;
   case FMT_WMA2:
      mEncAudioCodecCtx->bit_rate = gPrefs->Read(wxT("/FileFormats/WMABitRate"), 198000);
      if (!CheckSampleRate(mSampleRate,ExportFFmpegWMAOptions::iWMASampleRates[0], ExportFFmpegWMAOptions::iWMASampleRates[4], &ExportFFmpegWMAOptions::iWMASampleRates[0]))
         mSampleRate = AskResample(mEncAudioCodecCtx->bit_rate,mSampleRate, ExportFFmpegWMAOptions::iWMASampleRates[0], ExportFFmpegWMAOptions::iWMASampleRates[4], &ExportFFmpegWMAOptions::iWMASampleRates[0]);
      break;
   case FMT_OTHER:
      av_dict_set(&mEncAudioStream->metadata, "language", gPrefs->Read(wxT("/FileFormats/FFmpegLanguage"),wxT("")).ToUTF8(), 0);
      mEncAudioCodecCtx->sample_rate = gPrefs->Read(wxT("/FileFormats/FFmpegSampleRate"),(long)0);
      if (mEncAudioCodecCtx->sample_rate != 0) mSampleRate = mEncAudioCodecCtx->sample_rate;
      mEncAudioCodecCtx->bit_rate = gPrefs->Read(wxT("/FileFormats/FFmpegBitRate"), (long)0);
      strncpy((char *)&mEncAudioCodecCtx->codec_tag,gPrefs->Read(wxT("/FileFormats/FFmpegTag"),wxT("")).mb_str(wxConvUTF8),4);
      mEncAudioCodecCtx->global_quality = gPrefs->Read(wxT("/FileFormats/FFmpegQuality"),(long)-99999);
      mEncAudioCodecCtx->cutoff = gPrefs->Read(wxT("/FileFormats/FFmpegCutOff"),(long)0);
      mEncAudioCodecCtx->flags2 = 0;
      if (gPrefs->Read(wxT("/FileFormats/FFmpegBitReservoir"),true))
         av_dict_set(&options, "reservoir", "1", 0);
      if (gPrefs->Read(wxT("/FileFormats/FFmpegVariableBlockLen"),true)) mEncAudioCodecCtx->flags2 |= 0x0004; //WMA only?
      mEncAudioCodecCtx->compression_level = gPrefs->Read(wxT("/FileFormats/FFmpegCompLevel"),-1);
      mEncAudioCodecCtx->frame_size = gPrefs->Read(wxT("/FileFormats/FFmpegFrameSize"),(long)0);

//FIXME The list of supported options for the selected encoder should be extracted instead of a few hardcoded
      set_dict_int(&options, "lpc_coeff_precision",     gPrefs->Read(wxT("/FileFormats/FFmpegLPCCoefPrec"),(long)0));
      set_dict_int(&options, "min_prediction_order",    gPrefs->Read(wxT("/FileFormats/FFmpegMinPredOrder"),(long)-1));
      set_dict_int(&options, "max_prediction_order",    gPrefs->Read(wxT("/FileFormats/FFmpegMaxPredOrder"),(long)-1));
      set_dict_int(&options, "min_partition_order",     gPrefs->Read(wxT("/FileFormats/FFmpegMinPartOrder"),(long)-1));
      set_dict_int(&options, "max_partition_order",     gPrefs->Read(wxT("/FileFormats/FFmpegMaxPartOrder"),(long)-1));
      set_dict_int(&options, "prediction_order_method", gPrefs->Read(wxT("/FileFormats/FFmpegPredOrderMethod"),(long)0));
      set_dict_int(&options, "muxrate",                 gPrefs->Read(wxT("/FileFormats/FFmpegMuxRate"),(long)0));
      mEncFormatCtx->packet_size = gPrefs->Read(wxT("/FileFormats/FFmpegPacketSize"),(long)0);
      if (!codec)
         mEncAudioCodecCtx->codec_id = mEncFormatDesc->audio_codec;
      break;
   default:
      return false;
   }

   // This happens if user refused to resample the project
   if (mSampleRate == 0) return false;

   if (mEncAudioCodecCtx->global_quality >= 0)
   {
      mEncAudioCodecCtx->flags |= AV_CODEC_FLAG_QSCALE;
   }
   else mEncAudioCodecCtx->global_quality = 0;

   mEncAudioCodecCtx->global_quality = mEncAudioCodecCtx->global_quality * FF_QP2LAMBDA;
   mEncAudioCodecCtx->sample_rate = mSampleRate;
   mEncAudioCodecCtx->channels = mChannels;
   mEncAudioCodecCtx->channel_layout = av_get_default_channel_layout(mChannels);
   mEncAudioCodecCtx->time_base.num = 1;
   mEncAudioCodecCtx->time_base.den = mEncAudioCodecCtx->sample_rate;
   mEncAudioCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
   mEncAudioCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

   if (codec->sample_fmts) {
      for (int i=0; codec->sample_fmts[i] != AV_SAMPLE_FMT_NONE; i++) {
         enum AVSampleFormat fmt = codec->sample_fmts[i];
         if (   fmt == AV_SAMPLE_FMT_U8
             || fmt == AV_SAMPLE_FMT_U8P
             || fmt == AV_SAMPLE_FMT_S16
             || fmt == AV_SAMPLE_FMT_S16P
             || fmt == AV_SAMPLE_FMT_S32
             || fmt == AV_SAMPLE_FMT_S32P
             || fmt == AV_SAMPLE_FMT_FLT
             || fmt == AV_SAMPLE_FMT_FLTP) {
            mEncAudioCodecCtx->sample_fmt = fmt;
         }
         if (   fmt == AV_SAMPLE_FMT_S16
             || fmt == AV_SAMPLE_FMT_S16P)
            break;
      }
   }

   if (codec->supported_samplerates)
   {
      // Workaround for crash in bug #2378.  Proper fix is to get a newer version of FFmpeg.
      if (codec->id == AV_CODEC_ID_AAC)
      {
         std::vector<int> rates;
         int i = 0;

         while (codec->supported_samplerates[i] && codec->supported_samplerates[i] != 7350)
         {
            rates.push_back(codec->supported_samplerates[i++]);
         }
         rates.push_back(0);

         if (!CheckSampleRate(mSampleRate, 0, 0, rates.data()))
         {
            mEncAudioCodecCtx->sample_rate = mSampleRate = AskResample(0, mSampleRate, 0, 0, rates.data());
         }
      }
      else
      {
         if (!CheckSampleRate(mSampleRate, 0, 0, codec->supported_samplerates))
         {
            mEncAudioCodecCtx->sample_rate = mSampleRate = AskResample(0, mSampleRate, 0, 0, codec->supported_samplerates);
         }
      }

      // This happens if user refused to resample the project
      if (mSampleRate == 0)
      {
         return false;
      }
   }

   if (mEncFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
   {
      mEncAudioCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
      mEncFormatCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
   }

   // Open the codec.
   int rc = avcodec_open2(mEncAudioCodecCtx.get(), codec, &options);
   if (rc < 0)
   {
      TranslatableString errmsg;

      switch (rc)
      {
      case -EPERM:
         errmsg = XO("The codec reported a generic error (EPERM)");
         break;
      case -EINVAL:
         errmsg = XO("The codec reported an invalid parameter (EINVAL)");
         break;
      default:
         char buf[AV_ERROR_MAX_STRING_SIZE];
         av_strerror(rc, buf, sizeof(buf));
         errmsg = Verbatim(buf);
      }

      AudacityMessageBox(
         /* i18n-hint: "codec" is short for a "coder-decoder" algorithm */
         XO("Can't open audio codec \"%s\" (0x%x)\n\n%s")
         .Format(codec->name, mEncAudioCodecCtx->codec_id, errmsg),
         XO("FFmpeg Error"),
         wxOK|wxCENTER|wxICON_EXCLAMATION);
      return false;
   }

   default_frame_size = mEncAudioCodecCtx->frame_size;
   if (default_frame_size == 0)
      default_frame_size = 1024; // arbitrary non zero value;

   wxLogDebug(wxT("FFmpeg : Audio Output Codec Frame Size: %d samples."), mEncAudioCodecCtx->frame_size);

   // The encoder may require a minimum number of raw audio samples for each encoding but we can't
   // guarantee we'll get this minimum each time an audio frame is decoded from the input file so
   // we use a FIFO to store up incoming raw samples until we have enough for one call to the codec.
   mEncAudioFifo.reset(av_fifo_alloc(1024));

   mEncAudioFifoOutBufSiz = 2*MAX_AUDIO_PACKET_SIZE;
   // Allocate a buffer to read OUT of the FIFO into. The FIFO maintains its own buffer internally.
   mEncAudioFifoOutBuf.reset(static_cast<int16_t*>(av_malloc(mEncAudioFifoOutBufSiz)));
   if (!mEncAudioFifoOutBuf)
   {
      AudacityMessageBox(
         XO("FFmpeg : ERROR - Can't allocate buffer to read into from audio FIFO."),
         XO("FFmpeg Error"),
         wxOK|wxCENTER|wxICON_EXCLAMATION
      );
      return false;
   }

   return true;
}

// Returns 0 if no more output, 1 if more output, negative if error
int encode_audio(AVCodecContext *avctx, AVFormatContext* avfctx, AVStream* audio_stream, AVPacket* pkt, const int16_t *audio_samples, int nb_samples, int64_t pts)
{
   // Assume *pkt is already initialized.

   int buffer_size, ret, got_output = 0;

   if (audio_samples) {
      AVFrameHolder frame{ av_frame_alloc() };
      if (!frame)
         return AVERROR(ENOMEM);

      frame->nb_samples = nb_samples;
      frame->format = avctx->sample_fmt;
      frame->sample_rate = avctx->sample_rate;
#if !defined(DISABLE_DYNAMIC_LOADING_FFMPEG) || (LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(54, 13, 0))
      frame->channel_layout = avctx->channel_layout;
#endif

      frame->pts = pts; // av_rescale_q(xyz, AVRational{ 1, avctx->sample_rate }, avctx->time_base);

      buffer_size = av_samples_get_buffer_size(nullptr, avctx->channels, frame->nb_samples,
         avctx->sample_fmt, 0);
      if (buffer_size < 0) {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Could not get sample buffer size"),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return buffer_size;
      }

      const AVMallocHolder<uint8_t> samples{ static_cast<uint8_t*>(av_malloc(buffer_size)) };
      if (!samples) {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Could not allocate bytes for samples buffer"),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return AVERROR(ENOMEM);
      }
      /* setup the data pointers in the AVFrame */
      ret = avcodec_fill_audio_frame(frame.get(), avctx->channels, avctx->sample_fmt,
         samples.get(), buffer_size, 0);

      if (ret < 0) {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Could not setup audio frame"),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return ret;
      }

      for (auto ch = 0; ch < avctx->channels; ch++) {
         for (auto i = 0; i < frame->nb_samples; i++) {
            switch (avctx->sample_fmt) {
            case AV_SAMPLE_FMT_U8:
               static_cast<uint8_t*>(frame->data[0])[ch + i * avctx->channels] = audio_samples[ch + i * avctx->channels] / 258 + 128;
               break;
            case AV_SAMPLE_FMT_U8P:
               static_cast<uint8_t*>(frame->data[ch])[i] = audio_samples[ch + i * avctx->channels] / 258 + 128;
               break;
            case AV_SAMPLE_FMT_S16:
               reinterpret_cast<int16_t*>(frame->data[0])[ch + i * avctx->channels] = audio_samples[ch + i * avctx->channels];
               break;
            case AV_SAMPLE_FMT_S16P:
               reinterpret_cast<int16_t*>(frame->data[ch])[i] = audio_samples[ch + i * avctx->channels];
               break;
            case AV_SAMPLE_FMT_S32:
               reinterpret_cast<int32_t*>(frame->data[0])[ch + i * avctx->channels] = audio_samples[ch + i * avctx->channels] << 16;
               break;
            case AV_SAMPLE_FMT_S32P:
               reinterpret_cast<int32_t*>(frame->data[ch])[i] = audio_samples[ch + i * avctx->channels] << 16;
               break;
            case AV_SAMPLE_FMT_FLT:
               reinterpret_cast<float*>(frame->data[0])[ch + i * avctx->channels] = static_cast<float>(audio_samples[ch + i * avctx->channels]) / 32767.0f;
               break;
            case AV_SAMPLE_FMT_FLTP:
               reinterpret_cast<float*>(frame->data[ch])[i] = static_cast<float>(audio_samples[ch + i * avctx->channels]) / 32767.f;
               break;
            case AV_SAMPLE_FMT_NONE:
            case AV_SAMPLE_FMT_DBL:
            case AV_SAMPLE_FMT_DBLP:
            case AV_SAMPLE_FMT_NB:
            default:
               wxASSERT(false);
               break;
            }
         }
      }

      ret = avcodec_send_frame(avctx, frame.get());
   }
   else
      ret = avcodec_send_frame(avctx, nullptr);

   if (ret < 0) {
      AudacityMessageBox(
         XO("FFmpeg : ERROR - encoding frame failed"),
         XO("FFmpeg Error"),
         wxOK|wxCENTER|wxICON_EXCLAMATION
      );
      return ret;
   }


   /* read all the available output packets(in general there may be any
    * number of them */
   while (ret >= 0)
   {
      pkt->data = nullptr; // packet data will be allocated by the encoder
      pkt->size = 0;

      pkt->dts = pkt->pts = AV_NOPTS_VALUE; // we don't set frame.pts thus don't trust the AVPacket ts

      ret = avcodec_receive_packet(avctx, pkt);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
         continue;

      if (ret < 0) {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Can't encode audio frame."),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return false;
      }

      // Rescale from the codec time_base to the AVStream time_base.
      if (pkt->pts != int64_t(AV_NOPTS_VALUE))
         pkt->pts = av_rescale_q(pkt->pts, avctx->time_base, avctx->time_base);
      if (pkt->dts != int64_t(AV_NOPTS_VALUE))
         pkt->dts = av_rescale_q(pkt->dts, avctx->time_base, avctx->time_base);
      //wxLogDebug(wxT("FFmpeg : (%d) Writing audio frame with PTS: %lld."), mEncAudioCodecCtx->frame_number, (long long) pkt.pts);

      pkt->stream_index = audio_stream->index;

      // Write the encoded audio frame to the output file.
      if ((ret = av_interleaved_write_frame(avfctx, pkt)) < 0)
      {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Failed to write audio frame to file."),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return false;
      }

      ++got_output;
   }

   if(!audio_samples)
   {
      // Flush the mux.
      if ((ret = av_interleaved_write_frame(avfctx, nullptr)) < 0)
      {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Failed to write audio frame to file."),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return false;
      }
   }


   return got_output;
}


bool ExportFFmpeg::Finalize()
{
   int encodeResult = 0;

   AVPacketEx pkt;

   // Flush the audio FIFO and encoder.
   for (;;)
   {
      const int nFifoBytes = av_fifo_size(mEncAudioFifo.get()); // any bytes left in audio FIFO?

      // Flush the audio FIFO first if necessary. It won't contain a _full_ audio frame because
      // if it did we'd have pulled it from the FIFO during the last encodeAudioFrame() call
      if (nFifoBytes > 0)
      {
         const int nAudioFrameSizeOut = default_frame_size * mEncAudioCodecCtx->channels * sizeof(int16_t);

         if (nAudioFrameSizeOut > mEncAudioFifoOutBufSiz || nFifoBytes > mEncAudioFifoOutBufSiz) {
            AudacityMessageBox(
               XO("FFmpeg : ERROR - Too much remaining data."),
               XO("FFmpeg Error"),
               wxOK | wxCENTER | wxICON_EXCLAMATION
            );
            return false;
         }

         // We have an incomplete buffer of samples left, encode it.
         // If codec supports CODEC_CAP_SMALL_LAST_FRAME, we can feed it with smaller frame
         // Or if frame_size is 1, then it's some kind of PCM codec, they don't have frames and will be fine with the samples
         // Otherwise we'll send a full frame of audio + silence padding to ensure all audio is encoded
         int frame_size = default_frame_size;
         if (mEncAudioCodecCtx->codec->capabilities & AV_CODEC_CAP_SMALL_LAST_FRAME ||
             frame_size == 1)
            frame_size = nFifoBytes / (mEncAudioCodecCtx->channels * sizeof(int16_t));

         wxLogDebug(wxT("FFmpeg : Audio FIFO still contains %d bytes, writing %d sample frame ..."),
            nFifoBytes, frame_size);

         // Fill audio buffer with zeroes. If codec tries to read the whole buffer,
         // it will just read silence. If not - who cares?
         memset(mEncAudioFifoOutBuf.get(), 0, mEncAudioFifoOutBufSiz);

         // Pull the bytes out from the FIFO and feed them to the encoder.
         if (av_fifo_generic_read(mEncAudioFifo.get(), mEncAudioFifoOutBuf.get(), nFifoBytes, nullptr) == 0)
         {
            const auto pts = av_rescale_q(mSamplesWritten, AVRational{ 1, mEncAudioCodecCtx->sample_rate }, mEncAudioCodecCtx->time_base);
            mSamplesWritten += frame_size;
            encodeResult = encode_audio(mEncAudioCodecCtx.get(), mEncFormatCtx.get(), mEncAudioStream, &pkt, mEncAudioFifoOutBuf.get(), frame_size, pts);
         }
         else
         {
            wxLogDebug(wxT("FFmpeg : Reading from Audio FIFO failed, aborting"));
            // TODO: more precise message
            AudacityMessageBox( XO("Unable to export") );
            return false;
         }
      }
      //else
      //{
      //   // Fifo is empty, flush encoder. May be called multiple times.
      //   encodeResult = encode_audio(mEncAudioCodecCtx.get(), mEncFormatCtx.get(), &pkt, nullptr, 0);
      //}

      if (encodeResult < 0) {
         // TODO: more precise message
         AudacityMessageBox( XO("Unable to export") );
         return false;
      }

      if (encodeResult == 0)
         break;

      // We have a packet, send to the muxer
      pkt.stream_index = mEncAudioStream->index;

      // Set presentation time of frame (currently in the codec's timebase) in the stream timebase.
      if (pkt.pts != int64_t(AV_NOPTS_VALUE))
         pkt.pts = av_rescale_q(pkt.pts, mEncAudioCodecCtx->time_base, mEncAudioStream->time_base);
      if (pkt.dts != int64_t(AV_NOPTS_VALUE))
         pkt.dts = av_rescale_q(pkt.dts, mEncAudioCodecCtx->time_base, mEncAudioStream->time_base);
      if (pkt.duration)
         pkt.duration = av_rescale_q(pkt.duration, mEncAudioCodecCtx->time_base, mEncAudioStream->time_base);

      if (av_interleaved_write_frame(mEncFormatCtx.get(), &pkt) != 0)
      {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Couldn't write last audio frame to output file."),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return false;
      }
   }


   { // Scope
      const auto pts = av_rescale_q(mSamplesWritten, AVRational{ 1, mEncAudioCodecCtx->sample_rate }, mEncAudioCodecCtx->time_base);
      encodeResult = encode_audio(mEncAudioCodecCtx.get(), mEncFormatCtx.get(), mEncAudioStream, &pkt, nullptr, 0, pts);
      if (encodeResult < 0) {
         // TODO: more precise message
         AudacityMessageBox(XO("Unable to export"));
         return false;
      }
   }

   // Write any file trailers.
   if (av_write_trailer(mEncFormatCtx.get()) != 0) {
      // TODO: more precise message
      AudacityMessageBox( XO("Unable to export") );
      return false;
   }

   return true;
}

void ExportFFmpeg::FreeResources()
{
   // Close the codecs.
   mEncAudioCodecCtx.reset();

   // Close the output file if we created it.
   mUfileCloser.reset();

   // Free any buffers or structures we allocated.
   mEncFormatCtx.reset();

   mEncAudioFifoOutBuf.reset();
   mEncAudioFifoOutBufSiz = 0;

   mEncAudioFifo.reset();

   av_log_set_callback(av_log_default_callback);
}

bool ExportFFmpeg::EncodeAudioFrame(int16_t *pFrame, size_t frameSize)
{
   int nBytesToWrite = 0;
   uint8_t *pRawSamples = nullptr;
   const int nAudioFrameSizeOut = default_frame_size * mEncAudioCodecCtx->channels * sizeof(int16_t);

   nBytesToWrite = frameSize;
   pRawSamples  = reinterpret_cast<uint8_t*>(pFrame);
   if (av_fifo_realloc2(mEncAudioFifo.get(), av_fifo_size(mEncAudioFifo.get()) + frameSize) < 0)
      return false;

   // Put the raw audio samples into the FIFO.
   int ret = av_fifo_generic_write(mEncAudioFifo.get(), pRawSamples, nBytesToWrite, nullptr);

   if(ret != nBytesToWrite)
      return false;

   if (nAudioFrameSizeOut > mEncAudioFifoOutBufSiz) {
      AudacityMessageBox(
         XO("FFmpeg : ERROR - nAudioFrameSizeOut too large."),
         XO("FFmpeg Error"),
         wxOK|wxCENTER|wxICON_EXCLAMATION
      );
      return false;
   }

   AVPacketEx pkt;

   // Read raw audio samples out of the FIFO in nAudioFrameSizeOut byte-sized groups to encode.
   while ( av_fifo_size(mEncAudioFifo.get()) >= nAudioFrameSizeOut)
   {
      ret = av_fifo_generic_read(mEncAudioFifo.get(), mEncAudioFifoOutBuf.get(), nAudioFrameSizeOut, nullptr);

      const auto pts = av_rescale_q(mSamplesWritten, AVRational{ 1, mEncAudioCodecCtx->sample_rate }, mEncAudioCodecCtx->time_base);

      ret = encode_audio(mEncAudioCodecCtx.get(), mEncFormatCtx.get(), mEncAudioStream,
         &pkt,
         mEncAudioFifoOutBuf.get(), // in
         default_frame_size,
         pts);

      if (ret < 0)
      {
         AudacityMessageBox(
            XO("FFmpeg : ERROR - Can't encode audio frame."),
            XO("FFmpeg Error"),
            wxOK | wxCENTER | wxICON_EXCLAMATION
         );
         return false;
      }

      mSamplesWritten += default_frame_size;
   }
   return true;
}


ProgressResult ExportFFmpeg::Export(AudacityProject *project,
   std::unique_ptr<ProgressDialog> &pDialog,
   unsigned channels, const wxFileNameWrapper &fName,
   bool selectionOnly, double t0, double t1,
   MixerSpec *mixerSpec, const Tags *metadata, int subformat)
{
   if (!CheckFFmpegPresence())
      return ProgressResult::Cancelled;
   mChannels = channels;
   // subformat index may not correspond directly to fmts[] index, convert it
   mSubFormat = AdjustFormatIndex(subformat);
   if (channels > ExportFFmpegOptions::fmts[mSubFormat].maxchannels)
   {
      AudacityMessageBox(
         XO(
"Attempted to export %d channels, but maximum number of channels for selected output format is %d")
            .Format(
               channels,
               ExportFFmpegOptions::fmts[mSubFormat].maxchannels ),
         XO("Error"));
      return ProgressResult::Cancelled;
   }
   mName = fName;
   const auto &tracks = TrackList::Get( *project );
   bool ret = true;

   if (mSubFormat >= FMT_LAST) {
      // TODO: more precise message
      AudacityMessageBox( XO("Unable to export") );
      return ProgressResult::Cancelled;
   }

   wxString shortname(ExportFFmpegOptions::fmts[mSubFormat].shortname);
   if (mSubFormat == FMT_OTHER)
      shortname = gPrefs->Read(wxT("/FileFormats/FFmpegFormat"),wxT("matroska"));
   ret = Init(shortname.mb_str(),project, metadata, subformat);
   auto cleanup = finally ( [&] { FreeResources(); } );

   if (!ret) {
      // TODO: more precise message
      AudacityMessageBox( XO("Unable to export") );
      return ProgressResult::Cancelled;
   }

   size_t pcmBufferSize = 1024;

   auto mixer = CreateMixer(tracks, selectionOnly,
      t0, t1,
      channels, pcmBufferSize, true,
      mSampleRate, int16Sample, true, mixerSpec);

   auto updateResult = ProgressResult::Success;
   {
      InitProgress( pDialog, fName,
         selectionOnly
            ? XO("Exporting selected audio as %s")
                 .Format( ExportFFmpegOptions::fmts[mSubFormat].description )
            : XO("Exporting the audio as %s")
                 .Format( ExportFFmpegOptions::fmts[mSubFormat].description ) );
      auto &progress = *pDialog;

      while (updateResult == ProgressResult::Success) {
         auto pcmNumSamples = mixer->Process(pcmBufferSize);

         if (pcmNumSamples == 0)
            break;

         short *pcmBuffer = (short *)mixer->GetBuffer();

         if (!EncodeAudioFrame(
            pcmBuffer, (pcmNumSamples)*sizeof(int16_t)*mChannels)) {
            // TODO: more precise message, and fix redundancy with messages
            // already given on some of the failure paths of the above call
            AudacityMessageBox( XO("Unable to export") );
            updateResult = ProgressResult::Cancelled;
            break;
         }

         updateResult = progress.Update(mixer->MixGetCurrentTime() - t0, t1 - t0);
      }
   }

   if ( updateResult != ProgressResult::Cancelled )
      if ( !Finalize() ) // Finalize makes its own messages
         return ProgressResult::Cancelled;

   if ( mUfileCloser.close() != 0 ) {
      // TODO: more precise message
      AudacityMessageBox( XO("Unable to export") );
      return ProgressResult::Cancelled;
   }

   return updateResult;
}

void AddStringTagUTF8(char field[], int size, wxString value)
{
      memset(field,0,size);
      memcpy(field,value.ToUTF8(),(int)strlen(value.ToUTF8()) > size -1 ? size -1 : strlen(value.ToUTF8()));
}

void AddStringTagANSI(char field[], int size, wxString value)
{
      memset(field,0,size);
      memcpy(field,value.mb_str(),(int)strlen(value.mb_str()) > size -1 ? size -1 : strlen(value.mb_str()));
}

bool ExportFFmpeg::AddTags(const Tags *tags)
{
   if (tags == nullptr)
   {
      return false;
   }

   SetMetadata(tags, "author", TAG_ARTIST);
   SetMetadata(tags, "album", TAG_ALBUM);
   SetMetadata(tags, "comment", TAG_COMMENTS);
   SetMetadata(tags, "genre", TAG_GENRE);
   SetMetadata(tags, "title", TAG_TITLE);
   SetMetadata(tags, "year", TAG_YEAR);
   SetMetadata(tags, "track", TAG_TRACK);

   return true;
}

void ExportFFmpeg::SetMetadata(const Tags *tags, const char *name, const wxChar *tag)
{
   if (tags->HasTag(tag))
   {
      wxString value = tags->GetTag(tag);

      av_dict_set(&mEncFormatCtx->metadata, name, mSupportsUTF8 ? value.ToUTF8() : value.mb_str(), 0);
   }
}


//----------------------------------------------------------------------------
// AskResample dialog
//----------------------------------------------------------------------------

int ExportFFmpeg::AskResample(int bitrate, int rate, int lowrate, int highrate, const int *sampRates)
{
#if defined(FFMPEG_AUTO_RESAMPLE)
   std::vector<int> rates;

   for (int i = 0; sampRates[i]; ++i)
   {
      rates.push_back(sampRates[i]);
   }

   std::sort(rates.begin(), rates.end());

   int bestRate = 0;
   for (auto i : rates)
   {
      bestRate = i;
      if (i > rate)
      {
         break;
      }
   }

   return bestRate;
#else
   wxDialogWrapper d(nullptr, wxID_ANY, XO("Invalid sample rate"));
   d.SetName();
   wxChoice *choice;
   ShuttleGui S(&d, eIsCreating);

   int selected = -1;

   S.StartVerticalLay();
   {
      S.SetBorder(10);
      S.StartStatic(XO("Resample"));
      {
         S.StartHorizontalLay(wxALIGN_CENTER, false);
         {
            S.AddTitle(
               (bitrate == 0
                  ? XO(
"The project sample rate (%d) is not supported by the current output\nfile format. ")
                       .Format( rate )
                  : XO(
"The project sample rate (%d) and bit rate (%d kbps) combination is not\nsupported by the current output file format. ")
                       .Format( rate, bitrate/1000))
               + XO("You may resample to one of the rates below.")
            );
         }
         S.EndHorizontalLay();

         S.StartHorizontalLay(wxALIGN_CENTER, false);
         {
            choice = S.AddChoice(XO("Sample Rates"),
               [&]{
                  TranslatableStrings choices;
                  for (int i = 0; sampRates[i] > 0; i++)
                  {
                     int label = sampRates[i];
                     if ((!lowrate || label >= lowrate) && (!highrate || label <= highrate))
                     {
                        wxString name = wxString::Format(wxT("%d"),label);
                        choices.push_back( Verbatim( name ) );
                        if (label <= rate)
                           selected = i;
                     }
                  }
                  return choices;
               }(),
               std::max( 0, selected )
            );
         }
         S.EndHorizontalLay();
      }
      S.EndStatic();

      S.AddStandardButtons();
   }
   S.EndVerticalLay();

   d.Layout();
   d.Fit();
   d.SetMinSize(d.GetSize());
   d.Center();

   if (d.ShowModal() == wxID_CANCEL) {
      return 0;
   }

   return wxAtoi(choice->GetStringSelection());
#endif
}

void ExportFFmpeg::OptionsCreate(ShuttleGui &S, int format)
{
   // subformat index may not correspond directly to fmts[] index, convert it
   mSubFormat = AdjustFormatIndex(format);
   if (mSubFormat == FMT_M4A)
   {
      S.AddWindow(
         safenew ExportFFmpegAACOptions{ S.GetParent(), format } );
      return;
   }
   else if (mSubFormat == FMT_AC3)
   {
      S.AddWindow(
         safenew ExportFFmpegAC3Options{ S.GetParent(), format } );
      return;
   }
   else if (mSubFormat == FMT_AMRNB)
   {
      S.AddWindow(
         safenew ExportFFmpegAMRNBOptions{ S.GetParent(), format } );
      return;
   }
   else if (mSubFormat == FMT_OPUS)
   {
      S.AddWindow(
         safenew ExportFFmpegOPUSOptions{ S.GetParent(), format });
      return;
   }
   else if (mSubFormat == FMT_WMA2)
   {
      S.AddWindow(
         safenew ExportFFmpegWMAOptions{ S.GetParent(), format } );
      return;
   }
   else if (mSubFormat == FMT_OTHER)
   {
      S.AddWindow(
         safenew ExportFFmpegCustomOptions{ S.GetParent(), format } );
      return;
   }

   ExportPlugin::OptionsCreate(S, format);
}

} // namespace

static Exporter::RegisteredExportPlugin sRegisteredPlugin{ "FFmpeg",
   []{ return std::make_unique< ExportFFmpeg >(); }
};

#endif

