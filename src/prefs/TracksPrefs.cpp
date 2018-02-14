/**********************************************************************

  Audacity: A Digital Audio Editor

  TracksPrefs.cpp

  Brian Gunlogson
  Joshua Haberman
  Dominic Mazzoni
  James Crook


*******************************************************************//**

\class TracksPrefs
\brief A PrefsPanel for track display and behavior properties.

*//*******************************************************************/

#include "../Audacity.h"
#include "TracksPrefs.h"

//#include <algorithm>
//#include <wx/defs.h>

#include "../Prefs.h"
#include "../ShuttleGui.h"
#include "../WaveTrack.h"

#include "../Experimental.h"
#include "../Internat.h"

int TracksPrefs::iPreferencePinned = -1;

namespace {
   const wxChar *PinnedHeadPreferenceKey()
   {
      return wxT("/AudioIO/PinnedHead");
   }

   bool PinnedHeadPreferenceDefault()
   {
      return false;
   }
}


TracksPrefs::TracksPrefs(wxWindow * parent)
/* i18n-hint: "Tracks" include audio recordings but also other collections of
 * data associated with a time line, such as sequences of labels, and musical
 * notes */
:  PrefsPanel(parent, _("Tracks"))
{
   // Bugs 1043, 1044
   // First rewrite legacy preferences
   gPrefs->Write(wxT("/GUI/DefaultViewModeNew"),
      (int) WaveTrack::FindDefaultViewMode());

   Populate();
}

TracksPrefs::~TracksPrefs()
{
}

void TracksPrefs::Populate()
{
   // Keep view choices and codes in proper correspondence --
   // we don't display them by increasing integer values.

   mViewChoices.Add(_("Waveform"));
   mViewCodes.Add((int)(WaveTrack::Waveform));

   mViewChoices.Add(_("Waveform (dB)"));
   mViewCodes.Add((int)(WaveTrack::obsoleteWaveformDBDisplay));

   mViewChoices.Add(_("Spectrogram"));
   mViewCodes.Add(WaveTrack::Spectrum);


   // How samples are displayed when zoomed in:

   mSampleDisplayChoices.Add(_("Connect dots"));
   mSampleDisplayCodes.Add((int) WaveTrack::LinearInterpolate);

   mSampleDisplayChoices.Add(_("Stem plot"));
   mSampleDisplayCodes.Add((int) WaveTrack::StemPlot);

   mZoomChoices.Add( _("Fit to Width") );
   mZoomCodes.Add( WaveTrack::kZoomToFit );
   mZoomChoices.Add( _("Zoom to Selection") );
   mZoomCodes.Add( WaveTrack::kZoomToSelection );
   mZoomChoices.Add( _("Zoom Default") );
   mZoomCodes.Add( WaveTrack::kZoomDefault );
   mZoomChoices.Add( _("Minutes") );
   mZoomCodes.Add( WaveTrack::kZoomMinutes );
   mZoomChoices.Add( _("Seconds") );
   mZoomCodes.Add( WaveTrack::kZoomSeconds );
   mZoomChoices.Add( _("MilliSeconds") );
   mZoomCodes.Add( WaveTrack::kZoomMilliSeconds );
   mZoomChoices.Add( _("Samples") );
   mZoomCodes.Add( WaveTrack::kZoomSamples );
   mZoomChoices.Add( _("4 Pixels per Sample") );
   mZoomCodes.Add( WaveTrack::kZoom4To1 );
   mZoomChoices.Add( _("Max Zoom") );
   mZoomCodes.Add( WaveTrack::kMaxZoom );



   //------------------------- Main section --------------------
   // Now construct the GUI itself.
   // Use 'eIsCreatingFromPrefs' so that the GUI is
   // initialised with values from gPrefs.
   ShuttleGui S(this, eIsCreatingFromPrefs);
   PopulateOrExchange(S);
   // ----------------------- End of main section --------------
}

