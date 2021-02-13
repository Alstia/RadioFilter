/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/* �O���錾 */
namespace Alstia {
    namespace Buffer {
        class RingBuffer;
    }
}

//==============================================================================
/**
*/
class RadioFilterAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    RadioFilterAudioProcessor();
    ~RadioFilterAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioFilterAudioProcessor)



    //�ȉ��ǉ���
private:
    /* �p�����[�^ */
    juce::AudioParameterFloat* paramGain;
    juce::AudioParameterFloat* paramFreqMin;
    juce::AudioParameterFloat* paramFreqMax;


    /* �����p */
    //���̓o�b�t�@
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbInL;
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbInR;
    //���ԃo�b�t�@
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbTmpL;
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbTmpR;
    //�o�̓o�b�t�@
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbOutL;
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbOutR;

    /* FFT�p�ϐ���p�ӂ��� */
    //2��n�� (2^10 = 1024)
    static constexpr int fftOrder = 10;
    //FFT�T�C�Y(�r�b�g�V�t�g�Ōv�Z)
    static constexpr int fftSize = 1 << fftOrder;
    //�I�[�o�[���b�v���ݒ� (0.5 : �n�[�t�I�[�o�[���b�v)
    static constexpr float overlap = 0.5f;
    //���֐��p�z��
    std::array<float, fftSize> window;
    //FFT�p�I�u�W�F�N�g
    juce::dsp::FFT fft;


    //�G�t�F�N�g�����p�֐�
    void processEffect(std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf, std::shared_ptr<Alstia::Buffer::RingBuffer>& tmpBuf, std::shared_ptr<Alstia::Buffer::RingBuffer>& outBuf);
    //�����o��
    void processOutSamples(std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf, float* outBuf, const int numSamples);
};
