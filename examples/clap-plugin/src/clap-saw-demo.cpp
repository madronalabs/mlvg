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

void ClapSawDemo::processAudioContext() {
  // Safety check - ensure AudioContext is valid
  if (!audioContext) {
    return;
  }

  // AudioContext has already processed events and voice management.
  // We just read voice states and generate audio.
  activeVoiceCount = 0;
  audioContext->outputs[0] = ml::DSPVector{0.0f};
  audioContext->outputs[1] = ml::DSPVector{0.0f};

  try {
    // Process all voices - the gate multiplication naturally silences inactive ones
    // Limit to our voice array size to prevent crashes
    const int maxVoices = std::min(static_cast<int>(voiceDSP.size()), audioContext->getInputPolyphony());

    ml::DSPVector totalOutput{0.0f};

    for (int v = 0; v < maxVoices; ++v) {
      // Safety check for voice access
      if (v >= audioContext->getInputPolyphony()) {
        break;
      }

      auto& voice = const_cast<ml::EventsToSignals::Voice&>(audioContext->getInputVoice(v));

      // Process all voices - ADSR will handle envelope generation
      ml::DSPVector voiceOutput = processVoice(v, voice);

      // Track activity using actual voice output (includes ADSR envelope)
      float voiceLevel = ml::sum(voiceOutput * voiceOutput);
      const float silenceThreshold = 1e-6f; // Small epsilon for floating-point comparison
      if (voiceLevel > silenceThreshold) {
        activeVoiceCount++;
        totalOutput += voiceOutput;
      }
    }

    // Apply gain parameter and voice normalization
    float masterGain = this->getRealFloatParam("gain");

    // Normalize by voice count to prevent clipping when multiple voices play
    float voiceNormalization = (activeVoiceCount > 0) ? (1.0f / std::sqrt(activeVoiceCount)) : 1.0f;

    // Apply moderate gain reduction (can increase once testing confirms we're not exploding speakers)
    float debugGainReduction = 0.5f;
    float totalGain = debugGainReduction * voiceNormalization * masterGain;

    // Apply final gain staging
    audioContext->outputs[0] = totalOutput * ml::DSPVector(totalGain);  // Mono to stereo
    audioContext->outputs[1] = totalOutput * ml::DSPVector(totalGain);



  } catch (const std::exception& e) {
    // If any exception occurs, output silence to prevent audio issues
    audioContext->outputs[0] = ml::DSPVector{0.0f};
    audioContext->outputs[1] = ml::DSPVector{0.0f};
    activeVoiceCount = 0;
  }
}

ml::DSPVector ClapSawDemo::processVoice(int voiceIndex, ml::EventsToSignals::Voice& voice) {
  // Bounds check to prevent crashes
  if (voiceIndex < 0 || voiceIndex >= voiceDSP.size()) {
    return ml::DSPVector{0.0f};
  }

  // Safety check for AudioContext
  if (!audioContext) {
    return ml::DSPVector{0.0f};
  }

  try {
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

  } catch (const std::exception& e) {
    // If any exception occurs, return silence
    return ml::DSPVector{0.0f};
  }
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

// All CLAP boilerplate methods moved to CLAPSignalProcessor base class
