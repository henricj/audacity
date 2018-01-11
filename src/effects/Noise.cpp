/**********************************************************************

  Audacity: A Digital Audio Editor

  Noise.cpp

  Dominic Mazzoni

*******************************************************************//**

\class EffectNoise
\brief An effect to add white noise.

*//*******************************************************************/

#include "../Audacity.h"
#include "Noise.h"
#include "RngSupport.h"

#include <cmath>

#include <wx/choice.h>
#include <wx/intl.h>
#include <wx/textctrl.h>
#include <wx/valgen.h>

#include "../Prefs.h"
#include "../Shuttle.h"
#include "../ShuttleGui.h"
#include "../widgets/valnum.h"
#include "../widgets/NumericTextCtrl.h"

namespace
{
   const auto uniform_to_normal = sqrtf(1 / 3.0f);
};

enum kTypes
{
   kWhite,
   kPink,
   kBrownian,
   nTypes
};

static const EnumValueSymbol kTypeStrings[nTypes] =
{
   // These are acceptable dual purpose internal/visible names
   { XO("White") },
   { XO("Pink") },
   { XO("Brownian") }
};

enum kDists
{
   kUniform,
   kGaussian,
   nDists
};

static const EnumValueSymbol kDistStrings[nDists] =
{
   { XO("Uniform") },
   { XO("Gaussian") }
};

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name    Type     Key               Def      Min   Max            Scale
Param( Type,   int,     wxT("Type"),       kWhite,  0,    nTypes - 1, 1  );
Param( Amp,    double,  wxT("Amplitude"),  0.8,     0.0,  1.0,           1  );
Param( Dist,   int,     wxT("Distribution"), kUniform, 0, nDists - 1, 1);

//
// EffectNoise
//

EffectNoise::EffectNoise() : brownian(*this), pink(*this)
{
   mType = DEF_Type;
   mAmp = DEF_Amp;
   mDist = DEF_Dist;

   SetLinearEffectFlag(true);
}

EffectNoise::~EffectNoise() = default;

// ComponentInterface implementation

ComponentInterfaceSymbol EffectNoise::GetSymbol()
{
   return NOISE_PLUGIN_SYMBOL;
}

TranslatableString EffectNoise::GetDescription()
{
   return XO("Generates one of three different types of noise");
}

wxString EffectNoise::ManualPage()
{
   return wxT("Noise");
}

// EffectDefinitionInterface implementation

EffectType EffectNoise::GetType()
{
   return EffectTypeGenerate;
}

// EffectClientInterface implementation

unsigned EffectNoise::GetAudioOutCount()
{
   return 1;
}

template<typename Distribution>
void EffectNoise::Brownian::Process(size_t size, float* const buffer, Distribution& dist)
{
   const auto sample_rate = static_cast<float>(noise.mSampleRate);
   const auto amp = static_cast<float>(noise.mAmp);
   auto& rng = noise.generator;

   //float leakage=0.997; // experimental value at 44.1kHz
   //double scaling = 0.05; // experimental value at 44.1kHz
   // min and max protect against instability at extreme sample rates.
   const auto leakage = ((sample_rate - 144.0f) / sample_rate < 0.9999f)
                           ? (sample_rate - 144.0f) / sample_rate
                           : 0.9999f;

   const auto scaling = (9.0f / sqrtf(sample_rate) > 0.01f)
                           ? 9.0f / sqrtf(sample_rate)
                           : 0.01f;

   std::generate_n(&buffer[0], size,
                   [&] {
                      const auto white = dist(rng);
                      z = leakage * y + white * scaling;
                      y = fabsf(z) > 1.0f
                             ? leakage * y - white * scaling
                             : z;
                      return amp * y;
                   });
}

template<typename Distribution>
void EffectNoise::Pink::Process(size_t size, float* const buffer, Distribution& dist)
{
   const auto noise_amp = static_cast<float>(noise.mAmp);
   auto& rng = noise.generator;

   // based on Paul Kellet's "instrumentation grade" algorithm.

   // 0.129f is an experimental normalization factor.
   const auto amplitude = noise_amp * 0.129f;
   for (decltype(size) i = 0; i < size; i++)
   {
      const auto white = dist(rng);
      buf0 = 0.99886f * buf0 + 0.0555179f * white;
      buf1 = 0.99332f * buf1 + 0.0750759f * white;
      buf2 = 0.96900f * buf2 + 0.1538520f * white;
      buf3 = 0.86650f * buf3 + 0.3104856f * white;
      buf4 = 0.55000f * buf4 + 0.5329522f * white;
      buf5 = -0.7616f * buf5 - 0.0168980f * white;
      buffer[i] = amplitude *
         (buf0 + buf1 + buf2 + buf3 + buf4 + buf5 + buf6 + white * 0.5362f);
      buf6 = white * 0.115926f;
   }
}