void TracksPrefs::PopulateOrExchange(ShuttleGui & S)
{
   S.SetBorder(2);
   S.StartScroller();

   S.StartStatic(_("Display"));
   {
      S.TieCheckBox(_("&Pinned Recording/Playback head"),
                    PinnedHeadPreferenceKey(),
                    PinnedHeadPreferenceDefault());
      S.TieCheckBox(_("A&uto-scroll if head unpinned"),
                    wxT("/GUI/AutoScroll"),
                    true);
      S.TieCheckBox(_("Auto-&fit track height"),
                    wxT("/GUI/TracksFitVerticallyZoomed"),
                    false);

      S.AddSpace(10);

      S.StartMultiColumn(2);
      {
         S.TieChoice(_("Default &view mode:"),
                     wxT("/GUI/DefaultViewModeNew"),
                     0,
                     mViewChoices,
                     mViewCodes);
         S.SetSizeHints(mViewChoices);

         S.TieChoice(_("Display &samples:"),
                     wxT("/GUI/SampleView"),
                     1,
                     mSampleDisplayChoices,
                     mSampleDisplayCodes);
         S.SetSizeHints(mSampleDisplayChoices);

         S.TieTextBox(_("Default audio track &name:"),
                      wxT("/GUI/TrackNames/DefaultTrackName"),
                      _("Audio Track"),
                      30);
      }
      S.EndMultiColumn();

      S.TieCheckBox(_("Sho&w audio track name as overlay"),
                  wxT("/GUI/ShowTrackNameInWaveform"),
                  false);
   }
   S.EndStatic();

   S.StartStatic(_("Zoom Toggle"));
   {
      S.StartMultiColumn(4);
      {
         S.TieChoice(_("Preset 1:"),
                     wxT("/GUI/ZoomPreset1"),
                     WaveTrack::kZoomDefault,
                     mZoomChoices,
                     mZoomCodes);
         S.SetSizeHints(mZoomChoices);

         S.TieChoice(_("Preset 2:"),
                     wxT("/GUI/ZoomPreset2"),
                     WaveTrack::kZoom4To1,
                     mZoomChoices,
                     mZoomCodes);
         S.SetSizeHints(mZoomChoices);
      }
   }
   S.EndStatic();
   S.EndScroller();
}

bool TracksPrefs::GetPinnedHeadPreference()
{
   // JKC: Cache this setting as it is read many times during drawing, and otherwise causes screen flicker.
   // Correct solution would be to re-write wxFileConfig to be efficient.
   if( iPreferencePinned >= 0 )
      return iPreferencePinned == 1;
   bool bResult = gPrefs->ReadBool(PinnedHeadPreferenceKey(), PinnedHeadPreferenceDefault());
   iPreferencePinned = bResult ? 1: 0;
   return bResult;
}

void TracksPrefs::SetPinnedHeadPreference(bool value, bool flush)
{
   iPreferencePinned = value ? 1 :0;
   gPrefs->Write(PinnedHeadPreferenceKey(), value);
   if(flush)
      gPrefs->Flush();
}

wxString TracksPrefs::GetDefaultAudioTrackNamePreference()
{
   const auto name =
      gPrefs->Read(wxT("/GUI/TrackNames/DefaultTrackName"), wxT(""));
   if (name.empty())
      // When nothing was specified,
      // the default-default is whatever translation of...
      return _("Audio Track");
   else
      return name;
}

bool TracksPrefs::Commit()
{
   // Bug 1583: Clear the caching of the preference pinned state.
   iPreferencePinned = -1;
   ShuttleGui S(this, eIsSavingToPrefs);
   PopulateOrExchange(S);

   if (gPrefs->Read(wxT("/GUI/TrackNames/DefaultTrackName"),
                    _("Audio Track")) == _("Audio Track")) {
      gPrefs->DeleteEntry(wxT("/GUI/TrackNames/DefaultTrackName"));
      gPrefs->Flush();
   }

   return true;
}

wxString TracksPrefs::HelpPageName()
{
   return "Tracks_Preferences";
}

PrefsPanel *TracksPrefsFactory::Create(wxWindow *parent) const
{
   wxASSERT(parent); // to justify safenew
   return safenew TracksPrefs(parent);
}
