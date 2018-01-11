/**********************************************************************

  Audacity: A Digital Audio Editor

  Noise.h

  Dominic Mazzoni

  An effect to add white noise.

**********************************************************************/

#ifndef __AUDACITY_EFFECT_NOISE__
#define __AUDACITY_EFFECT_NOISE__

#include "Effect.h"
#include "RngSupport.h"
#include <nyquist/xlisp/xlisp.h>

class NumericTextCtrl;
class ShuttleGui;

class EffectNoise final : public Effect
{
public:
   static const ComponentInterfaceSymbol Symbol;

   EffectNoise();
   virtual ~EffectNoise();

   // ComponentInterface implementation

   ComponentInterfaceSymbol GetSymbol() override;
   TranslatableString GetDescription() override;
   wxString ManualPage() override;

   // EffectDefinitionInterface implementation

   EffectType GetType() override;

   // EffectClientInterface implementation

   unsigned GetAudioOutCount() override;
   size_t ProcessBlock(float **inBlock, float **outBlock, size_t blockLen) override;
   bool DefineParams( ShuttleParams & S ) override;
   bool GetAutomationParameters(CommandParameters & parms) override;
   bool SetAutomationParameters(CommandParameters & parms) override;

   // Effect implementation

   bool Startup() override;
   void PopulateOrExchange(ShuttleGui & S) override;
   bool TransferDataToWindow() override;
   bool TransferDataFromWindow() override;

private:
   // EffectNoise implementation

private:
   int mType;
   double mAmp;
   int mDist;

   class Brownian final
   {
      float y{}, z{};
      EffectNoise& noise;
   public:
      Brownian(EffectNoise& parent) : noise(parent) {}
      template<typename Distribution>
      void Process(size_t size, float* buffer, Distribution& dist);
   };

   class Pink final
   {
      float buf0{}, buf1{}, buf2{}, buf3{}, buf4{}, buf5{}, buf6{};
      EffectNoise& noise;
   public:
      Pink(EffectNoise& parent) : noise(parent) {}
      template<typename Distribution>
      void Process(size_t size, float* buffer, Distribution& dist);
   };

   Nyq::NyqEngine generator;

   Brownian brownian;
   Pink pink;

   NumericTextCtrl *mNoiseDurationT{};
};

#endif