size_t EffectNoise::ProcessBlock(float **WXUNUSED(inbuf), float **outbuf, size_t size)
{
   float * const buffer = outbuf[0];

   // The variance of a uniform distribution is (-mAmp - mAmp)^2 / 12.
   // Adjust the normal distribution to give the same variance

   const auto ampf = static_cast<float>(mAmp);

   switch (mType)
   {
   default:
   case kWhite: // white
      if (mDist == kGaussian)
      {
         std::normal_distribution<float> gaussian{0, ampf * uniform_to_normal};
         std::generate_n(&buffer[0], size, [&] { return gaussian(generator); });
      }
      else
      {
         std::uniform_real_distribution<float> uniform{ -ampf, ampf };
         std::generate_n(&buffer[0], size, [&] { return uniform(generator); });
      }
      break;

   case kPink: // pink
      if (mDist == kGaussian)
      {
         std::normal_distribution<float> gaussian{ 0.0f, 1.0f * uniform_to_normal };
         pink.Process(size, buffer, gaussian);
      }
      else
      {
         std::uniform_real_distribution<float> uniform{ -1.0f, 1.0f };
         pink.Process(size, buffer, uniform);
      }
      break;

   case kBrownian: // Brownian
      if (mDist == kGaussian)
      {
         std::normal_distribution<float> gaussian{ 0.0f, ampf * uniform_to_normal };
         brownian.Process(size, buffer, gaussian);
      }
      else
      {
         std::uniform_real_distribution<float> uniform{ -ampf, ampf };
         brownian.Process(size, buffer, uniform);
      }
      break;
   }

   return size;
}
bool EffectNoise::DefineParams( ShuttleParams & S ){
   S.SHUTTLE_ENUM_PARAM( mType, Type, kTypeStrings, nTypes );
   S.SHUTTLE_PARAM( mAmp, Amp );
   S.SHUTTLE_ENUM_PARAM( mDist, Dist, kDistStrings, nDists );

   return true;
}

bool EffectNoise::GetAutomationParameters(CommandParameters & parms)
{
   parms.Write(KEY_Type, kTypeStrings[mType].Internal());
   parms.Write(KEY_Amp, mAmp);
   parms.Write(KEY_Dist, kDistStrings[mDist].Internal());

   return true;
}

bool EffectNoise::SetAutomationParameters(CommandParameters & parms)
{
   ReadAndVerifyEnum(Type, kTypeStrings, nTypes);
   ReadAndVerifyDouble(Amp);
   ReadAndVerifyEnum(Dist, kDistStrings, nDists);

   mType = Type;
   mAmp = Amp;
   mDist = Dist;

   return true;
}

// Effect implementation

bool EffectNoise::Startup()
{
   const wxString base = wxT("/Effects/Noise/");

   // Migrate settings from 2.1.0 or before

   // Already migrated, so bail
   if (gPrefs->Exists(base + wxT("Migrated")))
   {
      return true;
   }

   // Load the old "current" settings
   if (gPrefs->Exists(base))
   {
      gPrefs->Read(base + wxT("Type"), &mType, 0L);
      gPrefs->Read(base + wxT("Amplitude"), &mAmp, 0.8f);

      SaveUserPreset(GetCurrentSettingsGroup());

      // Do not migrate again
      gPrefs->Write(base + wxT("Migrated"), true);
      gPrefs->Flush();
   }

   return true;
}

void EffectNoise::PopulateOrExchange(ShuttleGui & S)
{
   wxASSERT(nTypes == WXSIZEOF(kTypeStrings));
   wxASSERT(nDists == WXSIZEOF(kDistStrings));

   S.StartMultiColumn(2, wxCENTER);
   {
      S.Validator<wxGenericValidator>(&mType)
         .AddChoice(XO("Noise type:"), Msgids(kTypeStrings, nTypes));

      S.Validator<wxGenericValidator>(&mDist)
         .AddChoice(XO("Distribution:"), Msgids(kDistStrings, nDists));
         
      S.Validator<FloatingPointValidator<double>>(
            6, &mAmp, NumValidatorStyle::NO_TRAILING_ZEROES, MIN_Amp, MAX_Amp
         )
         .AddTextBox(XO("Amplitude (0-1):"), wxT(""), 12);

      S.AddPrompt(XO("Duration:"));
      mNoiseDurationT = safenew
         NumericTextCtrl(S.GetParent(), wxID_ANY,
                         NumericConverter::TIME,
                         GetDurationFormat(),
                         GetDuration(),
                         mProjectRate,
                         NumericTextCtrl::Options{}
                            .AutoPos(true));
      S.Name(XO("Duration"))
         .Position(wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL | wxALL)
         .AddWindow(mNoiseDurationT);
   }
   S.EndMultiColumn();
}

bool EffectNoise::TransferDataToWindow()
{
   if (!mUIParent->TransferDataToWindow())
   {
      return false;
   }

   mNoiseDurationT->SetValue(GetDuration());

   return true;
}

bool EffectNoise::TransferDataFromWindow()
{
   if (!mUIParent->Validate() || !mUIParent->TransferDataFromWindow())
   {
      return false;
   }

   SetDuration(mNoiseDurationT->GetValue());

   return true;
}
