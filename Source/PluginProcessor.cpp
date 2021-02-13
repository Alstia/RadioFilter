/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"

#include "util/buffer/ring/RingBuffer.h"

//==============================================================================
RadioFilterAudioProcessor::RadioFilterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                        ),
#else
    :
#endif
//メンバ変数初期化
paramGain(nullptr),
paramFreqMin(nullptr),
paramFreqMax(nullptr),
rbInL(nullptr),
rbInR(nullptr),
rbTmpL(nullptr),
rbTmpR(nullptr),
rbOutL(nullptr),
rbOutR(nullptr),
window(),
fft(fftOrder)
{
    /* パラメータ初期化 (生ポインターの所有権は親クラスによって管理されるので手動での解放は不要) */
    //ゲイン率
    addParameter(paramGain =
        new juce::AudioParameterFloat(
            "gain", //パラメータID
            "Gain", //パラメータ名
            0.0f,   //最小値
            1.0f,   //最大値
            1.0f    //初期値
        ));
    //ゲイン率
    addParameter(paramFreqMin =
        new juce::AudioParameterFloat(
            "freqMin",      //パラメータID
            "FreqMin",      //パラメータ名
            1.0f,           //最小値
            4000.0f,        //最大値
            650.0f          //初期値
        ));
    //ゲイン率
    addParameter(paramFreqMax =
        new juce::AudioParameterFloat(
            "freqMax",      //パラメータID
            "FreqMax",      //パラメータ名
            1.0f,           //最小値
            4000.0f,        //最大値
            2500.0f         //初期値
        ));
}

RadioFilterAudioProcessor::~RadioFilterAudioProcessor()
{
}

//==============================================================================
const juce::String RadioFilterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RadioFilterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool RadioFilterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool RadioFilterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double RadioFilterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RadioFilterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int RadioFilterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void RadioFilterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String RadioFilterAudioProcessor::getProgramName (int index)
{
    return {};
}

void RadioFilterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void RadioFilterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /* 初期化 */
    //サイズ計算 (とりあえず最低必要サイズの倍確保しておく)
    const unsigned int bufferSize = fftSize < samplesPerBlock ? samplesPerBlock * 2 : fftSize * 2;

    //リングバッファ初期化 (入出力をステレオで初期化しているので2チャンネル)
    rbInL = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbInR = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbTmpL = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbTmpR = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbOutL = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);
    rbOutR = std::make_shared<Alstia::Buffer::RingBuffer>(bufferSize);

    //窓関数用配列初期化
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        window.data(), fftSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hamming);
}

void RadioFilterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool RadioFilterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void RadioFilterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    //入力
    auto* inL = buffer.getReadPointer(0);
    auto* inR = buffer.getReadPointer(1);
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        rbInL->Write(inL[sample]);
        rbInR->Write(inR[sample]);
    }

    //エフェクト処理
    processEffect(rbInL, rbTmpL, rbOutL);
    processEffect(rbInR, rbTmpR, rbOutR);

    //出力
    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    processOutSamples(rbOutL, outL, buffer.getNumSamples());
    processOutSamples(rbOutR, outR, buffer.getNumSamples());
}

//==============================================================================
bool RadioFilterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* RadioFilterAudioProcessor::createEditor()
{
    //現状で描画処理は自前で行わないのでデフォルトのものを使う
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void RadioFilterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    //パラメータの保存
    std::unique_ptr <juce::XmlElement> xml(new juce::XmlElement("AlstiaRadioFilter"));
    xml->setAttribute("gain", (double)*paramGain);
    xml->setAttribute("freqMin", (double)*paramFreqMin);
    xml->setAttribute("freqMax", (double)*paramFreqMax);
    copyXmlToBinary(*xml, destData);
}

void RadioFilterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    //パラメータの読み込み
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName("AlstiaRadioFilter")) {
            *paramGain = (float)xmlState->getDoubleAttribute("gain", 1.0);
            *paramFreqMin = (float)xmlState->getDoubleAttribute("freqMin", 650.0);
            *paramFreqMax = (float)xmlState->getDoubleAttribute("freqMax", 2500.0);
        }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RadioFilterAudioProcessor();
}



void RadioFilterAudioProcessor::processEffect(
    std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf,
    std::shared_ptr<Alstia::Buffer::RingBuffer>& tmpBuf,
    std::shared_ptr<Alstia::Buffer::RingBuffer>& outBuf)
{
    //シフト量計算
    const int fftShift = (int)(fftSize * overlap);

    //サンプルレート取得(44100.000...)
    const double sampleRate = getSampleRate();

    //FFTしてエフェクトをかける
    while (true) {
        //必要データ量、出力バッファをチェック
        if (inBuf->GetDataSize() < fftSize || (outBuf->GetBufferSize() - outBuf->GetDataSize()) < fftSize) break;

        /* エフェクト処理 */
        //処理用バッファ確保
        std::array<float, fftSize * 2> fftBuff;
        //コピー
        for (int i = 0; i < fftSize; i++) {
            fftBuff[i] = inBuf->Read(i);
        }
        //読み取った半分のデータをバッファから消す
        inBuf->Erase(fftShift);


        /* 窓関数を適用(ハミング窓) */
        juce::FloatVectorOperations::multiply(fftBuff.data(), window.data(), fftSize);


        /* FFT実行 */
        fft.performRealOnlyForwardTransform(fftBuff.data());


        /* 周波数フィルタ実行 */
        if (paramFreqMin->get() < paramFreqMax->get()) {
            for (int i = 0; i < fftSize; i++) {
                //Hz
                double hz = sampleRate / fftSize * i;

                if (!(hz >= paramFreqMin->get() && hz <= paramFreqMax->get())) {
                    fftBuff[i] = 0.0f;
                }
            }
        }


        /* IFFT実行 */
        fft.performRealOnlyInverseTransform(fftBuff.data());



        /* バッファに書き出し */
        //オーバーラップ処理をして出力バッファに書き出し (現フレームの前半データ+前フレームの後半データ)
        for (unsigned int i = 0; i < fftShift; i++) {
            outBuf->Write((fftBuff[i] + tmpBuf->Read(i)) * paramGain->get());
        }

        //今フレームで使用したオーバーラップ用のデータを削除
        tmpBuf->Erase(fftShift);

        //一時バッファに現フレームの後半データを格納
        for (unsigned int i = 0; i < fftShift; i++) {
            tmpBuf->Write(fftBuff[fftSize - fftShift + i]);
        }

        /* オーバーラップ参考資料
         * https://watlab-blog.com/2019/04/17/python-overlap/
         * http://www.kumikomi.net/archives/2010/09/ep30rir2.php?page=4
         */
    }
}

void RadioFilterAudioProcessor::processOutSamples(std::shared_ptr<Alstia::Buffer::RingBuffer>& inBuf, float* outBuf, const int numSamples)
{
    if (inBuf->GetDataSize() >= numSamples) {
        //バッファをコピー
        for (int i = 0; i < numSamples; i++) {
            outBuf[i] = inBuf->Read(i);
        }
        //バッファを消費扱いにする
        inBuf->Erase(numSamples);
    }
    else {
        //0埋め
        for (int i = 0; i < numSamples; i++) {
            outBuf[i] = 0;
        }
    }
}
