#pragma once

#include "CLAPExport.h"  // Includes madronalib core + CLAPSignalProcessor base class

#ifdef HAS_GUI
class ClapSawDemoGUI;
#endif

class ClapSawDemo : public ml::CLAPSignalProcessor<> {
public:
  // Required constants for CLAP plugin
  static constexpr int kNumVoices = 16;
  
private:
  // the AudioContext's EventsToSignals handles state
  ml::AudioContext* audioContext = nullptr;  // Set by wrapper

  // Per-voice DSP components
  struct VoiceDSP {
    ml::SawGen sawOscillator;
    ml::Lopass mLoPass;
    ml::ADSR mADSR;
  };
  std::array<VoiceDSP, kNumVoices> voiceDSP;

  // Simple voice activity tracking for CLAP
  int activeVoiceCount = 0;



public:
  ClapSawDemo();
  ~ClapSawDemo() = default;

  // SignalProcessor interface
  void setSampleRate(double sr) override;
  void buildParameterDescriptions();

  // Required processVector method for CLAP
  void processVector(const ml::DSPVectorDynamic& inputs, ml::DSPVectorDynamic& outputs, void* stateData = nullptr) override;

  void setAudioContext(ml::AudioContext* ctx) { audioContext = ctx; }

  // Voice activity for CLAP sleep/continue
  bool hasActiveVoices() const override { return activeVoiceCount > 0; }



  // Plugin-specific interface
  const ml::ParameterTree& getParameterTree() const { return this->_params; }

private:
  // Helper methods go here
  ml::DSPVector processVoice(int voiceIndex, ml::EventsToSignals::Voice& voice, ml::AudioContext* audioContext);
};
