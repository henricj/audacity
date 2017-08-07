/**********************************************************************

  Audacity: A Digital Audio Editor

  Prefs.h

  Dominic Mazzoni
  Markus Meyer

  Audacity uses wxWidgets' wxFileConfig class to handle preferences.
  In Audacity versions prior to 1.3.1, it used wxConfig, which would
  store the prefs in a platform-dependent way (e.g. in the registry
  on Windows). Now it always stores the settings in a configuration file
  in the Audacity Data Directory.

  Every time we read a preference, we need to specify the default
  value for that preference, to be used if the preference hasn't
  been set before.

  So, to avoid code duplication, we provide functions in this file
  to read and write preferences which have a nonobvious default
  value, so that if we later want to change this value, we only
  have to change it in one place.

  See Prefs.cpp for a (complete?) list of preferences we keep
  track of...

**********************************************************************/
#ifndef __AUDACITY_PREFS__
#define __AUDACITY_PREFS__

#include "Audacity.h"

#include "../include/audacity/ComponentInterface.h"

#include <wx/fileconf.h>  // to inherit wxFileConfig

#include <iterator>
#include <vector>

void InitPreferences();
void FinishPreferences();

class AudacityPrefs;


extern AUDACITY_DLL_API AudacityPrefs *gPrefs;
extern int gMenusDirty;


/// \brief Our own specialisation of wxFileConfig.  It is essentially a renaming,
/// though it does provide one new access function.  Most of the prefs work
/// is actually done by the InitPreferences() function.
class  AUDACITY_DLL_API AudacityPrefs : public wxFileConfig 
{
public:
   AudacityPrefs(const wxString& appName = {},
               const wxString& vendorName = {},
               const wxString& localFilename = {},
               const wxString& globalFilename = {},
               long style = wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_GLOBAL_FILE,
               const wxMBConv& conv = wxConvAuto());
   bool GetEditClipsCanMove();
};

/// Packages a table of user-visible choices each with an internal code string,
/// a preference key path, and a default choice
class ChoiceSetting
{
public:
   ChoiceSetting(
      const wxString &key,
      const EnumValueSymbol symbols[], size_t nSymbols,
      size_t defaultSymbol
   )
      : mKey{ key }

      , mSymbols{ symbols }
      , mnSymbols{ nSymbols }

      , mDefaultSymbol{ defaultSymbol }
   {
      wxASSERT( defaultSymbol < nSymbols );
   }

   const wxString &Key() const { return mKey; }
   const EnumValueSymbol &Default() const
      { return mSymbols[mDefaultSymbol]; }
   const EnumValueSymbol *begin() const { return mSymbols; }
   const EnumValueSymbol *end() const { return mSymbols + mnSymbols; }

   wxString Read() const;
   bool Write( const wxString &value ); // you flush gPrefs afterward

protected:
   size_t Find( const wxString &value ) const;
   virtual void Migrate( wxString& );

   const wxString mKey;

   const EnumValueSymbol *mSymbols;
   const size_t mnSymbols;

   // stores an internal value
   mutable bool mMigrated { false };

   const size_t mDefaultSymbol;
};

/// Extends ChoiceSetting with a corresponding table of integer codes
/// (generally not equal to their table positions),
/// and optionally an old preference key path that stored integer codes, to be
/// migrated into one that stores internal string values instead
class EnumSetting : public ChoiceSetting
{
public:
   EnumSetting(
      const wxString &key,
      const EnumValueSymbol symbols[], size_t nSymbols,
      size_t defaultSymbol,

      const int intValues[] = nullptr, // must have same size as symbols
      const wxString &oldKey = wxString("")
   )
      : ChoiceSetting{ key, symbols, nSymbols, defaultSymbol }
      , mIntValues{ intValues }
      , mOldKey{ oldKey }
   {
      wxASSERT( mIntValues );
   }

   // Read and write the encoded values
   virtual int ReadInt() const;
   bool WriteInt( int code ); // you flush gPrefs afterward

protected:
   size_t FindInt( int code ) const;
   void Migrate( wxString& ) override;

private:
   const int *mIntValues;
   const wxString mOldKey;
};

struct PrefsDefaultConfigRef { static wxConfigBase* get() { return wxConfigBase::Get(); } };
struct PrefsGlobalConfigRef { static wxConfigBase* get() { return gPrefs; } };

template<typename T>
class PreferenceVariable final
{
   const wxString name_;
   const T defaultValue_;
public:
   explicit PreferenceVariable(const wxString& name, T defaultValue = T())
      : name_(name), defaultValue_(defaultValue)
   { }
   explicit PreferenceVariable(wxString&& name, T defaultValue = T())
      : name_(name), defaultValue_(defaultValue)
   { }

   const wxString& name() const { return name_; }

   bool Exists() const { return gPrefs->HasEntry(name_); }
   bool Delete(bool groupIfEmptyAlso = true) const { return gPrefs->DeleteEntry(name_, groupIfEmptyAlso); }

   bool Read(T& value) const { return gPrefs->Read(name_, &value, defaultValue_); }
   T Read() const { return gPrefs->Read(name_, defaultValue_); }

   bool Write(const T& value) const { return gPrefs->Write(name_, value); }
   bool Write(T&& value) const { return gPrefs->Write(name_, value); }
};

template<>
inline bool PreferenceVariable<bool>::Read() const { return gPrefs->ReadBool(name_, defaultValue_); }
template<>
inline long PreferenceVariable<long>::Read() const { return gPrefs->ReadLong(name_, defaultValue_); }
template<>
inline double PreferenceVariable<double>::Read() const { return gPrefs->ReadDouble(name_, defaultValue_); }

typedef PreferenceVariable<wxString> PreferenceVariableString;
typedef PreferenceVariable<bool> PreferenceVariableBool;
typedef PreferenceVariable<long> PreferenceVariableLong;
typedef PreferenceVariable<double> PreferenceVariableDouble;

template<typename TConfigRef = PrefsGlobalConfigRef>
class PreferenceGroup final
{
   const wxString name_;
public:
   explicit PreferenceGroup(const wxString& name) : name_(name)
   { }
   explicit PreferenceGroup(wxString&& name)
      : name_(name)
   { }

   const wxString& name() const { return name_; }

   bool Exists() const { return gPrefs->HasGroup(name_); }
   bool Delete() const { return gPrefs->DeleteGroup(name_); }

   std::vector<wxString> Groups() const;
   std::vector<wxString> Entries() const;
};


template<typename TConfigRef>
std::vector<wxString> PreferenceGroup<TConfigRef>::Groups() const
{
   auto oldPath = gPrefs->GetPath();
   gPrefs->SetPath(name_);

   std::vector<wxString> v;

   long index;
   wxString name;

   auto x = gPrefs->GetFirstGroup(name, index);

   while (x)
   {
      v.push_back(name);

      x = gPrefs->GetNextGroup(name, index);
   }

   gPrefs->SetPath(oldPath);

   return v;
}

template<typename TConfigRef>
std::vector<wxString> PreferenceGroup<TConfigRef>::Entries() const
{
   auto oldPath = gPrefs->GetPath();
   gPrefs->SetPath(name_);

   std::vector<wxString> v;

   long index;
   wxString name;

   auto x = gPrefs->GetFirstEntry(name, index);

   while (x)
   {
      v.push_back(name);

      x = gPrefs->GetNextEntry(name, index);
   }

   gPrefs->SetPath(oldPath);

   return v;
}

#endif
