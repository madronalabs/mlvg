#include "clap-saw-demo.h"
#include <algorithm>

ClapSawDemo::ClapSawDemo() {
  buildParameterDescriptions();
}

void ClapSawDemo::setSampleRate(double sr) {
  // AudioContext is configured by the wrapper, so we just update voice DSP
  for (auto& voice : voiceDSP) {
    // Any samplerate-dependent setup
  }
}

void ClapSawDemo::processVector(const ml::DSPVectorDynamic& inputs, ml::DSPVectorDynamic& outputs, void* stateData) {
  // CRITICAL: Handle denormal numbers to prevent inconsistent loudness and performance issues
  // This was Randy Jones' fix in Sumu - denormals cause unpredictable behavior in filters/envelopes
  ml::UsingFlushDenormalsToZero denormalHandler;
  
  // Get AudioContext from stateData for MIDI voice access
  auto* audioContext = static_cast<ml::AudioContext*>(stateData);
  if (!audioContext) {
    // Output silence if no audio context
    outputs = ml::DSPVectorDynamic(2);
    outputs[0] = ml::DSPVector{0.0f};
    outputs[1] = ml::DSPVector{0.0f};
    return;
  }

  // Buffer process voices to generate audio.
  ml::DSPVector totalOutput{0.0f};

  // Process all voices - use the same pattern as mltemplate
  const int maxVoices = std::min(static_cast<int>(voiceDSP.size()), audioContext->getInputPolyphony());

  for (int v = 0; v < maxVoices; ++v) {
    // Process voice - follow Sumu's pattern exactly: always process, let processVoice handle activity
    auto& voice = const_cast<ml::EventsToSignals::Voice&>(audioContext->getInputVoice(v));
    ml::DSPVector voiceOutput = processVoice(v, voice, audioContext);
    totalOutput += voiceOutput;
  }

  // Apply gain parameter
  float masterGain = this->getRealFloatParam("gain");
  float debugGainReduction = 0.5f;
  float totalGain = debugGainReduction * masterGain;

  // Apply gain and set outputs directly
  totalOutput = totalOutput * ml::DSPVector(totalGain);
  outputs[0] = totalOutput;
  outputs[1] = totalOutput;
}


ml::DSPVector ClapSawDemo::processVoice(int voiceIndex, ml::EventsToSignals::Voice& voice, ml::AudioContext* audioContext) {
  // Bounds check to prevent crashes
  if (voiceIndex < 0 || voiceIndex >= voiceDSP.size()) {
    return ml::DSPVector{0.0f};
  }

  // Safety check for AudioContext
  if (!audioContext) {
    return ml::DSPVector{0.0f};
  }

  // Get voice control signals provided by EventsToSignals
  const ml::DSPVector vPitch = voice.outputs.row(ml::kPitch);  // MIDI note with pitch bend, gliding
  const ml::DSPVector vGate = voice.outputs.row(ml::kGate);    // MIDI velocity
  const ml::DSPVector vMod = voice.outputs.row(ml::kMod);      // Mod wheel, aftertouch

  const float sr = audioContext->getSampleRate();
  if (sr <= 0.0f) {
    return ml::DSPVector{0.0f};
  }

  // Convert MIDI pitch to Hz using standard formula: 440 * 2^((note-69)/12)
  const ml::DSPVector vPitchOffset = vPitch - ml::DSPVector(69.0f);
  const ml::DSPVector vPitchRatio = pow(ml::DSPVector(2.0f), vPitchOffset * ml::DSPVector(1.0f/12.0f));
  const ml::DSPVector vFreqHz = ml::DSPVector(440.0f) * vPitchRatio;

  const ml::DSPVector vFreqNorm = vFreqHz / ml::DSPVector(sr);
  const ml::DSPVector vOscillator = voiceDSP[voiceIndex].sawOscillator(vFreqNorm);

  // Get filter parameters from ParameterTree with safety bounds
  float filterFreq = std::max(10.0f, std::min(10000.0f, this->getRealFloatParam("f0")));
  float filterQ = std::max(0.01f, std::min(10.0f, this->getRealFloatParam("Q")));

  // Update filter coefficients with current parameter values
  // Lopass::makeCoeffs expects k = 1/Q, where k=0 is maximum resonance
  float filterK = 1.0f / filterQ;
  voiceDSP[voiceIndex].mLoPass._coeffs = ml::Lopass::makeCoeffs(filterFreq / sr, filterK);
  const ml::DSPVector vFiltered = voiceDSP[voiceIndex].mLoPass(vOscillator);

  // Get ADSR parameters
  float attack = this->getRealFloatParam("attack");
  float decay = this->getRealFloatParam("decay");
  float sustain = this->getRealFloatParam("sustain");
  float release = this->getRealFloatParam("release");

  // Update ADSR coefficients
  voiceDSP[voiceIndex].mADSR.coeffs = ml::ADSR::calcCoeffs(attack, decay, sustain, release, sr);

  // Process ADSR envelope with gate signal
  const ml::DSPVector vEnvelope = voiceDSP[voiceIndex].mADSR(vGate);
  const ml::DSPVector vOutput = vFiltered * vEnvelope;

  return vOutput;
}

void ClapSawDemo::buildParameterDescriptions() {
  ml::ParameterDescriptionList params;

  params.push_back(std::make_unique<ml::ParameterDescription>(ml::WithValues{
    {"name", "gain"},
    {"range", {0.0f, 1.0f}},
    {"plaindefault", 0.7f},
    {"units", ""}
  }));

  params.push_back(std::make_unique<ml::ParameterDescription>(ml::WithValues{
    {"name", "f0"},
    {"range", {10.0f, 10000.0f}},
    {"plaindefault", 2000.0f},
    {"units", "Hz"}
  }));

  params.push_back(std::make_unique<ml::ParameterDescription>(ml::WithValues{
    {"name", "Q"},
    {"range", {0.4f, 20.0f}},
    {"plaindefault", 3.4f},
    {"units", ""}
  }));

  // ADSR envelope parameters
  params.push_back(std::make_unique<ml::ParameterDescription>(ml::WithValues{
    {"name", "attack"},
    {"range", {0.001f, 2.0f}},
    {"plaindefault", 0.01f},
    {"units", "s"}
  }));

  params.push_back(std::make_unique<ml::ParameterDescription>(ml::WithValues{
    {"name", "decay"},
    {"range", {0.001f, 2.0f}},
    {"plaindefault", 0.1f},
    {"units", "s"}
  }));

  params.push_back(std::make_unique<ml::ParameterDescription>(ml::WithValues{
    {"name", "sustain"},
    {"range", {0.0f, 1.0f}},
    {"plaindefault", 0.7f},
    {"units", ""}
  }));

  params.push_back(std::make_unique<ml::ParameterDescription>(ml::WithValues{
    {"name", "release"},
    {"range", {0.001f, 4.0f}},
    {"plaindefault", 0.2f},
    {"units", "s"}
  }));

  this->buildParams(params);

  // Set default parameter values after building
  this->setDefaultParams();
}
