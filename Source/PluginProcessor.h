/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

/* 前方宣言 */
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



    //以下追加分
private:
    /* パラメータ */
    juce::AudioParameterFloat* paramGain;
    juce::AudioParameterFloat* paramFreqMin;
    juce::AudioParameterFloat* paramFreqMax;


    /* 処理用 */
    //入力バッファ
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbInL;
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbInR;
    //中間バッファ
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbTmpL;
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbTmpR;
    //出力バッファ
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbOutL;
    std::shared_ptr<Alstia::Buffer::RingBuffer> rbOutR;

    /* FFT用変数を用意する */
    //2のn乗 (2^10 = 1024)
    static constexpr int fftOrder = 10;
    //FFTサイズ(ビットシフトで計算)
    static constexpr int fftSize = 1 << fftOrder;
    //オーバーラップ率設定 (0.5 : ハーフオーバーラップ)
    static constexpr float overlap = 0.5f;
    //窓関数用配列
    std::array<float, fftSize> window;
    //FFT用オブジェクト
    juce::dsp::FFT fft;


    //エフェクト処理用関数
    void processEffect(std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf, std::shared_ptr<Alstia::Buffer::RingBuffer>& tmpBuf, std::shared_ptr<Alstia::Buffer::RingBuffer>& outBuf);
    //音声出力
    void processOutSamples(std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf, float* outBuf, const int numSamples);
};
